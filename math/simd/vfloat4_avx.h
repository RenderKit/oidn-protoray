// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <immintrin.h>
#include "../simd_common.h"
#include "vbool4_avx.h"
#include "vint4_avx.h"
#include "vuint4_avx.h"

namespace prt {

// vfloat4
template <>
struct var<float,4>
{
  union
  {
    __m128 m;
    float v[4];
  };

  prt_inline var() {}

  prt_inline var(const vfloat4& a) : m(a.m) {}
  prt_inline var(__m128 a) : m(a) {}
  prt_inline var(float a) : m(_mm_set1_ps(a)) {}
  prt_inline var(float a0, float a1, float a2, float a3) : m(_mm_set_ps(a3, a2, a1, a0)) {}

  prt_inline var(Zero)   : m(_mm_setzero_ps()) {}
  prt_inline var(One)    : m(_mm_set1_ps(1.0f)) {}
  prt_inline var(PosMax) : m(_mm_set1_ps(posMax)) {}
  prt_inline var(NegMax) : m(_mm_set1_ps(negMax)) {}
  prt_inline var(PosInf) : m(_mm_set1_ps(posInf)) {}
  prt_inline var(NegInf) : m(_mm_set1_ps(negInf)) {}
  prt_inline var(Qnan)   : m(_mm_set1_ps(qnan)) {}
  prt_inline var(Step)   : m(_mm_set_ps(3.0f, 2.0f, 1.0f, 0.0f)) {}

  prt_inline vfloat4& operator =(const vfloat4& a)
  {
    m = a.m;
    return *this;
  }

  prt_inline const float& operator [](size_t i) const
  {
    assert(i < 4);
    return v[i];
  }

