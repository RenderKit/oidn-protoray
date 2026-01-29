// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <immintrin.h>
#include "../simd_common.h"
#include "vbool16_avx512.h"
#include "vint16_avx512.h"

namespace prt {

// vuint16
template <>
struct var<uint,16>
{
  union
  {
    __m512i m;
    uint v[16];
  };

  prt_inline var() {}

  prt_inline var(const vuint16& a) : m(a.m) {}
  prt_inline var(const vint16& a) : m(a.m) {}
  prt_inline var(__m512i a) : m(a) {}
  prt_inline var(uint x) : m(_mm512_set1_epi32(x)) {}
  prt_inline var(uint x0, uint x1, uint x2, uint x3) : m(_mm512_set4_epi32(x3, x2, x1, x0)) {}

  prt_inline var(uint x0, uint x1, uint x2,  uint x3,  uint x4,  uint x5,  uint x6,  uint x7,
                 uint x8, uint x9, uint x10, uint x11, uint x12, uint x13, uint x14, uint x15)
    : m(_mm512_set_epi32(x15, x14, x13, x12, x11, x10, x9, x8, x7, x6, x5, x4, x3, x2, x1, x0)) {}

  prt_inline var(Zero) : m(_mm512_setzero_epi32()) {}
  prt_inline var(One)  : m(_mm512_set1_epi32(1)) {}
  prt_inline var(Step) : m(_mm512_set_epi32(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)) {}

  prt_inline vuint16& operator =(const vuint16& a)
  {
    m = a.m;
    return *this;
  }

  prt_inline const uint& operator [](size_t i) const
  {
    assert(i < 16);
    return v[i];
  }

