// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <immintrin.h>
#include "../simd_common.h"
#include "vbool16_avx512.h"
#include "vint16_avx512.h"
#include "vuint16_avx512.h"

namespace prt {

// vfloat16
template <>
struct var<float,16>
{
  union
  {
    __m512 m;
    float v[16];
  };

  prt_inline var() {}

  prt_inline var(const vfloat16& a) : m(a.m) {}
  prt_inline var(__m512 a) : m(a) {}
  prt_inline var(float a) : m(_mm512_set1_ps(a)) {}
  prt_inline var(float a0, float a1, float a2, float a3) : m(_mm512_set4_ps(a3, a2, a1, a0)) {}

  prt_inline var(float a0, float a1, float a2,  float a3,  float a4,  float a5,  float a6,  float a7,
                 float a8, float a9, float a10, float a11, float a12, float a13, float a14, float a15)
    : m(_mm512_set_ps(a15, a14, a13, a12, a11, a10, a9, a8, a7, a6, a5, a4, a3, a2, a1, a0)) {}

  prt_inline var(Zero)   : m(_mm512_setzero_ps()) {}
  prt_inline var(One)    : m(_mm512_set1_ps(1.0f)) {}
  prt_inline var(PosMax) : m(_mm512_set1_ps(posMax)) {}
  prt_inline var(NegMax) : m(_mm512_set1_ps(negMax)) {}
  prt_inline var(PosInf) : m(_mm512_set1_ps(posInf)) {}
  prt_inline var(NegInf) : m(_mm512_set1_ps(negInf)) {}
  prt_inline var(Qnan)   : m(_mm512_set1_ps(qnan)) {}
  prt_inline var(Step)   : m(_mm512_set_ps(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)) {}

  prt_inline vfloat16& operator =(const vfloat16& b)
  {
    m = b.m;
    return *this;
  }

  prt_inline const float& operator [](size_t i) const
  {
    assert(i < 16);
    return v[i];
  }

  prt_inline float& operator [](size_t i)
  {
    assert(i < 16);
    return v[i];
  }

  prt_inline const int& getInt(size_t i) const
  {
    assert(i < 16);
    return ((const int*)this)[i];
  }

