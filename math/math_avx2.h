// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <immintrin.h>
#include "math/math_common.h"

namespace prt {

prt_inline float rcp(float x)
{
  __m128 r = _mm_rcp_ss(_mm_set_ss(x));
  return _mm_cvtss_f32(_mm_sub_ss(_mm_add_ss(r, r), _mm_mul_ss(_mm_mul_ss(r, r), _mm_set_ss(x))));
}

prt_inline float rsqrt(float x)
{
  __m128 r = _mm_rsqrt_ss(_mm_set_ss(x));
  return _mm_cvtss_f32(_mm_add_ss(_mm_mul_ss(_mm_set_ss(1.5f), r), _mm_mul_ss(_mm_mul_ss(_mm_mul_ss(_mm_set_ss(x), _mm_set_ss(-0.5f)), r), _mm_mul_ss(r, r))));
}

prt_inline float floor(float x)
{
  return _mm_cvtss_f32(_mm_floor_ss(_mm_set_ss(x), _mm_set_ss(x)));
}

prt_inline float ceil(float x)
{
  return _mm_cvtss_f32(_mm_ceil_ss(_mm_set_ss(x), _mm_set_ss(x)));
}

prt_inline float round(float x)
{
  return _mm_cvtss_f32(_mm_round_ss(_mm_set_ss(x), _mm_set_ss(x), _MM_FROUND_TO_NEAREST_INT));
}

prt_inline unsigned int bitCount(uint32_t x)
{
  return _mm_popcnt_u32(x);
}

// If the value is 0, the result is undefined!
prt_inline unsigned int bitScan(uint32_t x)
{
  return _tzcnt_u32(x);
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

#ifdef NDEBUG // FIXME: GCC workaround for "impossible constraint in asm"
prt_inline void shiftRight128(uint64_t& low, uint64_t& high, int count)
{
  asm("shrd %2,%1,%0" : "=r"(low) : "r"(high), "J"(count), "0"(low) : "flags");
  high >>= count;
}

prt_inline void shiftLeft128(uint64_t& low, uint64_t& high, int count)
{
  asm("shld %2,%1,%0" : "=r"(high) : "r"(low), "J"(count), "0"(high) : "flags");
  low <<= count;
}
#else
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
#endif

} // namespace prt
