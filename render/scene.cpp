// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "sys/logging.h"
#include "sys/blob.h"
#include "sys/filesystem.h"
#include "geometry/instance.h"
#include "geometry/triangle_mesh.h"
#include "image/image_texture.h"
#include "material/mtl_loader.h"
#include "material/material_factory.h"
#include "light/ambient_light.h"
#include "light/distant_light.h"
#include "light/image_light.h"
#include "light/sun_sky_light.h"
#include "light/triangle_light.h"
#include "scene.h"

#define RKSCENE_FLOAT2   prt::Vec2f
#define RKSCENE_FLOAT3   prt::Vec3f
#define RKSCENE_FLOAT4   prt::Vec4f
#define RKSCENE_FLOAT3X4 prt::Affine3f
#if defined(PRT_CONVERT_SUPPORT)
  #define RKSCENE_GZIP_SUPPORT
#endif
#include "rkscene/rkscene.h"
#include "rkscene/rkscene_gltf.h"

namespace prt {

namespace
{
  template<typename T>
  void setMaterialParam(Props& props, const std::string& name, const T& param)
  {
    props.set(name, param);
  }

  template<typename FactorT>
  void setMaterialParam(Props& props, const std::string& name, const rkscene::Texture<FactorT>& param)
  {
    props.set(name, param.factor);
    if (!param.image.empty())
    {
      props.set(name + "Map", param.image);
      if (!param.swizzle.empty())
        props.set(name + "MapSwizzle", param.swizzle);
      props.set(name + "MapUV", param.texcoord);
      props.set(name + "MapUVOffset", param.transform.offset);
      props.set(name + "MapUVScale", param.transform.scale);
    }
  }