  prt_inline float& operator [](size_t i)
  {
    assert(i < 4);
    return v[i];
  }
};

// Load functions
// --------------

template <>
prt_inline vfloat4 load(const float* ptr)
{
  return _mm_load_ps(ptr);
}

template <>
prt_inline vfloat4 load(const vbool4& mask, const float* ptr)
{
  return _mm_maskload_ps(ptr, _mm_castps_si128(mask.m));
}

prt_inline vfloat4 gather(const float* ptr, const vint4& idx)
{
  vfloat4 r;
  for (int i = 0; i < 4; ++i)
    r[i] = ptr[idx[i]];
  return r;
}

prt_inline vfloat4 gather(const vbool4& mask, const float* ptr, const vint4& idx)
{
  vfloat4 r;
  for (int i = 0; i < 4; ++i)
    r[i] = mask[i] ? ptr[idx[i]] : 0.0f;
  return r;
}

// Store functions
// ---------------

prt_inline void store(float* ptr, const vfloat4& a)
{
  _mm_store_ps(ptr, a.m);
}

prt_inline void store(const vbool4& mask, float* ptr, const vfloat4& a)
{
  _mm_maskstore_ps(ptr, _mm_castps_si128(mask.m), a.m);
}

prt_inline void compress(const vbool4& mask, float* ptr, const vfloat4& a)
{
  int offset = 0;
  for (int i = 0; i < 4; ++i)
    if (mask[i]) ptr[offset++] = a[i];
}

prt_inline void scatter(float* ptr, const vint4& idx, const vfloat4& a)
{
  for (int i = 0; i < 4; ++i)
    ptr[idx[i]] = a[i];
}

prt_inline void scatter(const vbool4& mask, float* ptr, const vint4& idx, const vfloat4& a)
{
  for (int i = 0; i < 4; ++i)
    if (mask[i]) ptr[idx[i]] = a[i];
}

// Conversion functions
// --------------------

prt_inline float toScalar(const vfloat4& a)
{
  return _mm_cvtss_f32(a.m);
}

prt_inline vint4 toInt(const vfloat4& a)
{
  return _mm_cvttps_epi32(a.m);
}

prt_inline vint4 asInt(const vfloat4& a)
{
  return _mm_castps_si128(a.m);
}

prt_inline vuint4 asUInt(const vfloat4& a)
{
  return _mm_castps_si128(a.m);
}

prt_inline vfloat4 toFloat(const vint4& a)
{
  return _mm_cvtepi32_ps(a.m);
}

prt_inline vfloat4 toFloat(const vuint4& x)
{
  const __m128i a  = _mm_and_si128(x.m, _mm_set1_epi32(0x7FFFFFFF));
  const __m128i b  = _mm_and_si128(_mm_srai_epi32(x.m, 31), _mm_set1_epi32(0x4F000000)); // 0x4F000000 = 2^31
  const __m128  af = _mm_cvtepi32_ps(a);
  const __m128  bf = _mm_castsi128_ps(b);
  return _mm_add_ps(af, bf);
}

prt_inline vfloat4 asFloat(const vint4& a)
{
  return _mm_castsi128_ps(a.m);
}

// FIXME
prt_inline int toIntMask(const vfloat4& a)
{
  return _mm_movemask_ps(a.m);
}

// Select function
// ---------------

prt_inline vfloat4 select(const vbool4& mask, const vfloat4& a, const vfloat4& b)
{
  return _mm_blendv_ps(b.m, a.m, mask.m);
}

// Shuffle functions
// -----------------

template <int i>
prt_inline float extract(const vfloat4& a);

template <>
prt_inline float extract<0>(const vfloat4& a)
{
  return _mm_cvtss_f32(a.m);
}

template <int i>
prt_inline float extract(const vfloat4& a)
{
  return _mm_cvtss_f32(_mm_shuffle_ps(a.m, a.m, _MM_SHUFFLE(i, i, i, i)));
}

template <int i>
prt_inline vfloat4 broadcast(const vfloat4& a)
{
  return _mm_shuffle_ps(a.m, a.m, _MM_SHUFFLE(i, i, i, i));
}

template <int i0, int i1, int i2, int i3>
prt_inline vfloat4 permute4(const vfloat4& a)
{
  return _mm_shuffle_ps(a.m, a.m, _MM_SHUFFLE(i3, i2, i1, i0));
}

// Arithmetic operators
// --------------------

prt_inline vfloat4 operator +(const vfloat4& a)
{
  return a;
}

prt_inline vfloat4 operator -(const vfloat4& a)
{
  return _mm_xor_ps(a.m, _mm_castsi128_ps(_mm_set1_epi32(0x80000000)));
}

prt_inline vfloat4 operator +(const vfloat4& a, const vfloat4& b)
{
  return _mm_add_ps(a.m, b.m);
}

prt_inline vfloat4 operator -(const vfloat4& a, const vfloat4& b)
{
  return _mm_sub_ps(a.m, b.m);
}

prt_inline vfloat4 operator *(const vfloat4& a, const vfloat4& b)
{
  return _mm_mul_ps(a.m, b.m);
}

prt_inline vfloat4 operator /(const vfloat4& a, const vfloat4& b)
{
  //return _mm_div_ps(a.m, b.m);
  __m128 r = _mm_rcp_ps(b.m);
  r = _mm_sub_ps(_mm_add_ps(r, r), _mm_mul_ps(_mm_mul_ps(r, r), b.m));
  return _mm_mul_ps(a.m, r);
}

// Bitwise operators
// -----------------

prt_inline vfloat4 operator &(const vfloat4& a, const vfloat4& b)
{
  return _mm_and_ps(a.m, b.m);
}

prt_inline vfloat4 operator |(const vfloat4& a, const vfloat4& b)
{
  return _mm_or_ps(a.m, b.m);
}

prt_inline vfloat4 operator ^(const vfloat4& a, const vfloat4& b)
{
  return _mm_xor_ps(a.m, b.m);
}

prt_inline vfloat4 andn(const vfloat4& a, const vfloat4& b)
{
  return _mm_andnot_ps(b.m, a.m);
}

// Assignment operators
// --------------------

prt_inline vfloat4& operator +=(vfloat4& a, const vfloat4& b) { return a = a + b; }
prt_inline vfloat4& operator -=(vfloat4& a, const vfloat4& b) { return a = a - b; }
prt_inline vfloat4& operator *=(vfloat4& a, const vfloat4& b) { return a = a * b; }
prt_inline vfloat4& operator /=(vfloat4& a, const vfloat4& b) { return a = a / b; }
prt_inline vfloat4& operator &=(vfloat4& a, const vfloat4& b) { return a = a & b; }
prt_inline vfloat4& operator |=(vfloat4& a, const vfloat4& b) { return a = a | b; }
prt_inline vfloat4& operator ^=(vfloat4& a, const vfloat4& b) { return a = a ^ b; }

// Compare operators
// -----------------

prt_inline vbool4 operator ==(const vfloat4& a, const vfloat4& b)
{
  return _mm_cmpeq_ps(a.m, b.m);
}

prt_inline vbool4 operator !=(const vfloat4& a, const vfloat4& b)
{
  return _mm_cmpneq_ps(a.m, b.m);
}

prt_inline vbool4 operator <(const vfloat4& a, const vfloat4& b)
{
  return _mm_cmplt_ps(a.m, b.m);
}

prt_inline vbool4 operator >(const vfloat4& a, const vfloat4& b)
{
  return _mm_cmpgt_ps(a.m, b.m);
}

prt_inline vbool4 operator <=(const vfloat4& a, const vfloat4& b)
{
  return _mm_cmple_ps(a.m, b.m);
}

prt_inline vbool4 operator >=(const vfloat4& a, const vfloat4& b)
{
  return _mm_cmpge_ps(a.m, b.m);
}

// Compare functions
// -----------------

prt_inline vbool4 cmpEq(const vfloat4& a, const vfloat4& b) { return a == b; }
prt_inline vbool4 cmpNe(const vfloat4& a, const vfloat4& b) { return a != b; }
prt_inline vbool4 cmpLt(const vfloat4& a, const vfloat4& b) { return a <  b; }
prt_inline vbool4 cmpLe(const vfloat4& a, const vfloat4& b) { return a <= b; }
prt_inline vbool4 cmpGt(const vfloat4& a, const vfloat4& b) { return a >  b; }
prt_inline vbool4 cmpGe(const vfloat4& a, const vfloat4& b) { return a >= b; }

prt_inline vbool4 cmpEq(const vbool4& mask, const vfloat4& a, const vfloat4& b) { return (a == b) & mask; }
prt_inline vbool4 cmpNe(const vbool4& mask, const vfloat4& a, const vfloat4& b) { return (a != b) & mask; }
prt_inline vbool4 cmpLt(const vbool4& mask, const vfloat4& a, const vfloat4& b) { return (a <  b) & mask; }
prt_inline vbool4 cmpLe(const vbool4& mask, const vfloat4& a, const vfloat4& b) { return (a <= b) & mask; }
prt_inline vbool4 cmpGt(const vbool4& mask, const vfloat4& a, const vfloat4& b) { return (a >  b) & mask; }
prt_inline vbool4 cmpGe(const vbool4& mask, const vfloat4& a, const vfloat4& b) { return (a >= b) & mask; }

// Math functions
// --------------

prt_inline vfloat4 abs(const vfloat4& a)
{
  return _mm_and_ps(a.m, _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff)));
}

