// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "integrator.h"

namespace prt {

// Primary occlusion
template <class Sampler>
class PoIntegratorSingle : public IntegratorBase
{
public:
  PoIntegratorSingle(const Props& props) : IntegratorBase(props) {}

  int getSampleSize() const
  {
    return sampleDimBaseSize;
  }

  Vec3f getColor(Ray& ray, IntersectorSingle* intersector, IntersectorFilter* filter, const Scene* scene, Sampler& sampler, IntegratorState<Sampler>& state)
  {
    intersector->occluded(ray, state.rayStats, RayHint::Coherent);
    return ray.isHit() ? Vec3f(1.0f, 0.5f, 0.0f) : 0.05f;
  }
};

} // namespace prt
