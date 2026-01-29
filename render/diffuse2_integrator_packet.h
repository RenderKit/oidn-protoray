// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sampling/shape_sampler.h"
#include "integrator.h"

namespace prt {

// Diffuse path tracing with next event estimation (shadow rays)
template <class ShadingContextT, class Sampler>
class Diffuse2IntegratorPacket : public IntegratorBase
{
public:
  Diffuse2IntegratorPacket(const Props& props)
    : IntegratorBase(props)
  {
    maxDepth = props.get("maxDepth", 6);
  }

  int getSampleSize() const
  {
    return sampleDimBaseSize + 4 * maxDepth;
  }

  Vec3vf getColor(RaySimd& ray, IntersectorPacket* intersector, IntersectorFilterSimd* filter, const Scene* scene, Sampler& sampler, IntegratorState<Sampler>& state)
  {
    static const vfloat R = 0.8f;

    vfloat L = zero;
    vfloat throughput = 1.f;

    HitSimd hit;
    vbool m = one;

    int depth = 0;
    for (; ;)
    {
      // Shoot the extension ray
      RayHint rayHint = (depth == 0) ? RayHint::Coherent : RayHint::Incoherent;
      intersector->intersect(m, ray, hit, state.rayStats, rayHint);

      vbool mMiss = andn(m, ray.isHit());
      if (any(mMiss))
      {
        set(mMiss, L, L + (throughput * ((depth > 0) ? 0.5f : 1.f))); // with MIS weight
        m = andn(m, mMiss);
        if (none(m))
          break;
      }

      // Path termination
      if (depth == maxDepth)
        break;

      // Shade the hit point
      ShadingContextT ctx;
      scene->postIntersect(m, ray, hit, ctx);
      throughput *= R;

      // Generate and shoot a shadow ray
      Vec2vf s = sampler.get2D(m, state.sampler, sampleDimBaseSize + 4 * depth);
      ray.init(ctx.p, ctx.getFrame() * cosineSampleHemisphere(s), ctx.eps);

      intersector->occluded(m, ray, state.rayStats);

      vbool mDirMiss = andn(m, ray.isOccluded());
      if (any(mDirMiss))
        set(mDirMiss, L, L + (throughput * 0.5f)); // with MIS weight

      // Generate an extension ray
      s = sampler.get2D(m, state.sampler, sampleDimBaseSize + 4 * depth + 2);
      ray.init(ctx.p, ctx.getFrame() * cosineSampleHemisphere(s), ctx.eps);

      depth++;
    }

    return L;
  }
};

} // namespace prt
