// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <immintrin.h>
#include "../simd_common.h"

namespace prt {

// vbool4
template <>
struct var<bool,4>
{
  union
  {
    __m128 m;
    int v[4];
  };

  prt_inline var() {}

  prt_inline var(const vbool4& a) : m(a.m) {}
  prt_inline var(__m128 a) : m(a) {}
  prt_inline var(__m128i a) : m(_mm_castsi128_ps(a)) {}

  prt_inline var(int a)
  {
    assert(a >= 0 && a <= 0xf);
    m = *(const __m128*)&simdMaskTable4[a];
  }

  prt_inline var(Zero) : m(_mm_setzero_ps()) {}
  prt_inline var(One)  : m(_mm_castsi128_ps(_mm_set1_epi32(0xffffffff))) {}

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
  return _mm_movemask_ps(a.m);
}

// Select function
// ---------------

prt_inline vbool4 select(const vbool4& mask, const vbool4& a, const vbool4& b)
{
  return _mm_blendv_ps(b.m, a.m, mask.m);
}

// Bitwise operators
// -----------------

prt_inline vbool4 operator !(const vbool4& a)
{
  return _mm_xor_ps(a.m, _mm_castsi128_ps(_mm_set1_epi32(0xffffffff)));
}

prt_inline vbool4 operator &(const vbool4& a, const vbool4& b)
{
  return _mm_and_ps(a.m, b.m);
}

prt_inline vbool4 operator |(const vbool4& a, const vbool4& b)
{
  return _mm_or_ps(a.m, b.m);
}

prt_inline vbool4 operator ^(const vbool4& a, const vbool4& b)
{
  return _mm_xor_ps(a.m, b.m);
}

prt_inline vbool4 andn(const vbool4& a, const vbool4& b)
{
  return _mm_andnot_ps(b.m, a.m);
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
  return _mm_movemask_ps(a.m) == 0xf;
}

prt_inline bool any(const vbool4& a)
{
  return _mm_movemask_ps(a.m) != 0;
}

prt_inline bool none(const vbool4& a)
{
  return _mm_movemask_ps(a.m) == 0;
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