prt_inline vfloat4 rcp(const vfloat4& a)
{
  __m128 r = _mm_rcp_ps(a.m);
  return _mm_sub_ps(_mm_add_ps(r, r), _mm_mul_ps(_mm_mul_ps(r, r), a.m));
}

prt_inline vfloat4 sqrt(const vfloat4& a)
{
  return _mm_sqrt_ps(a.m);
}

prt_inline vfloat4 rsqrt(const vfloat4& a)
{
  __m128 r = _mm_rsqrt_ps(a.m);
  return _mm_add_ps(_mm_mul_ps(_mm_set1_ps(1.5f), r), _mm_mul_ps(_mm_mul_ps(_mm_mul_ps(a.m, _mm_set1_ps(-0.5f)), r), _mm_mul_ps(r, r)));
}

prt_inline vfloat4 floor(const vfloat4& a)
{
  return _mm_floor_ps(a.m);
}

prt_inline vfloat4 ceil(const vfloat4& a)
{
  return _mm_ceil_ps(a.m);
}

prt_inline vfloat4 round(const vfloat4& a)
{
  return _mm_round_ps(a.m, _MM_FROUND_TO_NEAREST_INT);
}

prt_inline vfloat4 min(const vfloat4& a, const vfloat4& b)
{
  return _mm_min_ps(a.m, b.m);
}

