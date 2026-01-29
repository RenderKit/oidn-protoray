// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/array.h"
#include "sys/props.h"
#include "geometry/group.h"
#include "geometry/triangle_mesh.h"
#include "material/material.h"
#include "light/light.h"
#include "sampling/distribution1d.h"

namespace prt {

class Scene
{
//private:
public:
  std::shared_ptr<Group> geometry;                    // scene geometry
  Array<std::shared_ptr<Material>> materials;         // all materials in the scene
  int lightMaterialCount;                             // number of area light materials
  Array<std::shared_ptr<Light>> lights;               // all lights in the scene (order: all area lights, all env lights)
  Distribution1D lightDistribution;                   // light sampling distribution
  Array<std::shared_ptr<AreaLight>> areaLights;       // area lights attached to the geometry
  Array<int> areaLightIds;                            // area light ID for each geomId
  Array<std::shared_ptr<EnvironmentLight>> envLights; // environment lights
  std::shared_ptr<Media> media;                       // all media in the scene
  Box3f bounds{empty};                                // scene bounds

  Array<std::string> materialNames; // material names of the geometry
  std::string path;                 // path to the scene
  Props props;

public:
  Scene(const std::string& path, const Props& props);

  void initLights(const Props& props);
  bool hasTransparentMaterials() const;

  std::string getPath() const
  {
    return path;
  }

  // Geometry
  // --------

  prt_inline void postIntersect(const Ray& ray, const Hit& hit, ShadingContext& ctx) const
  {
    geometry->postIntersect(ray, hit, ctx, 0);

    ctx.backfacing = dot(ctx.Ng, ray.dir) > 0.f;
    if (ctx.backfacing)
    {
      ctx.Ng = -ctx.Ng;
      ctx.f.N = -ctx.f.N;
    }
  }

  prt_inline void postIntersect(vbool m, const RaySimd& ray, const HitSimd& hit, ShadingContextSimd& ctx) const
  {
    geometry->postIntersect(m, ray, hit, ctx, 0);

    vbool backfacing = m & (dot(ctx.Ng, ray.dir) > 0.f);
    set(m, ctx.backfacing, backfacing);
    if (any(backfacing))
    {
      set(backfacing, ctx.Ng, -ctx.Ng);
      set(backfacing, ctx.f.N, -ctx.f.N);
    }
  }

  prt_inline void postIntersect(const Ray& ray, const Hit& hit, SimpleShadingContext& ctx) const
  {
    geometry->postIntersect(ray, hit, ctx, 0);

    bool backfacing = dot(ctx.Ng, ray.dir) > 0.f;
    if (backfacing)
      ctx.Ng = -ctx.Ng;
  }

  prt_inline void postIntersect(vbool m, const RaySimd& ray, const HitSimd& hit, SimpleShadingContextSimd& ctx) const
  {
    geometry->postIntersect(m, ray, hit, ctx, 0);

    vbool backfacing = m & (dot(ctx.Ng, ray.dir) > 0.f);
    if (any(backfacing))
      set(backfacing, ctx.Ng, -ctx.Ng);
  }

  prt_inline Box3f getBounds() const
  {
    return bounds;
  }

  prt_inline int getMaterialId(const Hit& hit) const
  {
    return geometry->getMaterialId(hit, 0);
  }

  prt_inline vint getMaterialId(vbool m, const HitSimd& hit) const
  {
    return geometry->getMaterialId(m, hit, 0);
  }

  prt_inline const Material* getMaterial(int matId) const
  {
    return materials[matId].get();
  }

  prt_inline std::string getMaterialName(int matId) const
  {
    return materialNames[matId];
  }

  prt_inline int getMaterialCount() const
  {
    return materials.getSize();
  }

  // Lights
  // ------

  prt_inline Vec3f evalEnvLight(const Vec3f& wi, float& pdf) const
  {
    Vec3f value = zero;
    pdf = zero;

    int envLightsOffset = areaLights.getSize();

    for (int i = 0; i < envLights.getSize(); ++i)
    {
      float curPdf;
      value += envLights[i]->eval(wi, curPdf);
      pdf += curPdf * lightDistribution.pdf(envLightsOffset + i);
    }

    return value;
  }

