// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "math/vec3.h"

namespace prt {

template <class T>
prt_inline T roughnessToAlpha(T roughness)
{
  // Roughness is squared for perceptual reasons
  return max(sqr(roughness), 0.001f);
}

// [Burley, 2012, "Physically Based Shading at Disney", Course Notes, v3]
template <class T>
prt_inline Vec2<T> roughnessToAlpha(T roughness, T anisotropy)
{
  T aspect = sqrt(1.f - 0.9f * anisotropy);
  return Vec2<T>(max(sqr(roughness) / aspect, 0.001f),
                 max(sqr(roughness) * aspect, 0.001f));
}

} // namespace prt
