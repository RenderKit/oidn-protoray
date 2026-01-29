// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "integrator.h"

namespace prt {

template <class ShadingContextT, class Sampler>
class PrimaryIntegratorSingle : public IntegratorBase
{
public:
  PrimaryIntegratorSingle(const Props& props) : IntegratorBase(props) {}

  int getSampleSize() const
  {
    return sampleDimBaseSize;
  }

  Vec3f getColor(Ray& ray, IntersectorSingle* intersector, IntersectorFilter* filter, const Scene* scene, Sampler& sampler, IntegratorState<Sampler>& state)
  {
    Hit hit;
    intersector->intersect(ray, hit, state.rayStats, RayHint::Coherent);

    if (ray.isHit())
    {
      ShadingContextT ctx;
      scene->postIntersect(ray, hit, ctx);
      return (ctx.getN() + 1.0f) * 0.5f;
    }

    return 0.05f;
  }
};

} // namespace prt
