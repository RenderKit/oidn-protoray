// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <immintrin.h>
#include "../simd_common.h"
#include "vbool8_avx.h"
#include "vint8_avx2.h"

namespace prt {

// vuint8
template <>
struct var<uint,8>
{
  union
  {
    __m256i m;
    uint v[8];
  };

  prt_inline var() {}

  prt_inline var(const vuint8& a) : m(a.m) {}
  prt_inline var(const vint8& a) : m(a.m) {}
  prt_inline var(__m256i a) : m(a) {}
  prt_inline var(uint a) : m(_mm256_set1_epi32(a)) {}
  prt_inline var(uint a0, uint a1, uint a2, uint a3, uint a4, uint a5, uint a6, uint a7) : m(_mm256_set_epi32(a7, a6, a5, a4, a3, a2, a1, a0)) {}

  prt_inline var(Zero) : m(_mm256_setzero_si256()) {}
  prt_inline var(One)  : m(_mm256_set1_epi32(1)) {}
  prt_inline var(Step) : m(_mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0)) {}

  prt_inline vuint8& operator =(const vuint8& a)
  {
    m = a.m;
    return *this;
  }

  prt_inline const uint& operator [](size_t i) const
  {
    assert(i < 8);
    return v[i];
  }

  prt_inline uint& operator [](size_t i)
  {
    assert(i < 8);
    return v[i];
  }
};

prt_inline vint8::var(const vuint8& a) : m(a.m) {}

// Load functions
// --------------

template <>
prt_inline vuint8 load(const uint* ptr)
{
  return _mm256_load_si256((const __m256i*)ptr);
}

template <>
prt_inline vuint8 load(const vbool8& mask, const uint* ptr)
{
  return _mm256_maskload_epi32((const int*)ptr, _mm256_castps_si256(mask.m));
}

template <>
prt_inline vuint8 loadu(const uint* ptr)
{
  return _mm256_loadu_si256((const __m256i*)ptr);
}

template <>
prt_inline vuint8 loadu(const vbool8& mask, const uint* ptr)
{
  return _mm256_maskload_epi32((const int*)ptr, _mm256_castps_si256(mask.m));
}

prt_inline vuint8 expand(const vbool8& mask, const uint* ptr)
{
  __m256i key = _mm256_load_si256((const __m256i*)&simdExpandTable8[_mm256_movemask_ps(mask.m)]);
  return _mm256_permutevar8x32_epi32(_mm256_maskload_epi32((const int*)ptr, key), key);
}

prt_inline vuint8 gather(const uint* ptr, const vint8& idx)
{
  return _mm256_i32gather_epi32((const int*)ptr, idx.m, 4);
}

prt_inline vuint8 gather(const vbool8& mask, const uint* ptr, const vint8& idx)
{
  return _mm256_mask_i32gather_epi32(_mm256_setzero_si256(), (const int*)ptr, idx.m, _mm256_castps_si256(mask.m), 4);
}

// Store functions
// ---------------

prt_inline void store(uint* ptr, const vuint8& a)
{
  _mm256_store_si256((__m256i*)ptr, a.m);
}

prt_inline void store(const vbool8& mask, uint* ptr, const vuint8& a)
{
  _mm256_maskstore_epi32((int*)ptr, _mm256_castps_si256(mask.m), a.m);
}

prt_inline void storeu(uint* ptr, const vuint8& a)
{
  _mm256_storeu_si256((__m256i*)ptr, a.m);
}

prt_inline void storeu(const vbool8& mask, uint* ptr, const vuint8& a)
{
  _mm256_maskstore_epi32((int*)ptr, _mm256_castps_si256(mask.m), a.m);
}

prt_inline void compress(const vbool8& mask, uint* ptr, const vuint8& a)
{
  __m256i key = _mm256_load_si256((const __m256i*)&simdCompressTable8[_mm256_movemask_ps(mask.m)]);
  _mm256_maskstore_epi32((int*)ptr, key, _mm256_permutevar8x32_epi32(a.m, key));
}

prt_inline void scatter(uint* ptr, const vint8& idx, const vuint8& a)
{
  for (uint i = 0; i < 8; ++i)
    ptr[idx[i]] = a[i];
}

prt_inline void scatter(const vbool8& mask, uint* ptr, const vint8& idx, const vuint8& a)
{
  for (uint i = 0; i < 8; ++i)
    if (mask[i]) ptr[idx[i]] = a[i];
}

// Select function
// ---------------

prt_inline vuint8 select(const vbool8& mask, const vuint8& a, const vuint8& b)
{
  return _mm256_blendv_epi8(b.m, a.m, _mm256_castps_si256(mask.m));
}

// Arithmetic operators
// --------------------

prt_inline vuint8 operator +(const vuint8& a, const vuint8& b)
{
  return _mm256_add_epi32(a.m, b.m);
}

