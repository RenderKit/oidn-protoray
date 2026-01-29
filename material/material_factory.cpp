// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "sys/filesystem.h"
#include "image/image_texture.h"
#include "image/bump.h"
#include "material.h"
#include "standard.h"
#include "principled.h"
#include "car_paint.h"
#include "marble.h"
#include "luminous.h"
#include "material_factory.h"

namespace prt {

MaterialFactory::MaterialFactory(const std::string& path, const Props& props)
{
  // Add default medium
  media = std::make_shared<Media>();
  media->add(std::make_shared<Medium>(one));

  texturingEnabled = !props.exists("no-tex");
  flipNormalMapYEnabled = props.exists("flipNormalMapY");
  emissiveEnabled = !props.exists("no-emissive") && !props.exists("no-emission");
  this->path = path;
}

std::shared_ptr<Material> MaterialFactory::make(const Props& props)
{
  std::string type = props.get("type", "OBJ");

  if (type == "Standard")
    return std::make_shared<StandardMaterial>(props, *this);
  if (type == "Principled") // deprecated
    return std::make_shared<PrincipledMaterial>(props, *this);

  // Backward compatibility
  if (type == "OBJ")
  {
    Props props2 = props;

    if (!props.exists("baseColor") && !props.exists("baseColorMap"))
    {
      props2.copy("baseColor", "Kd");
      props2.copy("baseColorMap", "map_Kd");
    }

    if (!props.exists("bumpMap") && !props.exists("normalMap"))
    {
      props2.copy("bumpMap", "map_Bump");
      props2.copy("normalMap", "map_bump");
    }

    if (!props.exists("opacityMap"))
      props2.copy("opacityMap", "map_d");

    if (!props.exists("roughness") && props.exists("Ns"))
    {
      float Ns = props.get<float>("Ns");
      float roughness = sqrt(sqrt(2.f / (Ns + 2.f))); // [Walter et al., 2007]
      props2.set("roughness", roughness);
    }

    if (!props.exists("transmission") && props.exists("Tf"))
    {
      Vec3f transmissionColor = props.get<Vec3f>("Tf");
      if (reduceMin(transmissionColor) < 1.f)
      {
        props2.set("transmission", 1.f);
        props2.set("transmissionColor", 1.f - transmissionColor); // 3ds Max
      }
    }

    if (!props.exists("ior") && props.exists("Ks"))
    {
      float f0 = average(props.get<Vec3f>("Ks")) * 0.08f;
      float ior = (1.f + sqrt(f0)) / (1.f - sqrt(f0));
      props2.set("ior", ior);
    }

    return std::make_shared<PrincipledMaterial>(props2, *this);
  }

  if (type == "CarPaint")
    return std::make_shared<CarPaintMaterial>(props, *this);

  if (type == "Luminous")
  {
    // Backward compatibility
    Props props2 = props;
    props2.copy("color", "Ke");

    return std::make_shared<LuminousMaterial>(props2, *this);
  }

  if (type == "marble")
    return std::make_shared<MarbleMaterial>(props, *this);

  return nullptr;
}

std::shared_ptr<Material> MaterialFactory::makeDefault()
{
  Props props;
  return std::make_shared<PrincipledMaterial>(props, *this);
}

bool MaterialFactory::isEmissive(const Props& props)
{
  std::string type = props.get("type", "Principled");
  return type == "Luminous";
}

std::shared_ptr<ImageTexture> MaterialFactory::makeTexture(const std::string& filename, const ImageTextureParams& params)
{
  if (!texturingEnabled)
    return 0;

  auto iter = textures.find(filename);
  std::shared_ptr<ImageTextureBuffer> texture;

  if (iter == textures.end())
  {
    texture = loadImageTexture(path + filename);
    textures.insert(std::make_pair(filename, texture));
  }
  else
  {
    texture = iter->second;
  }

  return texture->getTexture(params);
}

std::shared_ptr<ImageTexture> MaterialFactory::makeBumpNormalTexture(const std::string& filename, const ImageTextureParams& params)
{
  if (!texturingEnabled)
    return 0;

  auto iter = bumpNormalTextures.find(filename);
  std::shared_ptr<ImageTextureBuffer> texture;

  if (iter == bumpNormalTextures.end())
  {
    std::shared_ptr<Image<int>> image = bumpToNormalMap(makeTexture(filename, ImageTextureParams(ColorSpace::Linear)));
    //saveImage(getFilenameBase(filename) + "_normal.png", *image);
    texture = std::make_shared<ImageTextureBuffer>(image, PixelFormat::Bgrx8);
    bumpNormalTextures.insert(std::make_pair(filename, texture));
  }
  else
  {
    texture = iter->second;
  }

  return texture->getTexture(params);
}

std::shared_ptr<Medium> MaterialFactory::makeMedium(const Vec3f& color, float ior, int& id)
{
  Medium medium(color, ior);

  for (int i = 0; i < media->getSize(); ++i)
  {
    if (*media->get(i) == medium)
    {
      id = i;
      return media->get(i);
    }
  }

  std::shared_ptr<Medium> newMedium = std::make_shared<Medium>(medium);
  id = media->add(newMedium);
  return newMedium;
}

} // namespace prt

