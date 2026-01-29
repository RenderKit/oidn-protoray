// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <immintrin.h>
#include "../simd_common.h"

namespace prt {

// vbool8
template <>
struct var<bool,8>
{
  union
  {
    __m256 m;
    int v[8];
  };

  prt_inline var() {}

  prt_inline var(const vbool8& a) : m(a.m) {}
  prt_inline var(__m256 a) : m(a) {}
  prt_inline var(__m256i a) : m(_mm256_castsi256_ps(a)) {}

  prt_inline var(__m128i a0, __m128i a1) : m(_mm256_castsi256_ps(_mm256_insertf128_si256(_mm256_castsi128_si256(a0), a1, 1))) {}

  prt_inline var(int a)
  {
    assert(a >= 0 && a <= 0xff);

    __m128 lo = *(const __m128*)&simdMaskTable4[a & 0xf];
    __m128 hi = *(const __m128*)&simdMaskTable4[a >> 4];

    m = _mm256_insertf128_ps(_mm256_castps128_ps256(lo), hi, 1);
  }

  prt_inline var(Zero) : m(_mm256_setzero_ps()) {}
  prt_inline var(One)  : m(_mm256_castsi256_ps(_mm256_set1_epi32(0xffffffff))) {}

  prt_inline vbool8& operator =(const vbool8& a)
  {
    m = a.m;
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
  return _mm256_movemask_ps(a.m);
}

// Select function
// ---------------

prt_inline vbool8 select(const vbool8& mask, const vbool8& a, const vbool8& b)
{
  return _mm256_blendv_ps(b.m, a.m, mask.m);
}

// Bitwise operators
// -----------------

prt_inline vbool8 operator !(const vbool8& a)
{
  return _mm256_xor_ps(a.m, _mm256_castsi256_ps(_mm256_set1_epi32(0xffffffff)));
}

prt_inline vbool8 operator &(const vbool8& a, const vbool8& b)
{
  return _mm256_and_ps(a.m, b.m);
}

prt_inline vbool8 operator |(const vbool8& a, const vbool8& b)
{
  return _mm256_or_ps(a.m, b.m);
}

prt_inline vbool8 operator ^(const vbool8& a, const vbool8& b)
{
  return _mm256_xor_ps(a.m, b.m);
}

prt_inline vbool8 andn(const vbool8& a, const vbool8& b)
{
  return _mm256_andnot_ps(b.m, a.m);
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
  return _mm256_movemask_ps(a.m) == 0xff;
}

prt_inline bool any(const vbool8& a)
{
  return _mm256_movemask_ps(a.m) != 0;
}

prt_inline bool none(const vbool8& a)
{
  return _mm256_movemask_ps(a.m) == 0;
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
