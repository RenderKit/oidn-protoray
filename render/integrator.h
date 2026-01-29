// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/memory.h"
#include "core/intersector_single.h"
#include "core/intersector_packet.h"
#include "scene.h"

namespace prt {

template <class Sampler>
struct prt_align_cache IntegratorState
{
  typename Sampler::State sampler;
  RayStats rayStats;
};

class IntegratorBase
{
protected:
  int maxDepth;
  float maxRadiance;

  // Clamps radiance by luminance scaling
  template <class T>
  prt_inline Vec3<T> clampL(const Vec3<T>& L, bool scale = true)
  {
    // Clamp to zero safely (zero if NaN)
    Vec3<T> c(select(0.f < L.x, L.x, 0.f),
          select(0.f < L.y, L.y, 0.f),
          select(0.f < L.z, L.z, 0.f));

    // Luminance scaling
    if (scale)
    {
      // Compute luminance
      T y = luminance(c);

      // Scale if clamping is necessary
      c *= select(y > maxRadiance, maxRadiance * rcp(y), 1.f);
    }

    return c;
  }

public:
  // Sample dimensions
  enum
  {
    sampleDimPixel = 0,
    sampleDimLens  = 2,

    sampleDimBaseSize = 4
  };

  IntegratorBase(const Props& props)
  {
    maxDepth = props.get("maxDepth", 100);
    maxRadiance = props.get("maxRadiance", float(posMax)); // no clamping by default
  }

  void updateIntegrator(const Props& props)
  {
    maxDepth = props.get("maxDepth", maxDepth);
    Log() << "Updated maxDepth: " << maxDepth;

    maxRadiance = props.get("maxRadiance", maxRadiance);
    Log() << "Updated maxRadiance: " << maxRadiance;
  }
};

} // namespace prt
