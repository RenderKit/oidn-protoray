// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sampling/shape_sampler.h"
#include "integrator.h"

namespace prt {

template <class ShadingContextT, class Sampler>
class AoIntegratorPacket : public IntegratorBase
{
private:
  int sampleCount;
  float invSampleCount;

public:
  AoIntegratorPacket(const Props& props)
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

  Vec3vf getColor(RaySimd& ray, IntersectorPacket* intersector, IntersectorFilterSimd* filter, const Scene* scene, Sampler& sampler, IntegratorState<Sampler>& state)
  {
    HitSimd hit;
    intersector->intersect(one, ray, hit, state.rayStats, RayHint::Coherent);
    vbool active = ray.isHit();
    if (none(active)) return zero;

    ShadingContextT ctx;
    scene->postIntersect(active, ray, hit, ctx);

    vfloat sum = zero;
    for (int i = 0; i < sampleCount; ++i)
    {
      Vec2vf s = sampler.get2D(active, state.sampler, sampleDimBaseSize + 2 * i);
      ray.init(ctx.p, ctx.getFrame() * cosineSampleHemisphere(s), ctx.eps);

      //intersector->intersect(active, ray, hit, state.rayStats);
      intersector->occluded(active, ray, state.rayStats);
      sum += select(ray.isHit(), vfloat(zero), vfloat(one));
    }

    return select(active, sum * invSampleCount, zero);
  }
};

} // namespace prt
