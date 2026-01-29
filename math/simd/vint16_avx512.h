// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <immintrin.h>
#include "../simd_common.h"
#include "vbool16_avx512.h"

namespace prt {

// vint16
template <>
struct var<int,16>
{
  union
  {
    __m512i m;
    int v[16];
  };

  prt_inline var() {}

  prt_inline var(const vint16& a) : m(a.m) {}
  prt_inline var(const vuint16& a);
  prt_inline var(__m512i a) : m(a) {}
  prt_inline var(int x) : m(_mm512_set1_epi32(x)) {}
  prt_inline var(int x0, int x1, int x2, int x3) : m(_mm512_set4_epi32(x3, x2, x1, x0)) {}

  prt_inline var(int x0, int x1, int x2,  int x3,  int x4,  int x5,  int x6,  int x7,
                 int x8, int x9, int x10, int x11, int x12, int x13, int x14, int x15)
    : m(_mm512_set_epi32(x15, x14, x13, x12, x11, x10, x9, x8, x7, x6, x5, x4, x3, x2, x1, x0)) {}

  prt_inline var(Zero) : m(_mm512_setzero_epi32()) {}
  prt_inline var(One)  : m(_mm512_set1_epi32(1)) {}
  prt_inline var(Step) : m(_mm512_set_epi32(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)) {}

  prt_inline vint16& operator =(const vint16& a)
  {
    m = a.m;
    return *this;
  }

  prt_inline const int& operator [](size_t i) const
  {
    assert(i < 16);
    return v[i];
  }

