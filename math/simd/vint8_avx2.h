// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <immintrin.h>
#include "../simd_common.h"
#include "vbool8_avx.h"

namespace prt {

// vint8
template <>
struct var<int,8>
{
  union
  {
    __m256i m;
    int v[8];
  };

  prt_inline var() {}

  prt_inline var(const vint8& a) : m(a.m) {}
  prt_inline var(const vuint8& a);
  prt_inline var(__m256i a) : m(a) {}
  prt_inline var(int a) : m(_mm256_set1_epi32(a)) {}
  prt_inline var(int a0, int a1, int a2, int a3, int a4, int a5, int a6, int a7) : m(_mm256_set_epi32(a7, a6, a5, a4, a3, a2, a1, a0)) {}

  prt_inline var(Zero) : m(_mm256_setzero_si256()) {}
  prt_inline var(One)  : m(_mm256_set1_epi32(1)) {}
  prt_inline var(Step) : m(_mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0)) {}

  prt_inline vint8& operator =(const vint8& a)
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

// Load functions
// --------------

template <>
prt_inline vint8 load(const int* ptr)
{
  return _mm256_load_si256((const __m256i*)ptr);
}

template <>
prt_inline vint8 load(const vbool8& mask, const int* ptr)
{
  return _mm256_maskload_epi32(ptr, _mm256_castps_si256(mask.m));
}

template <>
prt_inline vint8 loadu(const int* ptr)
{
  return _mm256_loadu_si256((const __m256i*)ptr);
}

template <>
prt_inline vint8 loadu(const vbool8& mask, const int* ptr)
{
  return _mm256_maskload_epi32(ptr, _mm256_castps_si256(mask.m));
}

prt_inline vint8 expand(const vbool8& mask, const int* ptr)
{
  __m256i key = _mm256_load_si256((const __m256i*)&simdExpandTable8[_mm256_movemask_ps(mask.m)]);
  return _mm256_permutevar8x32_epi32(_mm256_maskload_epi32(ptr, key), key);
}

prt_inline vint8 gather(const int* ptr, const vint8& idx)
{
  return _mm256_i32gather_epi32(ptr, idx.m, 4);
}

prt_inline vint8 gather(const vbool8& mask, const int* ptr, const vint8& idx)
{
  return _mm256_mask_i32gather_epi32(_mm256_setzero_si256(), ptr, idx.m, _mm256_castps_si256(mask.m), 4);
}

// Store functions
// ---------------

prt_inline void store(int* ptr, const vint8& a)
{
  _mm256_store_si256((__m256i*)ptr, a.m);
}

prt_inline void store(const vbool8& mask, int* ptr, const vint8& a)
{
  _mm256_maskstore_epi32(ptr, _mm256_castps_si256(mask.m), a.m);
}

prt_inline void storeu(int* ptr, const vint8& a)
{
  _mm256_storeu_si256((__m256i*)ptr, a.m);
}

prt_inline void storeu(const vbool8& mask, int* ptr, const vint8& a)
{
  _mm256_maskstore_epi32(ptr, _mm256_castps_si256(mask.m), a.m);
}

prt_inline void compress(const vbool8& mask, int* ptr, const vint8& a)
{
  __m256i key = _mm256_load_si256((const __m256i*)&simdCompressTable8[_mm256_movemask_ps(mask.m)]);
  _mm256_maskstore_epi32(ptr, key, _mm256_permutevar8x32_epi32(a.m, key));
}

prt_inline void scatter(int* ptr, const vint8& idx, const vint8& a)
{
  for (int i = 0; i < 8; ++i)
    ptr[idx[i]] = a[i];
}

prt_inline void scatter(const vbool8& mask, int* ptr, const vint8& idx, const vint8& a)
{
  for (int i = 0; i < 8; ++i)
    if (mask[i]) ptr[idx[i]] = a[i];
}

// Select function
// ---------------

prt_inline vint8 select(const vbool8& mask, const vint8& a, const vint8& b)
{
  return _mm256_blendv_epi8(b.m, a.m, _mm256_castps_si256(mask.m));
}

// Arithmetic operators
// --------------------

prt_inline vint8 operator +(const vint8& a, const vint8& b)
{
  return _mm256_add_epi32(a.m, b.m);
}

prt_inline vint8 operator -(const vint8& a, const vint8& b)
{
  return _mm256_sub_epi32(a.m, b.m);
}

prt_inline vint8 operator *(const vint8& a, const vint8& b)
{
  return _mm256_mullo_epi32(a.m, b.m);
}

// Bitwise operators
// -----------------

prt_inline vint8 operator &(const vint8& a, const vint8& b)
{
   return _mm256_and_si256(a.m, b.m);
}

prt_inline vint8 operator |(const vint8& a, const vint8& b)
{
   return _mm256_or_si256(a.m, b.m);
}

prt_inline vint8 operator ^(const vint8& a, const vint8& b)
{
   return _mm256_xor_si256(a.m, b.m);
}

prt_inline vint8 andn(const vint8& a, const vint8& b)
{
  return _mm256_andnot_si256(b.m, a.m);
}

prt_inline vint8 operator <<(const vint8& a, int b)
{
  return _mm256_slli_epi32(a.m, b);
}

prt_inline vint8 operator >>(const vint8& a, int b)
{
  return _mm256_srai_epi32(a.m, b);
}

prt_inline vint8 shl(const vint8& a, int b)
{
  return _mm256_slli_epi32(a.m, b);
}

prt_inline vint8 shr(const vint8& a, int b)
{
  return _mm256_srli_epi32(a.m, b);
}

// Assignment operators
// --------------------

prt_inline vint8& operator +=(vint8& a, const vint8& b) { return a = a + b; }
prt_inline vint8& operator -=(vint8& a, const vint8& b) { return a = a - b; }
prt_inline vint8& operator *=(vint8& a, const vint8& b) { return a = a * b; }
prt_inline vint8& operator &=(vint8& a, const vint8& b) { return a = a & b; }
prt_inline vint8& operator |=(vint8& a, const vint8& b) { return a = a | b; }
prt_inline vint8& operator ^=(vint8& a, const vint8& b) { return a = a ^ b; }

prt_inline vint8& operator <<=(vint8& a, int b) { return a = a << b; }
prt_inline vint8& operator >>=(vint8& a, int b) { return a = a >> b; }

// Compare operators
// -----------------

prt_inline vbool8 operator ==(const vint8& a, const vint8& b)
{
  return _mm256_cmpeq_epi32(a.m, b.m);
}

prt_inline vbool8 operator <(const vint8& a, const vint8& b)
{
  return _mm256_cmpgt_epi32(b.m, a.m);
}

prt_inline vbool8 operator >(const vint8& a, const vint8& b)
{
  return _mm256_cmpgt_epi32(a.m, b.m);
}

prt_inline vbool8 operator !=(const vint8& a, const vint8& b)
{
  return !(a == b);
}

prt_inline vbool8 operator <=(const vint8& a, const vint8& b)
{
  return !(a > b);
}

prt_inline vbool8 operator >=(const vint8& a, const vint8& b)
{
  return !(a < b);
}

// Compare functions
// -----------------

prt_inline vbool8 cmpEq(const vint8& a, const vint8& b) { return a == b; }
prt_inline vbool8 cmpNe(const vint8& a, const vint8& b) { return a != b; }
prt_inline vbool8 cmpLt(const vint8& a, const vint8& b) { return a <  b; }
prt_inline vbool8 cmpLe(const vint8& a, const vint8& b) { return a <= b; }
prt_inline vbool8 cmpGt(const vint8& a, const vint8& b) { return a >  b; }
prt_inline vbool8 cmpGe(const vint8& a, const vint8& b) { return a >= b; }

prt_inline vbool8 cmpEq(const vbool8& mask, const vint8& a, const vint8& b) { return (a == b) & mask; }
prt_inline vbool8 cmpNe(const vbool8& mask, const vint8& a, const vint8& b) { return (a != b) & mask; }
prt_inline vbool8 cmpLt(const vbool8& mask, const vint8& a, const vint8& b) { return (a <  b) & mask; }
prt_inline vbool8 cmpLe(const vbool8& mask, const vint8& a, const vint8& b) { return (a <= b) & mask; }
prt_inline vbool8 cmpGt(const vbool8& mask, const vint8& a, const vint8& b) { return (a >  b) & mask; }
prt_inline vbool8 cmpGe(const vbool8& mask, const vint8& a, const vint8& b) { return (a >= b) & mask; }

// Math functions
// --------------

prt_inline vint8 min(const vint8& a, const vint8& b)
{
  return _mm256_min_epi32(a.m, b.m);
}

prt_inline vint8 max(const vint8& a, const vint8& b)
{
  return _mm256_max_epi32(a.m, b.m);
}

} // namespace prt
