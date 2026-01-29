// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "integrator.h"

namespace prt {

template <class ShadingContextT, class Sampler>
class PrimaryIntegratorPacket : public IntegratorBase
{
public:
  PrimaryIntegratorPacket(const Props& props) : IntegratorBase(props) {}

  int getSampleSize() const
  {
    return sampleDimBaseSize;
  }

  Vec3vf getColor(RaySimd& ray, IntersectorPacket* intersector, IntersectorFilterSimd* filter, const Scene* scene, Sampler& sampler, IntegratorState<Sampler>& state)
  {
    HitSimd hit;
    vbool active = one;
    intersector->intersect(active, ray, hit, state.rayStats, RayHint::Coherent);
    active = ray.isHit();

    Vec3vf color = zero;
    if (any(active))
    {
      ShadingContextT ctx;
      scene->postIntersect(active, ray, hit, ctx);
      color = (ctx.getN() + vfloat(1.0f)) * vfloat(0.5f);
    }

    return select(active, color, Vec3vf(0.05f));
  }
};

} // namespace prt