  prt_inline int& operator [](size_t i)
  {
    assert(i < 16);
    return v[i];
  }
};

// Load functions
// --------------

template <>
prt_inline vint16 load(const int* ptr)
{
  return _mm512_load_epi32(ptr);
}

template <>
prt_inline vint16 load(const vbool16& mask, const int* ptr)
{
  return _mm512_maskz_load_epi32(mask.m, ptr);
}

template <>
prt_inline vint16 loadu(const int* ptr)
{
  return _mm512_loadu_si512(ptr);
}

template <>
prt_inline vint16 loadu(const vbool16& mask, const int* ptr)
{
  return _mm512_maskz_loadu_epi32(mask.m, ptr);
}

prt_inline vint16 expand(const vbool16& mask, const int* ptr)
{
  return _mm512_maskz_expandloadu_epi32(mask.m, ptr);
}

prt_inline vint16 gather(const int* ptr, const vint16& idx)
{
  return _mm512_i32gather_epi32(idx.m, ptr, 4);
}

prt_inline vint16 gather(const vbool16& mask, const int* ptr, const vint16& idx)
{
  return _mm512_mask_i32gather_epi32(_mm512_setzero_epi32(), mask.m, idx.m, ptr, 4);
}

// Store functions
// ---------------

prt_inline void store(int* ptr, const vint16& a)
{
  _mm512_store_epi32(ptr, a.m);
}

prt_inline void store(const vbool16& mask, int* ptr, const vint16& a)
{
  _mm512_mask_store_epi32(ptr, mask.m, a.m);
}

prt_inline void storeu(int* ptr, const vint16& a)
{
  _mm512_storeu_si512(ptr, a.m);
}

prt_inline void storeu(const vbool16& mask, int* ptr, const vint16& a)
{
  _mm512_mask_storeu_epi32(ptr, mask.m, a.m);
}

prt_inline void compress(const vbool16& mask, int* ptr, const vint16& a)
{
  _mm512_mask_compressstoreu_epi32(ptr, mask.m, a.m);
}

prt_inline void scatter(int* ptr, const vint16& idx, const vint16& a)
{
  _mm512_i32scatter_epi32(ptr, idx.m, a.m, 4);
}

prt_inline void scatter(const vbool16& mask, int* ptr, const vint16& idx, const vint16& a)
{
  _mm512_mask_i32scatter_epi32(ptr, mask.m, idx.m, a.m, 4);
}

// Select function
// ---------------

prt_inline vint16 select(const vbool16& mask, const vint16& a, const vint16& b)
{
  return _mm512_mask_mov_epi32(b.m, mask.m, a.m);
}

// Arithmetic operators
// --------------------

prt_inline vint16 operator +(const vint16& a, const vint16& b)
{
  return _mm512_add_epi32(a.m, b.m);
}

prt_inline vint16 operator -(const vint16& a, const vint16& b)
{
  return _mm512_sub_epi32(a.m, b.m);
}

prt_inline vint16 operator *(const vint16& a, const vint16& b)
{
  return _mm512_mullo_epi32(a.m, b.m);
}

// Bitwise operators
// -----------------

prt_inline vint16 operator &(const vint16& a, const vint16& b)
{
  return _mm512_and_epi32(a.m, b.m);
}

prt_inline vint16 operator |(const vint16& a, const vint16& b)
{
  return _mm512_or_epi32(a.m, b.m);
}

prt_inline vint16 operator ^(const vint16& a, const vint16& b)
{
  return _mm512_xor_epi32(a.m, b.m);
}

prt_inline vint16 operator <<(const vint16& a, int b)
{
  return _mm512_slli_epi32(a.m, b);
}

prt_inline vint16 operator >>(const vint16& a, int b)
{
  return _mm512_srai_epi32(a.m, b);
}

prt_inline vint16 shl(const vint16& a, int b)
{
  return _mm512_slli_epi32(a.m, b);
}

prt_inline vint16 shr(const vint16& a, int b)
{
  return _mm512_srli_epi32(a.m, b);
}

// Assignment operators
// --------------------

prt_inline vint16& operator +=(vint16& a, const vint16& b) { return a = a + b; }
prt_inline vint16& operator -=(vint16& a, const vint16& b) { return a = a - b; }
prt_inline vint16& operator *=(vint16& a, const vint16& b) { return a = a * b; }
prt_inline vint16& operator &=(vint16& a, const vint16& b) { return a = a & b; }
prt_inline vint16& operator |=(vint16& a, const vint16& b) { return a = a | b; }
prt_inline vint16& operator ^=(vint16& a, const vint16& b) { return a = a ^ b; }

prt_inline vint16& operator <<=(vint16& a, int b) { return a = a << b; }
prt_inline vint16& operator >>=(vint16& a, int b) { return a = a >> b; }

// Compare operators
// -----------------

prt_inline vbool16 operator ==(const vint16& a, const vint16& b)
{
  return _mm512_cmp_epi32_mask(a.m, b.m, _MM_CMPINT_EQ);
}

prt_inline vbool16 operator <(const vint16& a, const vint16& b)
{
  return _mm512_cmp_epi32_mask(a.m, b.m, _MM_CMPINT_LT);
}

prt_inline vbool16 operator >(const vint16& a, const vint16& b)
{
  return _mm512_cmp_epi32_mask(a.m, b.m, _MM_CMPINT_GT);
}

prt_inline vbool16 operator !=(const vint16& a, const vint16& b)
{
  return _mm512_cmp_epi32_mask(a.m, b.m, _MM_CMPINT_NE);
}

prt_inline vbool16 operator <=(const vint16& a, const vint16& b)
{
  return _mm512_cmp_epi32_mask(a.m, b.m, _MM_CMPINT_LE);
}

prt_inline vbool16 operator >=(const vint16& a, const vint16& b)
{
  return _mm512_cmp_epi32_mask(a.m, b.m, _MM_CMPINT_GE);
}

// Math functions
// --------------

prt_inline vint16 min(const vint16& a, const vint16& b)
{
  return _mm512_min_epi32(a.m, b.m);
}

prt_inline vint16 max(const vint16& a, const vint16& b)
{
  return _mm512_max_epi32(a.m, b.m);
}

} // namespace prt
