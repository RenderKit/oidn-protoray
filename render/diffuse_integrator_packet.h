// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sampling/shape_sampler.h"
#include "integrator.h"

namespace prt {

template <class ShadingContextT, class Sampler>
class DiffuseIntegratorPacket : public IntegratorBase
{
public:
  DiffuseIntegratorPacket(const Props& props)
    : IntegratorBase(props)
  {
    maxDepth = props.get("maxDepth", 6);
  }

  int getSampleSize() const
  {
    return sampleDimBaseSize + 2 * maxDepth;
  }

  Vec3vf getColor(RaySimd& ray, IntersectorPacket* intersector, IntersectorFilterSimd* filter, const Scene* scene, Sampler& sampler, IntegratorState<Sampler>& state)
  {
    static const vfloat R = 0.8f;
    vfloat Lw = 1.0f;

    HitSimd hit;
    vbool active = one;

    int depth = 0;
    for (; ;)
    {
      RayHint rayHint = (depth == 0) ? RayHint::Coherent : RayHint::Incoherent;
      intersector->intersect(active, ray, hit, state.rayStats, rayHint);

      active &= ray.isHit();
      if (none(active) || depth == maxDepth)
        break;

      set(active, Lw, Lw * R);

      ShadingContextT ctx;
      scene->postIntersect(active, ray, hit, ctx);

      Vec2vf s = sampler.get2D(active, state.sampler, sampleDimBaseSize + 2 * depth);
      ray.init(ctx.p, ctx.getFrame() * cosineSampleHemisphere(s), ctx.eps);
      depth++;
    }

    set(active, Lw, vfloat(zero));
    return Lw;
  }
};

} // namespace prt