  prt_inline uint& operator [](size_t i)
  {
    assert(i < 16);
    return v[i];
  }
};

prt_inline vint16::var(const vuint16& a) : m(a.m) {}

// Load functions
// --------------

template <>
prt_inline vuint16 load(const uint* ptr)
{
  return _mm512_load_epi32(ptr);
}

template <>
prt_inline vuint16 load(const vbool16& mask, const uint* ptr)
{
  return _mm512_maskz_load_epi32(mask.m, ptr);
}

template <>
prt_inline vuint16 loadu(const uint* ptr)
{
  return _mm512_loadu_si512(ptr);
}

template <>
prt_inline vuint16 loadu(const vbool16& mask, const uint* ptr)
{
  return _mm512_maskz_loadu_epi32(mask.m, ptr);
}

prt_inline vuint16 expand(const vbool16& mask, const uint* ptr)
{
  return _mm512_maskz_expandloadu_epi32(mask.m, ptr);
}

prt_inline vuint16 gather(const uint* ptr, const vint16& idx)
{
  return _mm512_i32gather_epi32(idx.m, ptr, 4);
}

prt_inline vuint16 gather(const vbool16& mask, const uint* ptr, const vint16& idx)
{
  return _mm512_mask_i32gather_epi32(_mm512_setzero_epi32(), mask.m, idx.m, ptr, 4);
}

// Store functions
// ---------------

prt_inline void store(uint* ptr, const vuint16& a)
{
  _mm512_store_epi32(ptr, a.m);
}

prt_inline void store(const vbool16& mask, uint* ptr, const vuint16& a)
{
  _mm512_mask_store_epi32(ptr, mask.m, a.m);
}

prt_inline void storeu(uint* ptr, const vuint16& a)
{
  _mm512_storeu_si512(ptr, a.m);
}

prt_inline void storeu(const vbool16& mask, uint* ptr, const vuint16& a)
{
  _mm512_mask_storeu_epi32(ptr, mask.m, a.m);
}

prt_inline void compress(const vbool16& mask, uint* ptr, const vuint16& a)
{
  _mm512_mask_compressstoreu_epi32(ptr, mask.m, a.m);
}

prt_inline void scatter(uint* ptr, const vuint16& idx, const vint16& a)
{
  _mm512_i32scatter_epi32(ptr, idx.m, a.m, 4);
}

prt_inline void scatter(const vbool16& mask, uint* ptr, const vint16& idx, const vuint16& a)
{
  _mm512_mask_i32scatter_epi32(ptr, mask.m, idx.m, a.m, 4);
}

// Select function
// ---------------

prt_inline vuint16 select(const vbool16& mask, const vuint16& a, const vuint16& b)
{
  return _mm512_mask_mov_epi32(b.m, mask.m, a.m);
}

// Arithmetic operators
// --------------------

prt_inline vuint16 operator +(const vuint16& a, const vuint16& b)
{
  return _mm512_add_epi32(a.m, b.m);
}

prt_inline vuint16 operator -(const vuint16& a, const vuint16& b)
{
  return _mm512_sub_epi32(a.m, b.m);
}

prt_inline vuint16 operator *(const vuint16& a, const vuint16& b)
{
  return _mm512_mullo_epi32(a.m, b.m);
}

// Bitwise operators
// -----------------

prt_inline vuint16 operator &(const vuint16& a, const vuint16& b)
{
  return _mm512_and_epi32(a.m, b.m);
}

prt_inline vuint16 operator |(const vuint16& a, const vuint16& b)
{
  return _mm512_or_epi32(a.m, b.m);
}

prt_inline vuint16 operator ^(const vuint16& a, const vuint16& b)
{
  return _mm512_xor_epi32(a.m, b.m);
}

prt_inline vuint16 operator <<(const vuint16& a, int b)
{
  return _mm512_slli_epi32(a.m, b);
}

prt_inline vuint16 operator >>(const vuint16& a, int b)
{
  return _mm512_srli_epi32(a.m, b);
}

prt_inline vuint16 shl(const vuint16& a, int b)
{
  return _mm512_slli_epi32(a.m, b);
}

prt_inline vuint16 shr(const vuint16& a, int b)
{
  return _mm512_srli_epi32(a.m, b);
}

// Assignment operators
// --------------------

prt_inline vuint16& operator +=(vuint16& a, const vuint16& b) { return a = a + b; }
prt_inline vuint16& operator -=(vuint16& a, const vuint16& b) { return a = a - b; }
prt_inline vuint16& operator *=(vuint16& a, const vuint16& b) { return a = a * b; }
prt_inline vuint16& operator &=(vuint16& a, const vuint16& b) { return a = a & b; }
prt_inline vuint16& operator |=(vuint16& a, const vuint16& b) { return a = a | b; }
prt_inline vuint16& operator ^=(vuint16& a, const vuint16& b) { return a = a ^ b; }

prt_inline vuint16& operator <<=(vuint16& a, uint b) { return a = a << b; }
prt_inline vuint16& operator >>=(vuint16& a, uint b) { return a = a >> b; }

// Compare operators
// -----------------

prt_inline vbool16 operator ==(const vuint16& a, const vuint16& b)
{
  return _mm512_cmp_epi32_mask(a.m, b.m, _MM_CMPINT_EQ);
}

/*
prt_inline vbool16 operator <(const vuint16& a, const vuint16& b)
{
  return _mm512_cmp_epi32_mask(a.m, b.m, _MM_CMPINT_LT);
}

prt_inline vbool16 operator >(const vuint16& a, const vuint16& b)
{
  return _mm512_cmp_epi32_mask(a.m, b.m, _MM_CMPINT_GT);
}
*/

prt_inline vbool16 operator !=(const vuint16& a, const vuint16& b)
{
  return _mm512_cmp_epi32_mask(a.m, b.m, _MM_CMPINT_NE);
}

/*
prt_inline vbool16 operator <=(const vuint16& a, const vuint16& b)
{
  return _mm512_cmp_epi32_mask(a.m, b.m, _MM_CMPINT_LE);
}

prt_inline vbool16 operator >=(const vuint16& a, const vuint16& b)
{
  return _mm512_cmp_epi32_mask(a.m, b.m, _MM_CMPINT_GE);
}
*/

// Math functions
// --------------

prt_inline vuint16 min(const vuint16& a, const vuint16& b)
{
  return _mm512_min_epu32(a.m, b.m);
}

prt_inline vuint16 max(const vuint16& a, const vuint16& b)
{
  return _mm512_max_epu32(a.m, b.m);
}

} // namespace prt