prt_inline vfloat4 max(const vfloat4& a, const vfloat4& b)
{
  return _mm_max_ps(a.m, b.m);
}

prt_inline vfloat4 vdot3(const vfloat4& a, const vfloat4& b)
{
  return _mm_dp_ps(a.m, b.m, 0x7f);
}

prt_inline vfloat4 vdot4(const vfloat4& a, const vfloat4& b)
{
  return _mm_dp_ps(a.m, b.m, 0xff);
}

prt_inline vfloat4 vcross3(const vfloat4& a, const vfloat4& b)
{
  __m128 p2 = _mm_mul_ps(b.m, _mm_shuffle_ps(a.m, a.m, _MM_SHUFFLE(3, 0, 2, 1)));
  __m128 s = _mm_sub_ps(_mm_mul_ps(a.m, _mm_shuffle_ps(b.m, b.m, _MM_SHUFFLE(3, 0, 2, 1))), p2);
  return _mm_shuffle_ps(s, s, _MM_SHUFFLE(3, 0, 2, 1));
}

// Reduction functions
// -------------------

prt_inline vfloat4 vreduceMin(const vfloat4& a)
{
  __m128 temp = _mm_min_ps(a.m, _mm_shuffle_ps(a.m, a.m, _MM_SHUFFLE(1, 0, 3, 2)));
  temp = _mm_min_ps(temp, _mm_shuffle_ps(temp, temp, _MM_SHUFFLE(2, 3, 0, 1)));
  return temp;
}

prt_inline vfloat4 vreduceMax(const vfloat4& a)
{
  __m128 temp = _mm_max_ps(a.m, _mm_shuffle_ps(a.m, a.m, _MM_SHUFFLE(1, 0, 3, 2)));
  temp = _mm_max_ps(temp, _mm_shuffle_ps(temp, temp, _MM_SHUFFLE(2, 3, 0, 1)));
  return temp;
}

prt_inline vfloat4 vreduceAdd(const vfloat4& a)
{
  __m128 temp = _mm_add_ps(a.m, _mm_shuffle_ps(a.m, a.m, _MM_SHUFFLE(1, 0, 3, 2)));
  temp = _mm_add_ps(temp, _mm_shuffle_ps(temp, temp, _MM_SHUFFLE(2, 3, 0, 1)));
  return temp;
}

prt_inline float reduceMin(const vfloat4& a) { return toScalar(vreduceMin(a)); }
prt_inline float reduceMax(const vfloat4& a) { return toScalar(vreduceMax(a)); }
prt_inline float reduceAdd(const vfloat4& a) { return toScalar(vreduceAdd(a)); }

prt_inline int selectMin(const vfloat4& a) { return bitScan(toIntMask(a == vreduceMin(a))); }
prt_inline int selectMax(const vfloat4& a) { return bitScan(toIntMask(a == vreduceMax(a))); }
prt_inline int selectAdd(const vfloat4& a) { return bitScan(toIntMask(a == vreduceAdd(a))); }

// Misc functions
// --------------

