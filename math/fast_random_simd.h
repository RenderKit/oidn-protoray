// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/common.h"
#include "simd.h"
#include "vec2.h"
#include "vec3.h"

namespace prt {

class FastRandomSimd
{
private:
  vint value;

public:
  prt_inline FastRandomSimd(uint32_t seed = 1)
  {
    reset(seed);
  }

  prt_inline void reset(uint32_t seed = 1)
  {
    int x = seed;
    for (int i = 0; i < simdSize; ++i)
    {
      x = (x * 8191) ^ 140167;
      value[i] = x;
    }
  }

  prt_inline void next()
  {
    const vint multiplier = 1664525;
    const vint increment = 1013904223;
    value = multiplier * value + increment;
  }

  prt_inline vint get1i()
  {
    next();
    return value;
  }

  prt_inline vfloat get1f()
  {
    next();
    return toFloatUnorm(value);
  }

  prt_inline Vec2vf get2f()
  {
    return Vec2vf(get1f(), get1f());
  }

  prt_inline Vec3vf get3f()
  {
    return Vec3vf(get1f(), get1f(), get1f());
  }
};

} // namespace prt
