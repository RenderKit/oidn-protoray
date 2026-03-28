// Copyright 2026 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <arm_neon.h>
#include "../simd_common.h"

namespace prt {

// vbool4
template <>
struct var<bool,4>
{
  union
  {
    uint32x4_t m;
    int v[4];
  };

  prt_inline var() {}

  prt_inline var(const vbool4& a) : m(a.m) {}
  prt_inline var(uint32x4_t a) : m(a) {}

  prt_inline var(int a)
  {
    assert(a >= 0 && a <= 0xf);
    m = vld1q_u32(simdMaskTable4[a]);
  }

  prt_inline var(Zero) : m(vdupq_n_u32(0)) {}
  prt_inline var(One)  : m(vdupq_n_u32(0xffffffff)) {}

  prt_inline vbool4& operator =(const vbool4& a)
  {
    m = a.m;
    return *this;
  }

  prt_inline const int& operator [](size_t i) const
  {
    assert(i < 4);
    return v[i];
  }

  prt_inline int& operator [](size_t i)
  {
    assert(i < 4);
    return v[i];
  }
};

// Conversions
// -----------

prt_inline int toIntMask(const vbool4& a)
{
  // Extract sign bits to form a 4-bit mask
  static const int32x4_t shift = {0, 1, 2, 3};
  uint32x4_t bits = vshrq_n_u32(a.m, 31);
  uint32x4_t shifted = vshlq_u32(bits, shift);
  return vaddvq_u32(shifted);
}

// Select function
// ---------------

prt_inline vbool4 select(const vbool4& mask, const vbool4& a, const vbool4& b)
{
  return vbslq_u32(mask.m, a.m, b.m);
}

// Bitwise operators
// -----------------

prt_inline vbool4 operator !(const vbool4& a)
{
  return vmvnq_u32(a.m);
}

prt_inline vbool4 operator &(const vbool4& a, const vbool4& b)
{
  return vandq_u32(a.m, b.m);
}

prt_inline vbool4 operator |(const vbool4& a, const vbool4& b)
{
  return vorrq_u32(a.m, b.m);
}

prt_inline vbool4 operator ^(const vbool4& a, const vbool4& b)
{
  return veorq_u32(a.m, b.m);
}

prt_inline vbool4 andn(const vbool4& a, const vbool4& b)
{
  return vbicq_u32(a.m, b.m); // a & ~b
}

// Assignment operators
// --------------------

prt_inline vbool4& operator &=(vbool4& a, const vbool4& b) { return a = a & b; }
prt_inline vbool4& operator |=(vbool4& a, const vbool4& b) { return a = a | b; }
prt_inline vbool4& operator ^=(vbool4& a, const vbool4& b) { return a = a ^ b; }

// Test functions
// --------------

prt_inline bool all(const vbool4& a)
{
  return vminvq_u32(a.m) != 0;
}

prt_inline bool any(const vbool4& a)
{
  return vmaxvq_u32(a.m) != 0;
}

prt_inline bool none(const vbool4& a)
{
  return vmaxvq_u32(a.m) == 0;
}

// Get/set functions
// -----------------

prt_inline bool get(const vbool4& a, size_t i)
{
  return a[i];
}

prt_inline void set(vbool4& a, size_t i)
{
  a[i] = -1;
}

prt_inline void clear(vbool4& a, size_t i)
{
  a[i] = 0;
}

} // namespace prt
