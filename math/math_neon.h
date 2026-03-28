// Copyright 2026 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <arm_neon.h>
#include "math/math_common.h"

namespace prt {

prt_inline float rcp(float x)
{
  float32x2_t v = vdup_n_f32(x);
  float32x2_t r = vrecpe_f32(v);
  r = vmul_f32(vrecps_f32(v, r), r);
  r = vmul_f32(vrecps_f32(v, r), r);
  return vget_lane_f32(r, 0);
}

prt_inline float rsqrt(float x)
{
  float32x2_t v = vdup_n_f32(x);
  float32x2_t r = vrsqrte_f32(v);
  r = vmul_f32(vrsqrts_f32(vmul_f32(v, r), r), r);
  r = vmul_f32(vrsqrts_f32(vmul_f32(v, r), r), r);
  return vget_lane_f32(r, 0);
}

prt_inline float floor(float x)
{
  return __builtin_floorf(x);
}

prt_inline float ceil(float x)
{
  return __builtin_ceilf(x);
}

prt_inline float round(float x)
{
  return __builtin_roundevenf(x);
}

prt_inline unsigned int bitCount(uint32_t x)
{
  return __builtin_popcount(x);
}

// If the value is 0, the result is undefined!
prt_inline unsigned int bitScan(uint32_t x)
{
  return __builtin_ctz(x);
}

// Starts scanning from prevPos+1
// If not found, the result is 32
prt_inline unsigned int bitScan(uint32_t x, unsigned int prevPos)
{
  unsigned int startPos = prevPos + 1;
  uint32_t xt = x >> startPos;
  if (xt == 0) return 32;
  return bitScan(xt) + startPos;
}

prt_inline void shiftRight128(uint64_t& low, uint64_t& high, int count)
{
  low = (low >> count) | (high << (64 - count));
  high >>= count;
}

prt_inline void shiftLeft128(uint64_t& low, uint64_t& high, int count)
{
  high = (high << count) | (low >> (64 - count));
  low <<= count;
}

} // namespace prt
