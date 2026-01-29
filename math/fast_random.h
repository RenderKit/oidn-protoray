// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/common.h"
#include "vec2.h"
#include "vec3.h"

namespace prt {

class FastRandom
{
private:
  uint32_t value;

public:
  prt_inline FastRandom(uint32_t seed = 1) : value(seed) {}

  prt_inline void reset(uint32_t seed = 1)
  {
    value = (seed * 8191) ^ 140167;
  }

  prt_inline void next()
  {
    const uint32_t multiplier = 1664525;
    const uint32_t increment = 1013904223;
    value = multiplier * value + increment;
  }

  prt_inline uint32_t get1ui()
  {
    next();
    return value;
  }

  prt_inline int get1i()
  {
    next();
    return value;
  }

  prt_inline float get1f()
  {
    next();
    return toFloatUnorm(value);
  }

  prt_inline Vec2f get2f()
  {
    return Vec2f(get1f(), get1f());
  }

  prt_inline Vec3f get3f()
  {
    return Vec3f(get1f(), get1f(), get1f());
  }
};

} // namespace prt
