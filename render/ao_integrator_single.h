// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sampling/shape_sampler.h"
#include "integrator.h"

namespace prt {

template <class Sampler>
class AoIntegratorSingle : public IntegratorBase
{
private:
  int sampleCount;
  float invSampleCount;

public:
  AoIntegratorSingle(const Props& props)
    : IntegratorBase(props)
  {
    sampleCount = props.get("samples", 16);
    if (sampleCount < 1)
      throw std::invalid_argument("integrator samples must be at least 1");
    invSampleCount = 1.0f / (float)sampleCount;
  }

  int getSampleSize() const
  {
    return sampleDimBaseSize + 2 * sampleCount;
  }

  Vec3f getColor(Ray& ray, IntersectorSingle* intersector, IntersectorFilter* filter, const Scene* scene, Sampler& sampler, IntegratorState<Sampler>& state)
  {
    Hit hit;
    intersector->intersect(ray, hit, state.rayStats, RayHint::Coherent);
    if (!ray.isHit()) return zero;

    ShadingContext ctx;
    scene->postIntersect(ray, hit, ctx);

    float sum = zero;
    for (int i = 0; i < sampleCount; ++i)
    {
      Vec2f s = sampler.get2D(state.sampler, sampleDimBaseSize + 2 * i);
      ray.init(ctx.p, ctx.getFrame() * cosineSampleHemisphere(s), ctx.eps);

      intersector->occluded(ray, state.rayStats);
      if (!ray.isHit()) sum += 1.0f;
    }

    return sum * invSampleCount;
  }
};

} // namespace prt