prt_inline vfloat4 sort(const vfloat4& a)
{
  // Compare 1
  __m128 v0 = a.m;
  __m128 v1 = _mm_setzero_ps();
  v1 = _mm_movelh_ps(v1, v0);

  __m128 temp1 = v0;
  v0 = _mm_max_ps(v0, v1);
  v1 = _mm_min_ps(v1, temp1);

  // Compare 2
  v0 = _mm_movehl_ps(v0, v1);
  v1 = _mm_shuffle_ps(v1, v0, 0x88);

  __m128 temp2 = v0;
  v0 = _mm_max_ps(v0, v1);
  v1 = _mm_min_ps(v1, temp2);

  // Compare 3
  __m128 v2 = _mm_setzero_ps();
  v2 = _mm_movehl_ps(v2, v1);
  __m128 v3 = v0;

  __m128 temp3 = v2;
  v2 = _mm_max_ps(v2, v3);
  v3 = _mm_min_ps(v3, temp3);

  // Gather
  v1 = _mm_movelh_ps(v1, v3);
  v0 = _mm_shuffle_ps(v0, v2, 0x13);
  v1 = _mm_shuffle_ps(v1, v0, 0x2d);
  return v1;
}

// Reverse sort
prt_inline vfloat4 rsort(const vfloat4& a)
{
  __m128 v0 = a.m;
  __m128 v1 = _mm_setzero_ps();
  v1 = _mm_movelh_ps(v1, v0);

  // Compare 1
  __m128 temp1 = v0;
  v0 = _mm_min_ps(v0, v1);
  v1 = _mm_max_ps(v1, temp1);

  v0 = _mm_movehl_ps(v0, v1);
  v1 = _mm_shuffle_ps(v1, v0, 0x88);

  // Compare 2
  __m128 temp2 = v0;
  v0 = _mm_min_ps(v0, v1);
  v1 = _mm_max_ps(v1, temp2);

  __m128 v2 = _mm_setzero_ps();
  v2 = _mm_movehl_ps(v2, v1);
  __m128 v3 = v0;

  // Compare 3
  __m128 temp3 = v2;
  v2 = _mm_min_ps(v2, v3);
  v3 = _mm_max_ps(v3, temp3);

  v1 = _mm_movelh_ps(v1, v3);
  v0 = _mm_shuffle_ps(v0, v2, 0x13);
  v1 = _mm_shuffle_ps(v1, v0, 0x2d);
  return v1;
}

// Approximation! Does not use full precision
prt_inline vint4 sortOrderFast(const vfloat4& a)
{
  const vfloat4 orderMask = asFloat(vint4(3));
  vfloat4 x = andn(a, orderMask) | asFloat(vint4(0, 1, 2, 3));
  return asInt(sort(x) & orderMask);
}

// Approximation! Does not use full precision
prt_inline vint4 rsortOrderFast(const vfloat4& a)
{
  const vfloat4 orderMask = asFloat(vint4(3));
  vfloat4 x = andn(a, orderMask) | asFloat(vint4(0, 1, 2, 3));
  return asInt(rsort(x) & orderMask);
}

prt_inline void transpose(const vfloat4& x0, const vfloat4& x1, const vfloat4& x2, const vfloat4& x3,
                           vfloat4& y0, vfloat4& y1, vfloat4& y2)
{
  __m128 ab0 = _mm_unpacklo_ps(x0.m, x2.m); // a0 a2 b0 b2
  __m128 ab1 = _mm_unpacklo_ps(x1.m, x3.m); // a1 a3 b1 b3

  y0 = _mm_unpacklo_ps(ab0, ab1); // a0 a1 a2 a3
  y1 = _mm_unpackhi_ps(ab0, ab1); // b0 b1 b2 b3

  __m128 cd0 = _mm_unpackhi_ps(x0.m, x2.m); // c0 c2 d0 d2
  __m128 cd1 = _mm_unpackhi_ps(x1.m, x3.m); // c1 c3 d1 d3

  y2 = _mm_unpacklo_ps(cd0, cd1); // c0 c1 c2 c3
}

} // namespace prt
