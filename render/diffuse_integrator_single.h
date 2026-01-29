// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sampling/shape_sampler.h"
#include "integrator.h"

namespace prt {

template <class ShadingContextT, class Sampler>
class DiffuseIntegratorSingle : public IntegratorBase
{
public:
  DiffuseIntegratorSingle(const Props& props)
    : IntegratorBase(props)
  {
    maxDepth = props.get("maxDepth", 6);
  }

  int getSampleSize() const
  {
    return sampleDimBaseSize + 2 * maxDepth;
  }

  Vec3f getColor(Ray& ray, IntersectorSingle* intersector, IntersectorFilter* filter, const Scene* scene, Sampler& sampler, IntegratorState<Sampler>& state)
  {
    const float R = 0.8f;
    float Lw = 1.0f;

    Hit hit;

    int depth = 0;
    for (; ;)
    {
      RayHint rayHint = (depth == 0) ? RayHint::Coherent : RayHint::Incoherent;
      intersector->intersect(ray, hit, state.rayStats, rayHint);

      if (!ray.isHit() || depth == maxDepth)
        break;

      Lw *= R;

      ShadingContextT ctx;
      scene->postIntersect(ray, hit, ctx);

      Vec2f s = sampler.get2D(state.sampler, sampleDimBaseSize + 2 * depth);
      ray.init(ctx.p, ctx.getFrame() * cosineSampleHemisphere(s), ctx.eps);
      depth++;
    }

    if (ray.isHit()) return zero;
    return Lw;
  }
};

} // namespace prt