  void setMaterialParam(Props& props, const std::string& normalName, const std::string& bumpName,
                        const rkscene::TextureNormal3f& param)
  {
    if (!param.image.empty())
    {
      std::string name = (param.type == rkscene::TextureType::Bump) ? bumpName : normalName;
      setMaterialParam(props, name, param);
    }
  }
}

Scene::Scene(const std::string& path, const Props& props)
{
  this->path = path;
  std::string pathExt = getFilenameExt(path);

  MaterialFactory materialFactory(getFilenamePrefix(path), props);

  if (pathExt == "mesh")
    loadMesh(path, props, materialFactory);
  else if (pathExt == "rkscene" || pathExt == "gltf")
    loadScene(path, props, materialFactory);
  else
    throw std::runtime_error("unknown scene format");

  Log() << "Materials: " << materials.getSize();

  // Compute scene bounds
  bounds = geometry->getBounds();

  // Create area lights
  // FIXME: only non-instanced triangle lights are supported
  areaLightIds.alloc(geometry->children.getSize());
  for (int i = 0; i < geometry->children.getSize(); ++i)
  {
    areaLightIds[i] = -1;

    if (auto mesh = std::dynamic_pointer_cast<TriangleMesh>(geometry->children[i]))
    {
      // Check whether the mesh is emissive
      bool isEmissive = false;
      for (int primId = 0; primId < mesh->getPrimCount(); ++primId)
      {
        const int matId = mesh->materialIds[(mesh->materialIds.getSize() > 1) ? primId : 0];
        const Material* mat = materials[matId].get();
        if (mat->getType() & MaterialType::Emissive)
        {
          isEmissive = true;
          break;
        }
      }

      if (isEmissive)
      {
        auto triLight = std::make_shared<TriangleLight>(mesh, materials);
        if (triLight->getTriangleCount() > 0)
        {
          areaLightIds[i] = areaLights.getSize();
          areaLights.pushBack(triLight);
        }
      }
    }
  }

  initLights(props);

  // Count the number of area light materials
  lightMaterialCount = 0;
  for (int i = 1; i < materials.getSize(); ++i)
  {
    if (materials[i]->getType() & MaterialType::Emissive)
    {
      lightMaterialCount++;
      if (i > lightMaterialCount)
        throw std::runtime_error("the material IDs must be remapped");
    }
  }

  // Set up media
  media = materialFactory.getMedia();
}

void Scene::loadMesh(const std::string& path, const Props& props, MaterialFactory& materialFactory)
{
  std::string pathBase = getFilenameBase(path);
  this->props = props;

  // Load the mesh
  std::shared_ptr<TriangleMesh> mesh = std::make_shared<TriangleMesh>();
  loadBlob(path, *mesh);

  geometry = std::make_shared<Group>();
  geometry->children.pushBack(mesh);

  // Load the materials
  std::string mtlPath = pathBase + ".mtl";

  PropsMap matLib;
  if (File::exists(mtlPath) && !props.exists("no-mtl"))
    MtlLoader(mtlPath, matLib);

  auto defaultMaterial = materialFactory.makeDefault();
  materialNames = mesh->getMaterialNames();
  if (!materialNames.isEmpty())
  {
    for (const std::string& matName : materialNames)
    {
      auto mat = matLib.find(matName);
      if (mat != matLib.end())
        materials.pushBack(materialFactory.make(mat->second));
      else
        materials.pushBack(defaultMaterial);
    }
  }
  else
  {
    materials.pushBack(defaultMaterial);
  }
}

void Scene::loadScene(const std::string& path, const Props& props, MaterialFactory& materialFactory)
{
  this->geometry = std::make_shared<Group>();

  std::shared_ptr<rkscene::Scene> inScene;

  std::string pathExt = getFilenameExt(path);
  if (pathExt == "rkscene")
  {
    Log() << "Loading rkscene: " << path;
    inScene = rkscene::load(path);
  }
  else if (pathExt == "gltf")
  {
    Log() << "Loading glTF: " << path;
    inScene = rkscene::loadGLTF(path);
  }
  else
  {
    throw std::runtime_error("unsupported scene format: " + pathExt);
  }

  const bool flatten = props.get("flatten", 0); // flatten instances

  std::unordered_map<int, std::shared_ptr<Geometry>> geomMap;
  std::unordered_map<int, int> materialMap;
  std::vector<int> inMatIds;

  // Add dummy material with ID 0 reserved for the environment light
  materials.pushBack(materialFactory.makeDefault());
  materialNames.pushBack("");
  inMatIds.push_back(-1);

  // Process materials
  for (int inMatId = 0; inMatId < (int)inScene->materials.size(); ++inMatId)
  {
    const auto& inMat = inScene->materials[inMatId];

    Props matProps;
    matProps.set("type", "Standard");

    setMaterialParam(matProps, "name", inMat->name);
    setMaterialParam(matProps, "baseWeight", inMat->baseWeight);
    setMaterialParam(matProps, "baseColor", inMat->baseColor);
    setMaterialParam(matProps, "metalness", inMat->metalness);
    setMaterialParam(matProps, "diffuseRoughness", inMat->diffuseRoughness);
    setMaterialParam(matProps, "normal", "bump", inMat->normal);
    setMaterialParam(matProps, "specularWeight", inMat->specularWeight);
    setMaterialParam(matProps, "specularColor", inMat->specularColor);
    setMaterialParam(matProps, "specularRoughness", inMat->specularRoughness);
    setMaterialParam(matProps, "specularIOR", inMat->specularIor);
    setMaterialParam(matProps, "transmissionWeight", inMat->transmissionWeight);
    setMaterialParam(matProps, "transmissionColor", inMat->transmissionColor);
    setMaterialParam(matProps, "transmissionDepth", inMat->transmissionDepth);
    setMaterialParam(matProps, "subsurfaceWeight", inMat->subsurfaceWeight);
    setMaterialParam(matProps, "subsurfaceColor", inMat->subsurfaceColor);
    setMaterialParam(matProps, "subsurfaceAnisotropy", inMat->subsurfaceAnisotropy);
    setMaterialParam(matProps, "coatWeight", inMat->coatWeight);
    setMaterialParam(matProps, "coatColor", inMat->coatColor);
    setMaterialParam(matProps, "coatRoughness", inMat->coatRoughness);
    setMaterialParam(matProps, "coatNormal", "coatBump", inMat->coatNormal);
    setMaterialParam(matProps, "coatIOR", inMat->coatIor);
    setMaterialParam(matProps, "sheenWeight", inMat->sheenWeight);
    setMaterialParam(matProps, "sheenColor", inMat->sheenColor);
    setMaterialParam(matProps, "sheenRoughness", inMat->sheenRoughness);
    setMaterialParam(matProps, "emissionLuminance", inMat->emissionLuminance);
    setMaterialParam(matProps, "emissionColor", inMat->emissionColor);
    setMaterialParam(matProps, "thinWalled", inMat->thinWalled);
    setMaterialParam(matProps, "opacity", inMat->opacity);

    materials.pushBack(materialFactory.make(matProps));
    materialNames.pushBack(inMat->name);
    inMatIds.push_back(inMatId);
  }

  // Add default material for missing materials
  materials.pushBack(materialFactory.makeDefault());
  materialNames.pushBack("");
  inMatIds.push_back(-1);

  // Reorder materials to have emissive materials first
  int nextLightMatId = 1;
  for (int i = 1; i < materials.getSize(); ++i)
  {
    if (materials[i]->getType() & MaterialType::Emissive)
    {
      if (i != nextLightMatId)
      {
        std::swap(materials[i], materials[nextLightMatId]);
        std::swap(materialNames[i], materialNames[nextLightMatId]);
        std::swap(inMatIds[i], inMatIds[nextLightMatId]);
      }
      nextLightMatId++;
    }
  }

  for (int i = 1; i < inMatIds.size(); ++i)
    materialMap[inMatIds[i]] = i;

  // Process objects
  for (auto& inObj : inScene->objects)
  {
    auto group = std::make_shared<Group>();
    bool isEmissive = false;

    for (auto& inGeomId : inObj->geometries)
    {
      std::shared_ptr<Geometry> geom;

      if (geomMap.find(inGeomId) == geomMap.end())
      {
        const auto& inGeom = inScene->geometries[inGeomId];

        auto mesh = std::make_shared<TriangleMesh>();
        geom = mesh;
        geomMap[inGeomId] = geom;

        mesh->alloc(inGeom->getNumPrimitives(), inGeom->positions.size(), !inGeom->normals.empty(),
                    static_cast<int>(inGeom->texcoords.size()), false);

        int materialId = materialMap[-1]; // default material
        if (inGeom->material >= 0)
        {
          const int inMatId = inGeom->material;
          auto mat = materialMap.find(inMatId);
          if (mat != materialMap.end())
            materialId = mat->second;
        }

        // Copy indices
        for (int i = 0; i < mesh->getPrimCount(); ++i)
          mesh->indices[i] = Vec3i(inGeom->indices[i*3+0], inGeom->indices[i*3+1], inGeom->indices[i*3+2]);

        // Copy vertex positions
        for (int i = 0; i < mesh->getVertexCount(); ++i)
          mesh->setPosition(i, inGeom->positions[i]);

        // Copy vertex normals
        if (!inGeom->normals.empty())
        {
          for (int i = 0; i < mesh->getVertexCount(); ++i)
            mesh->setNormal(i, inGeom->normals[i]);
        }

        // Copy vertex texcoords
        for (int t = 0; t < static_cast<int>(inGeom->texcoords.size()); ++t)
        {
          for (int i = 0; i < mesh->getVertexCount(); ++i)
            mesh->setTexcoord(t, i, inGeom->texcoords[t][i]);
        }

        // Copy material ID
        mesh->materialIds[0] = materialId;
      }
      else
        geom = geomMap[inGeomId];

      group->children.pushBack(geom);

      int inMatId = inScene->geometries[inGeomId]->material;
      if (inMatId >= 0)
        isEmissive |= materials[materialMap[inMatId]]->getType() & MaterialType::Emissive;
    }

    if (inObj->instances.empty())
    {
      for (const auto& child : group->children)
        this->geometry->children.pushBack(child);
    }
    else if (flatten || isEmissive) // FIXME: add support for instanced area lights
    {
      // Flatten instances
      for (auto transform : inObj->instances)
      {
        for (const auto& child : group->children)
          this->geometry->children.pushBack(child->clone(transform));
      }
    }
    else
    {
      for (auto transform : inObj->instances)
      {
        auto instance = std::make_shared<Instance>();
        instance->child = group;
        instance->transform = transform;
        this->geometry->children.pushBack(instance);
      }
    }
  }
}

void Scene::initLights(const Props& props)
{
  const float sceneRadius = length(bounds.getSize()) * 0.5f;

  lights.clear();
  envLights.clear();

  // Add area lights
  for (const auto& areaLight : areaLights)
    lights.pushBack(areaLight);

  // Add distant light
  Vec3f distantLightColor = props.get("distantLight", Vec3f(zero));
  if (reduceMax(distantLightColor) > 0.0f)
  {
    Props lightProps;
    lightProps.set("Ke", distantLightColor);

    if (props.exists("distantLightDir"))
      lightProps.set("direction", props.get<Vec3f>("distantLightDir"));

    if (props.exists("distantLightAngle"))
      lightProps.set("angle", props.get<float>("distantLightAngle"));

    envLights.pushBack(std::make_shared<DistantLight>(lightProps, sceneRadius));
  }

  // Add ambient light
  Vec3f ambientLightColor = props.get("ambientLight", Vec3f(zero));
  if (reduceMax(ambientLightColor) > 0.0f)
  {
    Props lightProps;
    lightProps.set("Ke", ambientLightColor);
    envLights.pushBack(std::make_shared<AmbientLight>(lightProps, sceneRadius));
  }

  // Add image light
  std::string imageLightFilename = props.get("imageLight", "");
  if (!imageLightFilename.empty())
  {
    std::shared_ptr<ImageTextureBuffer> texture = loadImageTexture(imageLightFilename);
    Props lightProps;
    if (props.exists("imageLightLutSize")) lightProps.set("lutSize", props.get<int>("imageLightLutSize"));
    envLights.pushBack(std::make_shared<ImageLight>(texture, lightProps, sceneRadius));
  }

  // Add sun/sky light
  Vec3f sunSkyLightDir = props.get("sunSkyLightDir", Vec3f(zero));
  if (sunSkyLightDir != Vec3f(zero))
  {
    Props lightProps;
    lightProps.set("direction", sunSkyLightDir);
    if (props.exists("sunSkyLightTurbidity")) lightProps.set("turbidity", props.get<float>("sunSkyLightTurbidity"));
    if (props.exists("sunSkyLightAlbedo")) lightProps.set("albedo", props.get<float>("sunSkyLightAlbedo"));
    if (props.exists("sunSkyLightHorizon")) lightProps.set("horizon", props.get<float>("sunSkyLightHorizon"));
    if (props.exists("sunSkyLightRadiusScale")) lightProps.set("radiusScale", props.get<float>("sunSkyLightRadiusScale"));
    if (props.exists("sunSkyLightSunScale")) lightProps.set("sunScale", props.get<float>("sunSkyLightSunScale"));
    if (props.exists("sunSkyLightSkyScale")) lightProps.set("skyScale", props.get<float>("sunSkyLightSkyScale"));
    if (props.exists("sunSkyLightSunSaturation")) lightProps.set("sunSaturation", props.get<float>("sunSkyLightSunSaturation"));
    if (props.exists("sunSkyLightSkySaturation")) lightProps.set("skySaturation", props.get<float>("sunSkyLightSkySaturation"));
    if (props.exists("sunSkyLightSunHue")) lightProps.set("sunHue", props.get<float>("sunSkyLightSunHue"));
    if (props.exists("sunSkyLightSkyHue")) lightProps.set("skyHue", props.get<float>("sunSkyLightSkyHue"));

    SunSkyLight sunSkyLight(lightProps, sceneRadius);
    envLights.pushBack(sunSkyLight.getSunLight());
    envLights.pushBack(sunSkyLight.getSkyLight());
  }

  // If there are no lights at all, add a default ambient light
  if (lights.isEmpty() && envLights.isEmpty())
  {
    LogWarn() << "No lights specified, adding default ambient light";
    Props lightProps;
    envLights.pushBack(std::make_shared<AmbientLight>(lightProps, sceneRadius));
  }

  // Add all environment lights to the light list
  for (int i = 0; i < envLights.getSize(); ++i)
    lights.pushBack(envLights[i]);

  // Build light distribution
  Array<float> lightWeights;
  for (int i = 0; i < lights.getSize(); ++i)
    lightWeights.pushBack(lights[i]->getPower());
  lightDistribution.init(lightWeights.getData(), lightWeights.getSize());
}

bool Scene::hasTransparentMaterials() const
{
  if (props.exists("no-transShadow"))
    return false;

  for (int i = 0; i < materials.getSize(); ++i)
    if (materials[i]->getType() & MaterialType::Transparent)
      return true;

  return false;
}

void Scene::mutateMaterials(Random& rng, bool mutateRegular, bool mutateEmissive)
{
  // Mutate materials
  for (int i = 0; i < materials.getSize(); ++i)
  {
    bool isEmissive = materials[i]->getType() & MaterialType::Emissive;
    if ((mutateRegular && !isEmissive) || (mutateEmissive && isEmissive))
      materials[i]->mutate(rng);
  }

  // Update area lights
  if (mutateEmissive)
  {
    for (const auto& areaLight : areaLights)
      areaLight->update(materials);
  }

  // Update media
  media->update();
}

} // namespace prt
