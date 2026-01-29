// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sampling/shape_sampler.h"
#include "integrator.h"

namespace prt {

// Diffuse path tracing with next event estimation (shadow rays)
template <class ShadingContextT, class Sampler>
class Diffuse2IntegratorSingle : public IntegratorBase
{
public:
  Diffuse2IntegratorSingle(const Props& props)
    : IntegratorBase(props)
  {
    maxDepth = props.get("maxDepth", 6);
  }

  int getSampleSize() const
  {
    return sampleDimBaseSize + 4 * maxDepth;
  }

  Vec3f getColor(Ray& ray, IntersectorSingle* intersector, IntersectorFilter* filter, const Scene* scene, Sampler& sampler, IntegratorState<Sampler>& state)
  {
    const float R = 0.8f;

    float L = zero;
    float throughput = 1.f;

    Hit hit;

    int depth = 0;
    for (; ;)
    {
      // Shoot the extension ray
      RayHint rayHint = (depth == 0) ? RayHint::Coherent : RayHint::Incoherent;
      intersector->intersect(ray, hit, state.rayStats, rayHint);

      if (!ray.isHit())
      {
        L += throughput * ((depth > 0) ? 0.5f : 1.f); // with MIS weight
        break;
      }

      // Path termination
      if (depth == maxDepth)
        break;

      // Shade the hit point
      ShadingContextT ctx;
      scene->postIntersect(ray, hit, ctx);
      throughput *= R;

      // Generate and shoot a shadow ray
      Vec2f s = sampler.get2D(state.sampler, sampleDimBaseSize + 4 * depth);
      ray.init(ctx.p, ctx.getFrame() * cosineSampleHemisphere(s), ctx.eps);

      intersector->occluded(ray, state.rayStats);

      if (!ray.isOccluded())
        L += throughput * 0.5f; // with MIS weight

      // Generate an extension ray
      s = sampler.get2D(state.sampler, sampleDimBaseSize + 4 * depth + 2);
      ray.init(ctx.p, ctx.getFrame() * cosineSampleHemisphere(s), ctx.eps);

      depth++;
    }

    return L;
  }
};

} // namespace prt