  prt_inline Vec3vf evalEnvLight(vbool m, const Vec3vf& wi, vfloat& pdf) const
  {
    Vec3vf value = zero;
    pdf = zero;

    int envLightsOffset = areaLights.getSize();

    for (int i = 0; i < envLights.getSize(); ++i)
    {
      vfloat curPdf;
      value += envLights[i]->eval(m, wi, curPdf);
      pdf += curPdf * lightDistribution.pdf(m, envLightsOffset + i);
    }

    return value;
  }

  prt_inline Vec3f evalAreaLight(const Vec3f& wi, float& pdf, float dist, const Hit& hit) const
  {
    // FIXME: only non-instanced area lights are supported
    if (hit.instId >= 0)
      return zero;

    int lightId = areaLightIds[hit.geomId];
    if (lightId < 0)
      return zero;
    Vec3f value = areaLights[lightId]->eval(wi, pdf, dist, hit);
    pdf *= lightDistribution.pdf(lightId);
    return value;
  }

  prt_inline Vec3vf evalAreaLight(vbool m, const Vec3vf& wi, vfloat& pdf, vfloat dist, const HitSimd& hit) const
  {
    Vec3vf value = zero;
    // FIXME: only non-instanced area lights are supported
    m &= hit.instId < 0;
    // For each unique geomId
    while (any(m))
    {
      int pos = bitScan(toIntMask(m));
      int geomId = hit.geomId[pos];
      vbool mCur = m & (hit.geomId == vint(geomId));

      int lightId = areaLightIds[geomId];
      if (lightId >= 0)
      {
        set(mCur, value, areaLights[lightId]->eval(mCur, wi, pdf, dist, hit));
        set(mCur, pdf, pdf * lightDistribution.pdf(lightId));
      }

      m = andn(m, mCur);
    };
    return value;
  }

  prt_inline Vec3f sampleLight(const ShadingContext& ctx, Vec3f& wi, float& pdf, float& dist, const LightSample& s) const
  {
    // If there's only one light, sample it
    if (lights.getSize() == 1)
      return lights[0]->sample(ctx, wi, pdf, dist, s);

    // Pick a light
    LightSample s1 = s;
    float pickProb;
    int lightId = lightDistribution.sampleReuse(pickProb, s1.c);

    // Sample the light
    pdf = zero;
    Vec3f weight = lights[lightId]->sample(ctx, wi, pdf, dist, s1);
    pdf *= pickProb;
    return weight * rcp(pickProb);
  }

  prt_inline Vec3vf sampleLight(vbool m, const ShadingContextSimd& ctx, Vec3vf& wi, vfloat& pdf, vfloat& dist, const LightSampleSimd& s) const
  {
    // If there's only one light, sample it
    if (lights.getSize() == 1)
      return lights[0]->sample(m, ctx, wi, pdf, dist, s);

    // Pick a light
    LightSampleSimd s1 = s;
    vfloat pickProb;
    vint lightId = lightDistribution.sampleReuse(m, pickProb, s1.c);

    // Sample the light
    Vec3vf weight;
    // For each unique ID
    while (any(m))
    {
      int pos = bitScan(toIntMask(m));
      int i = lightId[pos];
      vbool mCur = m & (lightId == vint(i));

      Vec3vf curWi;
      vfloat curPdf;
      vfloat curDist;
      Vec3vf curWeight = lights[i]->sample(mCur, ctx, curWi, curPdf, curDist, s1);

      set(mCur, &wi, curWi);
      set(mCur, &pdf, curPdf);
      set(mCur, &dist, curDist);
      set(mCur, &weight, curWeight);

      m = andn(m, mCur);
    }

    pdf *= pickProb;
    return weight * rcp(pickProb);
  }

  // Media
  // -----

  prt_inline const Media& getMedia() const
  {
    return *media;
  }

  // Misc
  // ----

  void mutateMaterials(Random& rng, bool mutateRegular, bool mutateEmissive);

private:
  void loadMesh(const std::string& path, const Props& props, MaterialFactory& matFactory);
  void loadScene(const std::string& path, const Props& props, MaterialFactory& matFactory);
};

} // namespace prt
