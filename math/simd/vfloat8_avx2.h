// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <immintrin.h>
#include "../simd_common.h"
#include "vbool8_avx.h"
#include "vint8_avx2.h"
#include "vuint8_avx2.h"

namespace prt {

// vfloat8
template <>
struct var<float,8>
{
  union
  {
    __m256 m;
    float v[8];
  };

  prt_inline var() {}

  prt_inline var(const vfloat8& a) : m(a.m) {}
  prt_inline var(__m256 a) : m(a) {}
  prt_inline var(float a) : m(_mm256_set1_ps(a)) {}
  prt_inline var(float a0, float a1, float a2, float a3, float a4, float a5, float a6, float a7) : m(_mm256_set_ps(a7, a6, a5, a4, a3, a2, a1, a0)) {}

  prt_inline var(Zero)   : m(_mm256_setzero_ps()) {}
  prt_inline var(One)    : m(_mm256_set1_ps(1.0f)) {}
  prt_inline var(PosMax) : m(_mm256_set1_ps(posMax)) {}
  prt_inline var(NegMax) : m(_mm256_set1_ps(negMax)) {}
  prt_inline var(PosInf) : m(_mm256_set1_ps(posInf)) {}
  prt_inline var(NegInf) : m(_mm256_set1_ps(negInf)) {}
  prt_inline var(Qnan)   : m(_mm256_set1_ps(qnan)) {}
  prt_inline var(Step)   : m(_mm256_set_ps(7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f, 0.0f)) {}

  prt_inline vfloat8& operator =(const vfloat8& a)
  {
    m = a.m;
    return *this;
  }

  prt_inline const float& operator [](size_t i) const
  {
    assert(i < 8);
    return v[i];
  }

