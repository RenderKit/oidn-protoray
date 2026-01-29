// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "core/intersector.h"
#include "scene.h"

namespace prt {

class TransparentShadowFilter : public IntersectorFilter
{
private:
  std::shared_ptr<const Scene> scene;

public:
  TransparentShadowFilter(const std::shared_ptr<const Scene>& scene)
    : scene(scene)
  {
  }

  bool filter(const Ray& ray, const Hit& hit, void* payload) override
  {
    Vec3f& T = *(Vec3f*)payload;

    int matId = scene->getMaterialId(hit);
    const Material* mat = scene->getMaterial(matId);

    if (mat->getType() & MaterialType::Transparent)
    {
      ShadingContext ctx;
      scene->postIntersect(ray, hit, ctx);
      T *= mat->getTransparency(ctx, -ray.dir);
      return reduceMax(T) < 1e-5f;
    }

    T = 0.f;
    return true;
  }
};

class TransparentShadowFilterSimd : public IntersectorFilterSimd
{
private:
  std::shared_ptr<const Scene> scene;

public:
  TransparentShadowFilterSimd(const std::shared_ptr<const Scene>& scene)
    : scene(scene)
  {
  }

  vbool filter(vbool m, vint rayId, const RaySimd& ray, const HitSimd& hit, void* payload) override
  {
    float** T = (float**)payload;
    vbool valid = one;

    vint matId = scene->getMaterialId(m, hit);

    // For each unique material ID
    vbool mLoop = m;
    while (any(mLoop))
    {
      vbool mCur;
      int i = nextUnique(mLoop, matId, mCur);

      const Material* mat = scene->getMaterial(i);

      if (mat->getType() & MaterialType::Transparent)
      {
        ShadingContextSimd ctx;
        scene->postIntersect(mCur, ray, hit, ctx);
        Vec3vf transparency = mat->getTransparency(mCur, ctx, -ray.dir);
        Vec3vf Tcur(gather(mCur, T[0], rayId),
                    gather(mCur, T[1], rayId),
                    gather(mCur, T[2], rayId));
        Tcur *= transparency;
        scatter(mCur, T[0], rayId, Tcur.x);
        scatter(mCur, T[1], rayId, Tcur.y);
        scatter(mCur, T[2], rayId, Tcur.z);
        valid = select(mCur, reduceMax(Tcur) < 1e-5f, valid);
      }
      else
      {
        scatter(mCur, T[0], rayId, vfloat(0.f));
        scatter(mCur, T[1], rayId, vfloat(0.f));
        scatter(mCur, T[2], rayId, vfloat(0.f));
      }
    }

    return valid;
  }
};

} // namespace prt
