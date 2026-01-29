// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "math/random.h"
#include "integrator.h"

namespace prt {

template <class Sampler>
class DebugIntegratorSingle : public IntegratorBase
{
public:
  DebugIntegratorSingle(const Props& props)
    : IntegratorBase(props)
  {
  }

  int getSampleSize() const
  {
    return sampleDimBaseSize;
  }

  Vec3f getColor(Ray& ray, IntersectorSingle* intersector, IntersectorFilter* filter, const Scene* scene, Sampler& sampler, IntegratorState<Sampler>& state)
  {
    Hit hit;
    intersector->intersect(ray, hit, state.rayStats);
    if (!ray.isHit())
      return zero;

    ShadingContext ctx;
    scene->postIntersect(ray, hit, ctx);

    //return scene.getMaterial(ctx)->getColor(ctx);
    //return ctx.backfacing ? 0.0f : (ctx.N + 1.0f) * 0.5f;
    //return (ctx.Ng + 1.0f) * 0.5f;
    //return (ctx.N + 1.0f) * 0.5f;
    //return ctx.backfacing ? 0.0f : (ctx.N + 1.0f) * 0.5f;
    //return ctx.backfacing ? 0.0f : (ctx.Ng + 1.0f) * 0.5f;

    Random rng(hit.primId);
    //Random rng(hit.matId);
    return rng.get3f() * luminance((ctx.Ng + 1.0f) * 0.5f);
  }
};

} // namespace prt