  prt_inline int& getInt(size_t i)
  {
    assert(i < 16);
    return ((int*)this)[i];
  }
};

// Load functions
// --------------

template <>
prt_inline vfloat16 load(const float* ptr)
{
  return _mm512_load_ps(ptr);
}

template <>
prt_inline vfloat16 load(const vbool16& mask, const float* ptr)
{
  return _mm512_maskz_load_ps(mask.m, ptr);
}

template <>
prt_inline vfloat16 loadu(const float* ptr)
{
  return _mm512_loadu_ps(ptr);
}

template <>
prt_inline vfloat16 loadu(const vbool16& mask, const float* ptr)
{
  return _mm512_maskz_loadu_ps(mask.m, ptr);
}

template <>
prt_inline vfloat16 load4(const float* ptr)
{
  return _mm512_broadcast_f32x4(_mm_load_ps(ptr));
}

prt_inline vfloat16 expand(const vbool16& mask, const float* ptr)
{
  return _mm512_maskz_expandloadu_ps(mask.m, ptr);
}

prt_inline vfloat16 gather(const float* ptr, const vint16& idx)
{
  return _mm512_i32gather_ps(idx.m, ptr, 4);
}

prt_inline vfloat16 gather(const vbool16& mask, const float* ptr, const vint16& idx)
{
  return _mm512_mask_i32gather_ps(_mm512_setzero_ps(), mask.m, idx.m, ptr, 4);
}

// Store functions
// ---------------

prt_inline void store(float* ptr, const vfloat16& a)
{
  _mm512_store_ps(ptr, a.m);
}

prt_inline void store(const vbool16& mask, float* ptr, const vfloat16& a)
{
  _mm512_mask_store_ps(ptr, mask.m, a.m);
}

prt_inline void storeu(float* ptr, const vfloat16& a)
{
  _mm512_storeu_ps(ptr, a.m);
}

prt_inline void storeu(const vbool16& mask, float* ptr, const vfloat16& a)
{
  _mm512_mask_storeu_ps(ptr, mask.m, a.m);
}

prt_inline void compress(const vbool16& mask, float* ptr, const vfloat16& a)
{
  _mm512_mask_compressstoreu_ps(ptr, mask.m, a.m);
}

prt_inline void scatter(float* ptr, const vint16& idx, const vfloat16& a)
{
  _mm512_i32scatter_ps(ptr, idx.m, a.m, 4);
}

prt_inline void scatter(const vbool16& mask, float* ptr, const vint16& idx, const vfloat16& a)
{
  _mm512_mask_i32scatter_ps(ptr, mask.m, idx.m, a.m, 4);
}

// Conversion functions
// --------------------

prt_inline float toScalar(const vfloat16& a)
{
  return _mm512_cvtss_f32(a.m);
}

prt_inline vfloat16 toFloat(const vint16& a)
{
  return _mm512_cvt_roundepi32_ps(a.m, _MM_FROUND_CUR_DIRECTION);
}

prt_inline vfloat16 toFloat(const vuint16& a)
{
  return _mm512_cvt_roundepu32_ps(a.m, _MM_FROUND_CUR_DIRECTION);
}

prt_inline vfloat16 asFloat(const vint16& a)
{
  return _mm512_castsi512_ps(a.m);
}

prt_inline vint16 toInt(const vfloat16& a)
{
  return _mm512_cvt_roundps_epi32(a.m, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC);
}

prt_inline vint16 asInt(const vfloat16& a)
{
  return _mm512_castps_si512(a.m);
}

prt_inline vuint16 asUInt(const vfloat16& a)
{
  return _mm512_castps_si512(a.m);
}

// Select function
// ---------------

prt_inline vfloat16 select(const vbool16& mask, const vfloat16& a, const vfloat16& b)
{
  return _mm512_mask_mov_ps(b.m, mask.m, a.m);
}

// Shuffle functions
// -----------------

template <int i0, int i1, int i2, int i3>
prt_inline vfloat16 shuffle(const vfloat16& a)
{
  return _mm512_permute_ps(a.m, _MM_SHUFFLE(i3, i2, i1, i0));
}

template <int i0, int i1, int i2, int i3>
prt_inline vfloat16 shuffle4(const vfloat16& a)
{
  return _mm512_shuffle_f32x4(a.m, a.m, _MM_SHUFFLE(i3, i2, i1, i0));
}

prt_inline vfloat16 permute(const vfloat16& a, const vint16& idx)
{
  return _mm512_castsi512_ps(_mm512_permutexvar_epi32(idx.m, _mm512_castps_si512(a.m)));
}

// Arithmetic operators
// --------------------

prt_inline vfloat16 operator -(const vfloat16& a)
{
  return _mm512_sub_ps(_mm512_setzero_ps(), a.m);
}

prt_inline vfloat16 operator +(const vfloat16& a, const vfloat16& b)
{
  return _mm512_add_ps(a.m, b.m);
}

prt_inline vfloat16 operator -(const vfloat16& a, const vfloat16& b)
{
  return _mm512_sub_ps(a.m, b.m);
}

prt_inline vfloat16 operator *(const vfloat16& a, const vfloat16& b)
{
  return _mm512_mul_ps(a.m, b.m);
}

prt_inline vfloat16 operator /(const vfloat16& a, const vfloat16& b)
{
  //return _mm512_div_ps(a.m, b.m);
#if defined(__AVX512ER__)
  return _mm512_mul_ps(a.m, _mm512_rcp28_ps(b.m));
#else
  __m512 r = _mm512_rcp14_ps(b.m);
  r = _mm512_sub_ps(_mm512_add_ps(r, r), _mm512_mul_ps(_mm512_mul_ps(r, r), b.m));
  return _mm512_mul_ps(a.m, r);
#endif
}

// Bitwise operators
// -----------------

prt_inline vfloat16 operator &(const vfloat16& a, const vfloat16& b)
{
  return _mm512_castsi512_ps(_mm512_and_epi32(_mm512_castps_si512(a.m), _mm512_castps_si512(b.m)));
}

prt_inline vfloat16 operator |(const vfloat16& a, const vfloat16& b)
{
  return _mm512_castsi512_ps(_mm512_or_epi32(_mm512_castps_si512(a.m), _mm512_castps_si512(b.m)));
}

prt_inline vfloat16 operator ^(const vfloat16& a, const vfloat16& b)
{
  return _mm512_castsi512_ps(_mm512_xor_epi32(_mm512_castps_si512(a.m), _mm512_castps_si512(b.m)));
}

// Assignment operators
// --------------------

prt_inline vfloat16& operator +=(vfloat16& a, const vfloat16& b) { return a = a + b; }
prt_inline vfloat16& operator -=(vfloat16& a, const vfloat16& b) { return a = a - b; }
prt_inline vfloat16& operator *=(vfloat16& a, const vfloat16& b) { return a = a * b; }
prt_inline vfloat16& operator /=(vfloat16& a, const vfloat16& b) { return a = a / b; }
prt_inline vfloat16& operator &=(vfloat16& a, const vfloat16& b) { return a = a & b; }
prt_inline vfloat16& operator |=(vfloat16& a, const vfloat16& b) { return a = a | b; }
prt_inline vfloat16& operator ^=(vfloat16& a, const vfloat16& b) { return a = a ^ b; }

// Compare operators
// -----------------

prt_inline vbool16 operator ==(const vfloat16& a, const vfloat16& b)
{
  return _mm512_cmp_ps_mask(a.m, b.m, _CMP_EQ_OQ);
}

prt_inline vbool16 operator !=(const vfloat16& a, const vfloat16& b)
{
  return _mm512_cmp_ps_mask(a.m, b.m, _CMP_NEQ_UQ);
}

prt_inline vbool16 operator <(const vfloat16& a, const vfloat16& b)
{
  return _mm512_cmp_ps_mask(a.m, b.m, _CMP_LT_OS);
}

prt_inline vbool16 operator >(const vfloat16& a, const vfloat16& b)
{
  return _mm512_cmp_ps_mask(a.m, b.m, _CMP_GT_OS);
}

prt_inline vbool16 operator <=(const vfloat16& a, const vfloat16& b)
{
  return _mm512_cmp_ps_mask(a.m, b.m, _CMP_LE_OS);
}

prt_inline vbool16 operator >=(const vfloat16& a, const vfloat16& b)
{
  return _mm512_cmp_ps_mask(a.m, b.m, _CMP_GE_OS);
}

// Compare functions
// -----------------

prt_inline vbool16 cmpEq(const vfloat16& a, const vfloat16& b) { return a == b; }
prt_inline vbool16 cmpNe(const vfloat16& a, const vfloat16& b) { return a != b; }
prt_inline vbool16 cmpLt(const vfloat16& a, const vfloat16& b) { return a <  b; }
prt_inline vbool16 cmpLe(const vfloat16& a, const vfloat16& b) { return a <= b; }
prt_inline vbool16 cmpGt(const vfloat16& a, const vfloat16& b) { return a >  b; }
prt_inline vbool16 cmpGe(const vfloat16& a, const vfloat16& b) { return a >= b; }

prt_inline vbool16 cmpEq(const vbool16& mask, const vfloat16& a, const vfloat16& b)
{
  return _mm512_mask_cmp_ps_mask(mask.m, a.m, b.m, _CMP_EQ_OQ);
}

prt_inline vbool16 cmpNe(const vbool16& mask, const vfloat16& a, const vfloat16& b)
{
  return _mm512_mask_cmp_ps_mask(mask.m, a.m, b.m, _CMP_NEQ_UQ);
}

prt_inline vbool16 cmpLt(const vbool16& mask, const vfloat16& a, const vfloat16& b)
{
  return _mm512_mask_cmp_ps_mask(mask.m, a.m, b.m, _CMP_LT_OS);
}

prt_inline vbool16 cmpGt(const vbool16& mask, const vfloat16& a, const vfloat16& b)
{
  return _mm512_mask_cmp_ps_mask(mask.m, a.m, b.m, _CMP_GT_OS);
}

prt_inline vbool16 cmpLe(const vbool16& mask, const vfloat16& a, const vfloat16& b)
{
  return _mm512_mask_cmp_ps_mask(mask.m, a.m, b.m, _CMP_LE_OS);
}

prt_inline vbool16 cmpGe(const vbool16& mask, const vfloat16& a, const vfloat16& b)
{
  return _mm512_mask_cmp_ps_mask(mask.m, a.m, b.m, _CMP_GE_OS);
}

// Math functions
// --------------

prt_inline vfloat16 abs(const vfloat16& a)
{
  return _mm512_abs_ps(a.m);
}

prt_inline vfloat16 rcp(const vfloat16& a)
{
#if defined(__AVX512ER__)
  return _mm512_rcp28_ps(a.m);
#else
  __m512 r = _mm512_rcp14_ps(a.m);
  return _mm512_sub_ps(_mm512_add_ps(r, r), _mm512_mul_ps(_mm512_mul_ps(r, r), a.m));
#endif
}

prt_inline vfloat16 sqrt(const vfloat16& a)
{
  return _mm512_sqrt_ps(a.m);
}

prt_inline vfloat16 rsqrt(const vfloat16& a)
{
#if defined(__AVX512ER__)
  return _mm512_rsqrt28_ps(a.m);
#else
  __m512 r = _mm512_rsqrt14_ps(a.m);
  return _mm512_add_ps(_mm512_mul_ps(_mm512_set1_ps(1.5f), r), _mm512_mul_ps(_mm512_mul_ps(_mm512_mul_ps(a.m, _mm512_set1_ps(-0.5f)), r), _mm512_mul_ps(r, r)));
#endif
}

prt_inline vfloat16 floor(const vfloat16& a)
{
  return _mm512_floor_ps(a.m);
}

prt_inline vfloat16 ceil(const vfloat16& a)
{
  return _mm512_ceil_ps(a.m);
}

prt_inline vfloat16 round(const vfloat16& a)
{
  return _mm512_roundscale_ps(a.m, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
}

prt_inline vfloat16 min(const vfloat16&a, const vfloat16& b)
{
  return _mm512_min_ps(a.m, b.m);
}

prt_inline vfloat16 max(const vfloat16&a, const vfloat16& b)
{
  return _mm512_max_ps(a.m, b.m);
}

prt_inline vfloat16 vdot4(const vfloat16& a, const vfloat16& b)
{
  vfloat16 p = a * b;
  vfloat16 s = p + shuffle<1,0,3,2>(p);
  return s + shuffle<2,3,0,1>(s);
}

prt_inline vfloat16 cross(const vfloat16& a, const vfloat16& b)
{
  vfloat16 p2 = b * shuffle<1,2,0,3>(a);
  vfloat16 s = a * shuffle<1,2,0,3>(b) - p2;
  return shuffle<1,2,0,3>(s);
}

prt_inline vfloat16 vreduceMin(const vfloat16& a)
{
  vfloat16 t = min(a, shuffle<1,0,3,2>(a));
  t = min(t, shuffle<2,3,0,1>(t));
  t = min(t, shuffle4<1,0,3,2>(t));
  return min(t, shuffle4<2,3,0,1>(t));
}

prt_inline vfloat16 vreduceMax(const vfloat16& a)
{
  vfloat16 t = max(a, shuffle<1,0,3,2>(a));
  t = max(t, shuffle<2,3,0,1>(t));
  t = max(t, shuffle4<1,0,3,2>(t));
  return max(t, shuffle4<2,3,0,1>(t));
}

} // namespace prt