  prt_inline float& operator [](size_t i)
  {
    assert(i < 8);
    return v[i];
  }
};

// Load functions
// --------------

template <>
prt_inline vfloat8 load(const float* ptr)
{
  return _mm256_load_ps(ptr);
}

template <>
prt_inline vfloat8 load(const vbool8& mask, const float* ptr)
{
  return _mm256_maskload_ps(ptr, _mm256_castps_si256(mask.m));
}

template <>
prt_inline vfloat8 loadu(const float* ptr)
{
  return _mm256_loadu_ps(ptr);
}

template <>
prt_inline vfloat8 loadu(const vbool8& mask, const float* ptr)
{
  return _mm256_maskload_ps(ptr, _mm256_castps_si256(mask.m));
}

prt_inline vfloat8 expand(const vbool8& mask, const float* ptr)
{
  __m256i key = _mm256_load_si256((const __m256i*)&simdExpandTable8[_mm256_movemask_ps(mask.m)]);
  return _mm256_permutevar8x32_ps(_mm256_maskload_ps(ptr, key), key);
}

prt_inline vfloat8 gather(const float* ptr, const vint8& idx)
{
  return _mm256_i32gather_ps(ptr, idx.m, 4);
}

prt_inline vfloat8 gather(const vbool8& mask, const float* ptr, const vint8& idx)
{
  return _mm256_mask_i32gather_ps(_mm256_setzero_ps(), ptr, idx.m, mask.m, 4);
}

// Store functions
// ---------------

prt_inline void store(float* ptr, const vfloat8& a)
{
  _mm256_store_ps(ptr, a.m);
}

prt_inline void store(const vbool8& mask, float* ptr, const vfloat8& a)
{
  _mm256_maskstore_ps(ptr, _mm256_castps_si256(mask.m), a.m);
}

prt_inline void storeu(float* ptr, const vfloat8& a)
{
  _mm256_storeu_ps(ptr, a.m);
}

prt_inline void storeu(const vbool8& mask, float* ptr, const vfloat8& a)
{
  _mm256_maskstore_ps(ptr, _mm256_castps_si256(mask.m), a.m);
}

prt_inline void compress(const vbool8& mask, float* ptr, const vfloat8& a)
{
  __m256i key = _mm256_load_si256((const __m256i*)&simdCompressTable8[_mm256_movemask_ps(mask.m)]);
  _mm256_maskstore_ps(ptr, key, _mm256_permutevar8x32_ps(a.m, key));
}

prt_inline void scatter(float* ptr, const vint8& idx, const vfloat8& a)
{
  for (int i = 0; i < 8; ++i)
    ptr[idx[i]] = a[i];
}

prt_inline void scatter(const vbool8& mask, float* ptr, const vint8& idx, const vfloat8& a)
{
  for (int i = 0; i < 8; ++i)
    if (mask[i]) ptr[idx[i]] = a[i];
}

// Conversion functions
// --------------------

prt_inline float toScalar(const vfloat8& a)
{
  return _mm_cvtss_f32(_mm256_castps256_ps128(a.m));
}

prt_inline vint8 toInt(const vfloat8& a)
{
  return _mm256_cvttps_epi32(a.m);
}

prt_inline vint8 asInt(const vfloat8& a)
{
  return _mm256_castps_si256(a.m);
}

prt_inline vuint8 asUInt(const vfloat8& a)
{
  return _mm256_castps_si256(a.m);
}

prt_inline vfloat8 toFloat(const vint8& a)
{
  return _mm256_cvtepi32_ps(a.m);
}

prt_inline vfloat8 toFloat(const vuint8& x)
{
  const __m256i a  = _mm256_and_si256(x.m, _mm256_set1_epi32(0x7FFFFFFF));
  const __m256i b  = _mm256_and_si256(_mm256_srai_epi32(x.m, 31), _mm256_set1_epi32(0x4F000000)); // 0x4F000000 = 2^31
  const __m256  af = _mm256_cvtepi32_ps(a);
  const __m256  bf = _mm256_castsi256_ps(b);
  return _mm256_add_ps(af, bf);
}

prt_inline vfloat8 asFloat(const vint8& a)
{
  return _mm256_castsi256_ps(a.m);
}

// FIXME
prt_inline int toIntMask(const vfloat8& a)
{
  return _mm256_movemask_ps(a.m);
}

// Select function
// ---------------

prt_inline vfloat8 select(const vbool8& mask, const vfloat8& a, const vfloat8& b)
{
  return _mm256_blendv_ps(b.m, a.m, mask.m);
}

// Arithmetic operators
// --------------------

prt_inline vfloat8 operator +(const vfloat8& a)
{
  return a;
}

prt_inline vfloat8 operator -(const vfloat8& a)
{
  return _mm256_sub_ps(_mm256_setzero_ps(), a.m);
}

prt_inline vfloat8 operator +(const vfloat8& a, const vfloat8& b)
{
  return _mm256_add_ps(a.m, b.m);
}

prt_inline vfloat8 operator -(const vfloat8& a, const vfloat8& b)
{
  return _mm256_sub_ps(a.m, b.m);
}

prt_inline vfloat8 operator *(const vfloat8& a, const vfloat8& b)
{
  return _mm256_mul_ps(a.m, b.m);
}

prt_inline vfloat8 operator /(const vfloat8& a, const vfloat8& b)
{
  //return _mm256_div_ps(a.m, b.m);
  __m256 r = _mm256_rcp_ps(b.m);
  r = _mm256_sub_ps(_mm256_add_ps(r, r), _mm256_mul_ps(_mm256_mul_ps(r, r), b.m));
  return _mm256_mul_ps(a.m, r);
}

// Bitwise operators
// -----------------

prt_inline vfloat8 operator &(const vfloat8& a, const vfloat8& b)
{
  return _mm256_and_ps(a.m, b.m);
}

prt_inline vfloat8 operator |(const vfloat8& a, const vfloat8& b)
{
  return _mm256_or_ps(a.m, b.m);
}

prt_inline vfloat8 operator ^(const vfloat8& a, const vfloat8& b)
{
  return _mm256_xor_ps(a.m, b.m);
}

prt_inline vfloat8 andn(const vfloat8& a, const vfloat8& b)
{
  return _mm256_andnot_ps(b.m, a.m);
}

// Assignment operators
// --------------------

prt_inline vfloat8& operator +=(vfloat8& a, const vfloat8& b) { return a = a + b; }
prt_inline vfloat8& operator -=(vfloat8& a, const vfloat8& b) { return a = a - b; }
prt_inline vfloat8& operator *=(vfloat8& a, const vfloat8& b) { return a = a * b; }
prt_inline vfloat8& operator /=(vfloat8& a, const vfloat8& b) { return a = a / b; }
prt_inline vfloat8& operator &=(vfloat8& a, const vfloat8& b) { return a = a & b; }
prt_inline vfloat8& operator |=(vfloat8& a, const vfloat8& b) { return a = a | b; }
prt_inline vfloat8& operator ^=(vfloat8& a, const vfloat8& b) { return a = a ^ b; }

// Compare operators
// -----------------

prt_inline vbool8 operator ==(const vfloat8& a, const vfloat8& b)
{
  return _mm256_cmp_ps(a.m, b.m, _CMP_EQ_OQ);
}

prt_inline vbool8 operator !=(const vfloat8& a, const vfloat8& b)
{
  return _mm256_cmp_ps(a.m, b.m, _CMP_NEQ_UQ);
}

prt_inline vbool8 operator <(const vfloat8& a, const vfloat8& b)
{
  return _mm256_cmp_ps(a.m, b.m, _CMP_LT_OS);
}

prt_inline vbool8 operator >(const vfloat8& a, const vfloat8& b)
{
  return _mm256_cmp_ps(a.m, b.m, _CMP_GT_OS);
}

prt_inline vbool8 operator <=(const vfloat8& a, const vfloat8& b)
{
  return _mm256_cmp_ps(a.m, b.m, _CMP_LE_OS);
}

prt_inline vbool8 operator >=(const vfloat8& a, const vfloat8& b)
{
  return _mm256_cmp_ps(a.m, b.m, _CMP_GE_OS);
}

// Compare functions
// -----------------

prt_inline vbool8 cmpEq(const vfloat8& a, const vfloat8& b) { return a == b; }
prt_inline vbool8 cmpNe(const vfloat8& a, const vfloat8& b) { return a != b; }
prt_inline vbool8 cmpLt(const vfloat8& a, const vfloat8& b) { return a <  b; }
prt_inline vbool8 cmpLe(const vfloat8& a, const vfloat8& b) { return a <= b; }
prt_inline vbool8 cmpGt(const vfloat8& a, const vfloat8& b) { return a >  b; }
prt_inline vbool8 cmpGe(const vfloat8& a, const vfloat8& b) { return a >= b; }

prt_inline vbool8 cmpEq(const vbool8& mask, const vfloat8& a, const vfloat8& b) { return (a == b) & mask; }
prt_inline vbool8 cmpNe(const vbool8& mask, const vfloat8& a, const vfloat8& b) { return (a != b) & mask; }
prt_inline vbool8 cmpLt(const vbool8& mask, const vfloat8& a, const vfloat8& b) { return (a <  b) & mask; }
prt_inline vbool8 cmpLe(const vbool8& mask, const vfloat8& a, const vfloat8& b) { return (a <= b) & mask; }
prt_inline vbool8 cmpGt(const vbool8& mask, const vfloat8& a, const vfloat8& b) { return (a >  b) & mask; }
prt_inline vbool8 cmpGe(const vbool8& mask, const vfloat8& a, const vfloat8& b) { return (a >= b) & mask; }

// Math functions
// --------------

prt_inline vfloat8 abs(const vfloat8& a)
{
  return _mm256_and_ps(a.m, _mm256_castsi256_ps(_mm256_set1_epi32(0x7fffffff)));
}

prt_inline vfloat8 rcp(const vfloat8& a)
{
  __m256 r = _mm256_rcp_ps(a.m);
  return _mm256_sub_ps(_mm256_add_ps(r, r), _mm256_mul_ps(_mm256_mul_ps(r, r), a.m));
}

prt_inline vfloat8 sqrt(const vfloat8& a)
{
  return _mm256_sqrt_ps(a.m);
}

prt_inline vfloat8 rsqrt(const vfloat8& a)
{
  __m256 r = _mm256_rsqrt_ps(a.m);
  return _mm256_add_ps(_mm256_mul_ps(_mm256_set1_ps(1.5f), r), _mm256_mul_ps(_mm256_mul_ps(_mm256_mul_ps(a.m, _mm256_set1_ps(-0.5f)), r), _mm256_mul_ps(r, r)));
}

prt_inline vfloat8 floor(const vfloat8& a)
{
  return _mm256_floor_ps(a.m);
}

prt_inline vfloat8 ceil(const vfloat8& a)
{
  return _mm256_ceil_ps(a.m);
}

prt_inline vfloat8 round(const vfloat8& a)
{
  return _mm256_round_ps(a.m, _MM_FROUND_TO_NEAREST_INT);
}

prt_inline vfloat8 min(const vfloat8& a, const vfloat8& b)
{
  return _mm256_min_ps(a.m, b.m);
}

prt_inline vfloat8 max(const vfloat8& a, const vfloat8& b)
{
  return _mm256_max_ps(a.m, b.m);
}

prt_inline vfloat8 vdot3(const vfloat8& a, const vfloat8& b)
{
  return _mm256_dp_ps(a.m, b.m, 0x7f);
}

prt_inline vfloat8 vdot4(const vfloat8& a, const vfloat8& b)
{
  return _mm256_dp_ps(a.m, b.m, 0xff);
}

prt_inline vfloat8 vcross3(const vfloat8& a, const vfloat8& b)
{
  __m256 p2 = _mm256_mul_ps(b.m, _mm256_permute_ps(a.m, _MM_SHUFFLE(3, 0, 2, 1)));
  __m256 s = _mm256_sub_ps(_mm256_mul_ps(a.m, _mm256_permute_ps(b.m, _MM_SHUFFLE(3, 0, 2, 1))), p2);
  return _mm256_permute_ps(s, _MM_SHUFFLE(3, 0, 2, 1));
}

// Reduction functions
// -------------------

prt_inline vfloat8 vreduceMin(const vfloat8& a)
{
  __m256 temp = _mm256_min_ps(a.m, _mm256_permute2f128_ps(a.m, a.m, 0x21));
  temp = _mm256_min_ps(temp, _mm256_permute_ps(temp, _MM_SHUFFLE(1, 0, 3, 2)));
  temp = _mm256_min_ps(temp, _mm256_permute_ps(temp, _MM_SHUFFLE(2, 3, 0, 1)));
  return temp;
}

prt_inline vfloat8 vreduceMax(const vfloat8& a)
{
  __m256 temp = _mm256_max_ps(a.m, _mm256_permute2f128_ps(a.m, a.m, 0x21));
  temp = _mm256_max_ps(temp, _mm256_permute_ps(temp, _MM_SHUFFLE(1, 0, 3, 2)));
  temp = _mm256_max_ps(temp, _mm256_permute_ps(temp, _MM_SHUFFLE(2, 3, 0, 1)));
  return temp;
}

prt_inline vfloat8 vreduceAdd(const vfloat8& a)
{
  __m256 temp = _mm256_add_ps(a.m, _mm256_permute2f128_ps(a.m, a.m, 0x21));
  temp = _mm256_add_ps(temp, _mm256_permute_ps(temp, _MM_SHUFFLE(1, 0, 3, 2)));
  temp = _mm256_add_ps(temp, _mm256_permute_ps(temp, _MM_SHUFFLE(2, 3, 0, 1)));
  return temp;
}

prt_inline float reduceMin(const vfloat8& a) { return toScalar(vreduceMin(a)); }
prt_inline float reduceMax(const vfloat8& a) { return toScalar(vreduceMax(a)); }
prt_inline float reduceAdd(const vfloat8& a) { return toScalar(vreduceAdd(a)); }

prt_inline int selectMin(const vfloat8& a) { return bitScan(toIntMask(a == vreduceMin(a))); }
prt_inline int selectMax(const vfloat8& a) { return bitScan(toIntMask(a == vreduceMax(a))); }
prt_inline int selectAdd(const vfloat8& a) { return bitScan(toIntMask(a == vreduceAdd(a))); }

// Misc functions
// --------------

prt_inline vfloat8 sort(const vfloat8& a)
{
  // Compare 1
  __m128 v0 = _mm256_castps256_ps128(a.m);
  __m128 v1 = _mm256_extractf128_ps(a.m, 1);

  __m128 temp = v0;
  v0 = _mm_min_ps(v0, v1);
  v1 = _mm_max_ps(temp, v1);

  // Compare 2
  __m128 v2 = _mm_movelh_ps(v0, v1);
  __m128 v3 = _mm_movehl_ps(v1, v0);

  temp = v2;
  v2 = _mm_min_ps(v2, v3);
  v3 = _mm_max_ps(temp, v3);

  // Compare 3
  v0 = v3;
  v1 = _mm_movehl_ps(v2, v2);

  temp = v0;
  v0 = _mm_min_ps(v0, v1);
  v1 = _mm_max_ps(temp, v1);

  // Compare 4
  v0 = _mm_movelh_ps(v2, v0);
  v1 = _mm_blend_ps(v1, v3, 0xc); // 1, 1, 0, 0
  v2 = _mm_shuffle_ps(v0, v1, _MM_SHUFFLE(2, 0, 2, 0));
  v3 = _mm_shuffle_ps(v0, v1, _MM_SHUFFLE(3, 1, 3, 1));

  temp = v2;
  v2 = _mm_min_ps(v2, v3);
  v3 = _mm_max_ps(temp, v3);

  // Compare 5
  v0 = v3;
  v1 = _mm_movehl_ps(v2, v2);

  temp = v0;
  v0 = _mm_min_ps(v0, v1);
  v1 = _mm_max_ps(temp, v1);

  // Compare 6
  v0 = _mm_blend_ps(v0, v3, 0xc); // 1, 1, 0, 0
  v1 = _mm_movelh_ps(v1, v2);
  v1 = _mm_permute_ps(v1, _MM_SHUFFLE(2, 1, 0, 3));

  temp = v0;
  v0 = _mm_min_ps(v0, v1);
  v1 = _mm_max_ps(temp, v1);

  // Gather
  v3 = _mm_blend_ps(v0, v3, 0x8); // 1, 0, 0, 0
  v2 = _mm_unpacklo_ps(v2, v1);
  v1 = _mm_shuffle_ps(v1, v3, _MM_SHUFFLE(3, 2, 2, 1));
  v0 = _mm_unpacklo_ps(v2, v0);
  v1 = _mm_permute_ps(v1, _MM_SHUFFLE(3, 1, 2, 0));
  __m256 result = _mm256_insertf128_ps(_mm256_castps128_ps256(v0), v1, 1);
  return result;
}

// Reverse sort
prt_inline vfloat8 rsort(const vfloat8& a)
{
  // Compare 1
  __m128 v0 = _mm256_castps256_ps128(a.m);
  __m128 v1 = _mm256_extractf128_ps(a.m, 1);

  __m128 temp = v0;
  v0 = _mm_max_ps(v0, v1);
  v1 = _mm_min_ps(temp, v1);

  // Compare 2
  __m128 v2 = _mm_movelh_ps(v0, v1);
  __m128 v3 = _mm_movehl_ps(v1, v0);

  temp = v2;
  v2 = _mm_max_ps(v2, v3);
  v3 = _mm_min_ps(temp, v3);

  // Compare 3
  v0 = v3;
  v1 = _mm_movehl_ps(v2, v2);

  temp = v0;
  v0 = _mm_max_ps(v0, v1);
  v1 = _mm_min_ps(temp, v1);

  // Compare 4
  v0 = _mm_movelh_ps(v2, v0);
  v1 = _mm_blend_ps(v1, v3, 0xc); // 1, 1, 0, 0
  v2 = _mm_shuffle_ps(v0, v1, _MM_SHUFFLE(2, 0, 2, 0));
  v3 = _mm_shuffle_ps(v0, v1, _MM_SHUFFLE(3, 1, 3, 1));

  temp = v2;
  v2 = _mm_max_ps(v2, v3);
  v3 = _mm_min_ps(temp, v3);

  // Compare 5
  v0 = v3;
  v1 = _mm_movehl_ps(v2, v2);

  temp = v0;
  v0 = _mm_max_ps(v0, v1);
  v1 = _mm_min_ps(temp, v1);

  // Compare 6
  v0 = _mm_blend_ps(v0, v3, 0xc); // 1, 1, 0, 0
  v1 = _mm_movelh_ps(v1, v2);
  v1 = _mm_permute_ps(v1, _MM_SHUFFLE(2, 1, 0, 3));

  temp = v0;
  v0 = _mm_max_ps(v0, v1);
  v1 = _mm_min_ps(temp, v1);

  // Gather
  v3 = _mm_blend_ps(v0, v3, 0x8); // 1, 0, 0, 0
  v2 = _mm_unpacklo_ps(v2, v1);
  v1 = _mm_shuffle_ps(v1, v3, _MM_SHUFFLE(3, 2, 2, 1));
  v0 = _mm_unpacklo_ps(v2, v0);
  v1 = _mm_permute_ps(v1, _MM_SHUFFLE(3, 1, 2, 0));
  __m256 result = _mm256_insertf128_ps(_mm256_castps128_ps256(v0), v1, 1);
  return result;
}

// Approximation! Does not use full precision
prt_inline vint8 sortOrderFast(const vfloat8& a)
{
  const vfloat8 orderMask = asFloat(vint8(7));
  vfloat8 x = andn(a, orderMask) | asFloat(vint8(0, 1, 2, 3, 4, 5, 6, 7));
  return asInt(sort(x) & orderMask);
}

// Approximation! Does not use full precision
prt_inline vint8 rsortOrderFast(const vfloat8& a)
{
  const vfloat8 orderMask = asFloat(vint8(7));
  vfloat8 x = andn(a, orderMask) | asFloat(vint8(0, 1, 2, 3, 4, 5, 6, 7));
  return asInt(rsort(x) & orderMask);
}

prt_inline void transpose(const vfloat4& x0, const vfloat4& x1, const vfloat4& x2, const vfloat4& x3,
                          const vfloat4& x4, const vfloat4& x5, const vfloat4& x6, const vfloat4& x7,
                          vfloat8& y0, vfloat8& y1, vfloat8& y2)
{
  __m256 t0, t1, ta, tb, tc, td;

  t0 = _mm256_insertf128_ps(_mm256_castps128_ps256(x0.m), x4.m, 1);
  t1 = _mm256_insertf128_ps(_mm256_castps128_ps256(x1.m), x5.m, 1);
  ta = _mm256_castpd_ps(_mm256_unpacklo_pd(_mm256_castps_pd(t0), _mm256_castps_pd(t1)));
  tb = _mm256_castpd_ps(_mm256_unpackhi_pd(_mm256_castps_pd(t0), _mm256_castps_pd(t1)));

  t0 = _mm256_insertf128_ps(_mm256_castps128_ps256(x2.m), x6.m, 1);
  t1 = _mm256_insertf128_ps(_mm256_castps128_ps256(x3.m), x7.m, 1);
  tc = _mm256_castpd_ps(_mm256_unpacklo_pd(_mm256_castps_pd(t0), _mm256_castps_pd(t1)));
  td = _mm256_castpd_ps(_mm256_unpackhi_pd(_mm256_castps_pd(t0), _mm256_castps_pd(t1)));

  y0 = _mm256_shuffle_ps(ta, tc, 0x88);
  y1 = _mm256_shuffle_ps(ta, tc, 0xDD);
  y2 = _mm256_shuffle_ps(tb, td, 0x88);
}

} // namespace prt
