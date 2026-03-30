// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <immintrin.h>
#include "../simd_common.h"
#include "vbool4_avx.h"
#include "vint4_avx.h"

namespace prt {

// vuint4
template <>
struct var<uint,4>
{
  union
  {
    __m128i m;
    uint v[4];
  };

  prt_inline var() {}

  prt_inline var(const vuint4& a) : m(a.m) {}
  prt_inline var(const vint4& a) : m(a.m) {}
  prt_inline var(__m128i a) : m(a) {}
  prt_inline var(uint a) : m(_mm_set1_epi32(a)) {}
  prt_inline var(uint a0, uint a1, uint a2, uint a3) : m(_mm_set_epi32(a3, a2, a1, a0)) {}

  prt_inline var(Zero) : m(_mm_setzero_si128()) {}
  prt_inline var(One)  : m(_mm_set1_epi32(1)) {}
  prt_inline var(Step) : m(_mm_set_epi32(3, 2, 1, 0)) {}

  prt_inline vuint4& operator =(const vuint4& a)
  {
    m = a.m;
    return *this;
  }

  prt_inline const uint& operator [](size_t i) const
  {
    assert(i < 4);
    return v[i];
  }

  prt_inline uint& operator [](size_t i)
  {
    assert(i < 4);
    return v[i];
  }
};

prt_inline vint4::var(const vuint4& a) : m(a.m) {}

// Load functions
// --------------

template <>
prt_inline vuint4 load(const uint* ptr)
{
  return _mm_load_si128((const __m128i*)ptr);
}

template <>
prt_inline vuint4 load(const vbool4& mask, const uint* ptr)
{
  vuint4 r;
  for (int i = 0; i < 4; ++i)
    r[i] = mask[i] ? ptr[i] : 0;
  return r;
}

template <>
prt_inline vuint4 loadu(const uint* ptr)
{
  return _mm_loadu_si128((const __m128i*)ptr);
}

prt_inline vuint4 gather(const uint* ptr, const vint4& idx)
{
  vuint4 r;
  for (int i = 0; i < 4; ++i)
    r[i] = ptr[idx[i]];
  return r;
}

prt_inline vuint4 gather(const vbool4& mask, const uint* ptr, const vint4& idx)
{
  vuint4 r;
  for (int i = 0; i < 4; ++i)
    r[i] = mask[i] ? ptr[idx[i]] : 0;
  return r;
}

// Store functions
// ---------------

prt_inline void store(uint* ptr, const vuint4& a)
{
  _mm_store_si128((__m128i*)ptr, a.m);
}

prt_inline void store(const vbool4& mask, uint* ptr, const vuint4& a)
{
  for (int i = 0; i < 4; ++i)
    if (mask[i]) ptr[i] = a[i];
}

prt_inline void compress(const vbool4& mask, uint* ptr, const vuint4& a)
{
  uint offset = 0;
  for (int i = 0; i < 4; ++i)
    if (mask[i]) ptr[offset++] = a[i];
}

prt_inline void scatter(uint* ptr, const vint4& idx, const vuint4& a)
{
  for (int i = 0; i < 4; ++i)
    ptr[idx[i]] = a[i];
}

prt_inline void scatter(const vbool4& mask, uint* ptr, const vint4& idx, const vuint4& a)
{
  for (int i = 0; i < 4; ++i)
    if (mask[i]) ptr[idx[i]] = a[i];
}

// Select function
// ---------------

prt_inline vuint4 select(const vbool4& mask, const vuint4& a, const vuint4& b)
{
  return _mm_blendv_epi8(b.m, a.m, _mm_castps_si128(mask.m));
}

// Shuffle functions
// -----------------

template <int i>
prt_inline uint extract(const vuint4& a)
{
  return _mm_extract_epi32(a.m, i);
}

template <int i>
prt_inline vuint4 broadcast(const vuint4& a)
{
  return _mm_shuffle_epi32(a.m, _MM_SHUFFLE(i, i, i, i));
}

template <int i0, int i1, int i2, int i3>
prt_inline vuint4 permute4(const vuint4& a)
{
  return _mm_shuffle_epi32(a.m, _MM_SHUFFLE(i3, i2, i1, i0));
}

// Arithmetic operators
// --------------------

prt_inline vuint4 operator +(const vuint4& a, const vuint4& b)
{
  return _mm_add_epi32(a.m, b.m);
}

prt_inline vuint4 operator -(const vuint4& a, const vuint4& b)
{
  return _mm_sub_epi32(a.m, b.m);
}

prt_inline vuint4 operator *(const vuint4& a, const vuint4& b)
{
  return _mm_mullo_epi32(a.m, b.m);
}

// Bitwise operators
// -----------------

prt_inline vuint4 operator &(const vuint4& a, const vuint4& b)
{
  return _mm_and_si128(a.m, b.m);
}

prt_inline vuint4 operator |(const vuint4& a, const vuint4& b)
{
  return _mm_or_si128(a.m, b.m);
}

prt_inline vuint4 operator ^(const vuint4& a, const vuint4& b)
{
  return _mm_xor_si128(a.m, b.m);
}

prt_inline vuint4 andn(const vuint4& a, const vuint4& b)
{
  return _mm_andnot_si128(b.m, a.m);
}

prt_inline vuint4 operator <<(const vuint4& a, int b)
{
  return _mm_slli_epi32(a.m, b);
}

prt_inline vuint4 operator >>(const vuint4& a, int b)
{
  return _mm_srli_epi32(a.m, b);
}

prt_inline vuint4 shl(const vuint4& a, int b)
{
  return _mm_slli_epi32(a.m, b);
}

prt_inline vuint4 shr(const vuint4& a, int b)
{
  return _mm_srli_epi32(a.m, b);
}

// Assignment operators
// --------------------

prt_inline vuint4& operator +=(vuint4& a, const vuint4& b) { return a = a + b; }
prt_inline vuint4& operator -=(vuint4& a, const vuint4& b) { return a = a - b; }
prt_inline vuint4& operator *=(vuint4& a, const vuint4& b) { return a = a * b; }
prt_inline vuint4& operator &=(vuint4& a, const vuint4& b) { return a = a & b; }
prt_inline vuint4& operator |=(vuint4& a, const vuint4& b) { return a = a | b; }
prt_inline vuint4& operator ^=(vuint4& a, const vuint4& b) { return a = a ^ b; }

prt_inline vuint4& operator <<=(vuint4& a, int b) { return a = a << b; }
prt_inline vuint4& operator >>=(vuint4& a, int b) { return a = a >> b; }

// Compare operators
// -----------------

prt_inline vbool4 operator ==(const vuint4& a, const vuint4& b)
{
  return _mm_cmpeq_epi32(a.m, b.m);
}

/*
prt_inline vbool4 operator <(const vuint4& a, const vuint4& b)
{
  return _mm_cmplt_epi32(a.m, b.m);
}

prt_inline vbool4 operator >(const vuint4& a, const vuint4& b)
{
  return _mm_cmpgt_epi32(a.m, b.m);
}
*/

prt_inline vbool4 operator !=(const vuint4& a, const vuint4& b)
{
  return !(a == b);
}

/*
prt_inline vbool4 operator <=(const vuint4& a, const vuint4& b)
{
  return !(a > b);
}

prt_inline vbool4 operator >=(const vuint4& a, const vuint4& b)
{
  return !(a < b);
}
*/

// Compare functions
// -----------------

prt_inline vbool4 cmpEq(const vuint4& a, const vuint4& b) { return a == b; }
prt_inline vbool4 cmpNe(const vuint4& a, const vuint4& b) { return a != b; }
/*
prt_inline vbool4 cmpLt(const vuint4& a, const vuint4& b) { return a <  b; }
prt_inline vbool4 cmpLe(const vuint4& a, const vuint4& b) { return a <= b; }
prt_inline vbool4 cmpGt(const vuint4& a, const vuint4& b) { return a >  b; }
prt_inline vbool4 cmpGe(const vuint4& a, const vuint4& b) { return a >= b; }
*/

prt_inline vbool4 cmpEq(const vbool4& mask, const vuint4& a, const vuint4& b) { return (a == b) & mask; }
prt_inline vbool4 cmpNe(const vbool4& mask, const vuint4& a, const vuint4& b) { return (a != b) & mask; }
/*
prt_inline vbool4 cmpLt(const vbool4& mask, const vuint4& a, const vuint4& b) { return (a <  b) & mask; }
prt_inline vbool4 cmpLe(const vbool4& mask, const vuint4& a, const vuint4& b) { return (a <= b) & mask; }
prt_inline vbool4 cmpGt(const vbool4& mask, const vuint4& a, const vuint4& b) { return (a >  b) & mask; }
prt_inline vbool4 cmpGe(const vbool4& mask, const vuint4& a, const vuint4& b) { return (a >= b) & mask; }
*/

// Math functions
// --------------

prt_inline vuint4 min(const vuint4& a, const vuint4& b)
{
  return _mm_min_epu32(a.m, b.m);
}

prt_inline vuint4 max(const vuint4& a, const vuint4& b)
{
  return _mm_max_epu32(a.m, b.m);
}

} // namespace prt
