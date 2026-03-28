// Copyright 2026 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <arm_neon.h>
#include "../simd_common.h"

namespace prt {

// vbool8 - double-pumped using two uint32x4_t
template <>
struct var<bool,8>
{
  union
  {
    struct { uint32x4_t lo, hi; };
    int v[8];
  };

  prt_inline var() {}

  prt_inline var(const vbool8& a) : lo(a.lo), hi(a.hi) {}
  prt_inline var(uint32x4_t a0, uint32x4_t a1) : lo(a0), hi(a1) {}

  prt_inline var(int a)
  {
    assert(a >= 0 && a <= 0xff);
    lo = vld1q_u32(simdMaskTable4[a & 0xf]);
    hi = vld1q_u32(simdMaskTable4[a >> 4]);
  }

  prt_inline var(Zero) : lo(vdupq_n_u32(0)), hi(vdupq_n_u32(0)) {}
  prt_inline var(One)  : lo(vdupq_n_u32(0xffffffff)), hi(vdupq_n_u32(0xffffffff)) {}

  prt_inline vbool8& operator =(const vbool8& a)
  {
    lo = a.lo;
    hi = a.hi;
    return *this;
  }

  prt_inline const int& operator [](size_t i) const
  {
    assert(i < 8);
    return v[i];
  }

  prt_inline int& operator [](size_t i)
  {
    assert(i < 8);
    return v[i];
  }
};

// Conversions
// -----------

prt_inline int toIntMask(const vbool8& a)
{
  static const int32x4_t shift = {0, 1, 2, 3};
  uint32x4_t bitsLo = vshrq_n_u32(a.lo, 31);
  uint32x4_t bitsHi = vshrq_n_u32(a.hi, 31);
  uint32x4_t shiftedLo = vshlq_u32(bitsLo, shift);
  uint32x4_t shiftedHi = vshlq_u32(bitsHi, vaddq_s32(shift, vdupq_n_s32(4)));
  return vaddvq_u32(shiftedLo) + vaddvq_u32(shiftedHi);
}

// Select function
// ---------------

prt_inline vbool8 select(const vbool8& mask, const vbool8& a, const vbool8& b)
{
  return vbool8(vbslq_u32(mask.lo, a.lo, b.lo), vbslq_u32(mask.hi, a.hi, b.hi));
}

// Bitwise operators
// -----------------

prt_inline vbool8 operator !(const vbool8& a)
{
  return vbool8(vmvnq_u32(a.lo), vmvnq_u32(a.hi));
}

prt_inline vbool8 operator &(const vbool8& a, const vbool8& b)
{
  return vbool8(vandq_u32(a.lo, b.lo), vandq_u32(a.hi, b.hi));
}

prt_inline vbool8 operator |(const vbool8& a, const vbool8& b)
{
  return vbool8(vorrq_u32(a.lo, b.lo), vorrq_u32(a.hi, b.hi));
}

prt_inline vbool8 operator ^(const vbool8& a, const vbool8& b)
{
  return vbool8(veorq_u32(a.lo, b.lo), veorq_u32(a.hi, b.hi));
}

prt_inline vbool8 andn(const vbool8& a, const vbool8& b)
{
  return vbool8(vbicq_u32(a.lo, b.lo), vbicq_u32(a.hi, b.hi));
}

// Assignment operators
// --------------------

prt_inline vbool8& operator &=(vbool8& a, const vbool8& b) { return a = a & b; }
prt_inline vbool8& operator |=(vbool8& a, const vbool8& b) { return a = a | b; }
prt_inline vbool8& operator ^=(vbool8& a, const vbool8& b) { return a = a ^ b; }

// Test functions
// --------------

prt_inline bool all(const vbool8& a)
{
  return vminvq_u32(a.lo) != 0 && vminvq_u32(a.hi) != 0;
}

prt_inline bool any(const vbool8& a)
{
  return vmaxvq_u32(a.lo) != 0 || vmaxvq_u32(a.hi) != 0;
}

prt_inline bool none(const vbool8& a)
{
  return vmaxvq_u32(a.lo) == 0 && vmaxvq_u32(a.hi) == 0;
}

// Get/set functions
// -----------------

prt_inline bool get(const vbool8& a, size_t i)
{
  return a[i];
}

prt_inline void set(vbool8& a, size_t i)
{
  a[i] = -1;
}

prt_inline void clear(vbool8& a, size_t i)
{
  a[i] = 0;
}

} // namespace prt