prt_inline vuint8 operator -(const vuint8& a, const vuint8& b)
{
  return _mm256_sub_epi32(a.m, b.m);
}

prt_inline vuint8 operator *(const vuint8& a, const vuint8& b)
{
  return _mm256_mullo_epi32(a.m, b.m);
}

// Bitwise operators
// -----------------

prt_inline vuint8 operator &(const vuint8& a, const vuint8& b)
{
   return _mm256_and_si256(a.m, b.m);
}

prt_inline vuint8 operator |(const vuint8& a, const vuint8& b)
{
   return _mm256_or_si256(a.m, b.m);
}

prt_inline vuint8 operator ^(const vuint8& a, const vuint8& b)
{
   return _mm256_xor_si256(a.m, b.m);
}

prt_inline vuint8 andn(const vuint8& a, const vuint8& b)
{
  return _mm256_andnot_si256(b.m, a.m);
}

prt_inline vuint8 operator <<(const vuint8& a, int b)
{
  return _mm256_slli_epi32(a.m, b);
}

prt_inline vuint8 operator >>(const vuint8& a, int b)
{
  return _mm256_srli_epi32(a.m, b);
}

prt_inline vuint8 shl(const vuint8& a, int b)
{
  return _mm256_slli_epi32(a.m, b);
}

prt_inline vuint8 shr(const vuint8& a, int b)
{
  return _mm256_srli_epi32(a.m, b);
}

// Assignment operators
// --------------------

prt_inline vuint8& operator +=(vuint8& a, const vuint8& b) { return a = a + b; }
prt_inline vuint8& operator -=(vuint8& a, const vuint8& b) { return a = a - b; }
prt_inline vuint8& operator *=(vuint8& a, const vuint8& b) { return a = a * b; }
prt_inline vuint8& operator &=(vuint8& a, const vuint8& b) { return a = a & b; }
prt_inline vuint8& operator |=(vuint8& a, const vuint8& b) { return a = a | b; }
prt_inline vuint8& operator ^=(vuint8& a, const vuint8& b) { return a = a ^ b; }

prt_inline vuint8& operator <<=(vuint8& a, uint b) { return a = a << b; }
prt_inline vuint8& operator >>=(vuint8& a, uint b) { return a = a >> b; }

// Compare operators
// -----------------

prt_inline vbool8 operator ==(const vuint8& a, const vuint8& b)
{
  return _mm256_cmpeq_epi32(a.m, b.m);
}

/*
prt_inline vbool8 operator <(const vuint8& a, const vuint8& b)
{
  return _mm256_cmpgt_epi32(b.m, a.m);
}

prt_inline vbool8 operator >(const vuint8& a, const vuint8& b)
{
  return _mm256_cmpgt_epi32(a.m, b.m);
}
*/

prt_inline vbool8 operator !=(const vuint8& a, const vuint8& b)
{
  return !(a == b);
}

/*
prt_inline vbool8 operator <=(const vuint8& a, const vuint8& b)
{
  return !(a > b);
}

prt_inline vbool8 operator >=(const vuint8& a, const vuint8& b)
{
  return !(a < b);
}
*/

// Compare functions
// -----------------

prt_inline vbool8 cmpEq(const vuint8& a, const vuint8& b) { return a == b; }
prt_inline vbool8 cmpNe(const vuint8& a, const vuint8& b) { return a != b; }
/*
prt_inline vbool8 cmpLt(const vuint8& a, const vuint8& b) { return a <  b; }
prt_inline vbool8 cmpLe(const vuint8& a, const vuint8& b) { return a <= b; }
prt_inline vbool8 cmpGt(const vuint8& a, const vuint8& b) { return a >  b; }
prt_inline vbool8 cmpGe(const vuint8& a, const vuint8& b) { return a >= b; }
*/

prt_inline vbool8 cmpEq(const vbool8& mask, const vuint8& a, const vuint8& b) { return (a == b) & mask; }
prt_inline vbool8 cmpNe(const vbool8& mask, const vuint8& a, const vuint8& b) { return (a != b) & mask; }
/*
prt_inline vbool8 cmpLt(const vbool8& mask, const vuint8& a, const vuint8& b) { return (a <  b) & mask; }
prt_inline vbool8 cmpLe(const vbool8& mask, const vuint8& a, const vuint8& b) { return (a <= b) & mask; }
prt_inline vbool8 cmpGt(const vbool8& mask, const vuint8& a, const vuint8& b) { return (a >  b) & mask; }
prt_inline vbool8 cmpGe(const vbool8& mask, const vuint8& a, const vuint8& b) { return (a >= b) & mask; }
*/

// Math functions
// --------------

prt_inline vuint8 min(const vuint8& a, const vuint8& b)
{
  return _mm256_min_epu32(a.m, b.m);
}

prt_inline vuint8 max(const vuint8& a, const vuint8& b)
{
  return _mm256_max_epu32(a.m, b.m);
}

} // namespace prt
