// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "integrator.h"

namespace prt {

// Primary occlusion
template <class Sampler>
class PoIntegratorPacket : public IntegratorBase
{
public:
  PoIntegratorPacket(const Props& props) : IntegratorBase(props) {}

  int getSampleSize() const
  {
    return sampleDimBaseSize;
  }

  Vec3vf getColor(RaySimd& ray, IntersectorPacket* intersector, IntersectorFilterSimd* filter, const Scene* scene, Sampler& sampler, IntegratorState<Sampler>& state)
  {
    intersector->occluded(one, ray, state.rayStats, RayHint::Coherent);
    return select(ray.isHit(), Vec3vf(1.0f, 0.5f, 0.0f), Vec3vf(0.05f));
  }
};

} // namespace prt
