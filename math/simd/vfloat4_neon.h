// Copyright 2026 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <arm_neon.h>
#include "../simd_common.h"
#include "vbool4_neon.h"
#include "vint4_neon.h"
#include "vuint4_neon.h"

namespace prt {

// vfloat4
template <>
struct var<float,4>
{
  union
  {
    float32x4_t m;
    float v[4];
  };

  prt_inline var() {}

  prt_inline var(const vfloat4& a) : m(a.m) {}
  prt_inline var(float32x4_t a) : m(a) {}
  prt_inline var(float a) : m(vdupq_n_f32(a)) {}
  prt_inline var(float a0, float a1, float a2, float a3) { float d[4] = {a0,a1,a2,a3}; m = vld1q_f32(d); }

  prt_inline var(Zero)   : m(vdupq_n_f32(0.0f)) {}
  prt_inline var(One)    : m(vdupq_n_f32(1.0f)) {}
  prt_inline var(PosMax) : m(vdupq_n_f32(posMax)) {}
  prt_inline var(NegMax) : m(vdupq_n_f32(negMax)) {}
  prt_inline var(PosInf) : m(vdupq_n_f32(posInf)) {}
  prt_inline var(NegInf) : m(vdupq_n_f32(negInf)) {}
  prt_inline var(Qnan)   : m(vdupq_n_f32(qnan)) {}
  prt_inline var(Step)   { float d[4] = {0.0f,1.0f,2.0f,3.0f}; m = vld1q_f32(d); }

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
  return vld1q_f32(ptr);
}

template <>
prt_inline vfloat4 load(const vbool4& mask, const float* ptr)
{
  vfloat4 r;
  for (int i = 0; i < 4; ++i)
    r[i] = mask[i] ? ptr[i] : 0.0f;
  return r;
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
  vst1q_f32(ptr, a.m);
}

prt_inline void store(const vbool4& mask, float* ptr, const vfloat4& a)
{
  for (int i = 0; i < 4; ++i)
    if (mask[i]) ptr[i] = a[i];
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
  return vgetq_lane_f32(a.m, 0);
}

prt_inline vint4 toInt(const vfloat4& a)
{
  return vcvtq_s32_f32(a.m);
}

prt_inline vint4 asInt(const vfloat4& a)
{
  return vreinterpretq_s32_f32(a.m);
}

prt_inline vuint4 asUInt(const vfloat4& a)
{
  return vreinterpretq_u32_f32(a.m);
}

prt_inline vfloat4 toFloat(const vint4& a)
{
  return vcvtq_f32_s32(a.m);
}

prt_inline vfloat4 toFloat(const vuint4& a)
{
  return vcvtq_f32_u32(a.m);
}

prt_inline vfloat4 asFloat(const vint4& a)
{
  return vreinterpretq_f32_s32(a.m);
}

prt_inline int toIntMask(const vfloat4& a)
{
  return toIntMask(vbool4(vreinterpretq_u32_f32(a.m)));
}

// Select function
// ---------------

prt_inline vfloat4 select(const vbool4& mask, const vfloat4& a, const vfloat4& b)
{
  return vbslq_f32(mask.m, a.m, b.m);
}

// Shuffle functions
// -----------------

template <int i>
prt_inline float extract(const vfloat4& a)
{
  return vgetq_lane_f32(a.m, i);
}

template <int i>
prt_inline vfloat4 broadcast(const vfloat4& a)
{
  return vdupq_laneq_f32(a.m, i);
}

template <int i0, int i1, int i2, int i3>
prt_inline vfloat4 permute4(const vfloat4& a)
{
  vfloat4 r;
  r[0] = a[i0]; r[1] = a[i1]; r[2] = a[i2]; r[3] = a[i3];
  return r;
}

// Arithmetic operators
// --------------------

prt_inline vfloat4 operator +(const vfloat4& a)
{
  return a;
}

prt_inline vfloat4 operator -(const vfloat4& a)
{
  return vnegq_f32(a.m);
}

prt_inline vfloat4 operator +(const vfloat4& a, const vfloat4& b)
{
  return vaddq_f32(a.m, b.m);
}

prt_inline vfloat4 operator -(const vfloat4& a, const vfloat4& b)
{
  return vsubq_f32(a.m, b.m);
}

prt_inline vfloat4 operator *(const vfloat4& a, const vfloat4& b)
{
  return vmulq_f32(a.m, b.m);
}

prt_inline vfloat4 operator /(const vfloat4& a, const vfloat4& b)
{
  return vdivq_f32(a.m, b.m);
}

// Bitwise operators
// -----------------

prt_inline vfloat4 operator &(const vfloat4& a, const vfloat4& b)
{
  return vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(a.m), vreinterpretq_u32_f32(b.m)));
}

prt_inline vfloat4 operator |(const vfloat4& a, const vfloat4& b)
{
  return vreinterpretq_f32_u32(vorrq_u32(vreinterpretq_u32_f32(a.m), vreinterpretq_u32_f32(b.m)));
}

prt_inline vfloat4 operator ^(const vfloat4& a, const vfloat4& b)
{
  return vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(a.m), vreinterpretq_u32_f32(b.m)));
}

prt_inline vfloat4 andn(const vfloat4& a, const vfloat4& b)
{
  return vreinterpretq_f32_u32(vbicq_u32(vreinterpretq_u32_f32(a.m), vreinterpretq_u32_f32(b.m))); // a & ~b
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
  return vceqq_f32(a.m, b.m);
}

prt_inline vbool4 operator !=(const vfloat4& a, const vfloat4& b)
{
  return vmvnq_u32(vceqq_f32(a.m, b.m));
}

prt_inline vbool4 operator <(const vfloat4& a, const vfloat4& b)
{
  return vcltq_f32(a.m, b.m);
}

prt_inline vbool4 operator >(const vfloat4& a, const vfloat4& b)
{
  return vcgtq_f32(a.m, b.m);
}

prt_inline vbool4 operator <=(const vfloat4& a, const vfloat4& b)
{
  return vcleq_f32(a.m, b.m);
}

prt_inline vbool4 operator >=(const vfloat4& a, const vfloat4& b)
{
  return vcgeq_f32(a.m, b.m);
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
  return vabsq_f32(a.m);
}

prt_inline vfloat4 rcp(const vfloat4& a)
{
  // Newton-Raphson refinement for reciprocal
  float32x4_t r = vrecpeq_f32(a.m);
  r = vmulq_f32(vrecpsq_f32(a.m, r), r);
  r = vmulq_f32(vrecpsq_f32(a.m, r), r);
  return r;
}

prt_inline vfloat4 sqrt(const vfloat4& a)
{
  return vsqrtq_f32(a.m);
}

prt_inline vfloat4 rsqrt(const vfloat4& a)
{
  // Newton-Raphson refinement for reciprocal sqrt
  float32x4_t r = vrsqrteq_f32(a.m);
  r = vmulq_f32(vrsqrtsq_f32(vmulq_f32(a.m, r), r), r);
  r = vmulq_f32(vrsqrtsq_f32(vmulq_f32(a.m, r), r), r);
  return r;
}

prt_inline vfloat4 floor(const vfloat4& a)
{
  return vrndmq_f32(a.m);
}

prt_inline vfloat4 ceil(const vfloat4& a)
{
  return vrndpq_f32(a.m);
}

prt_inline vfloat4 round(const vfloat4& a)
{
  return vrndnq_f32(a.m);
}

prt_inline vfloat4 min(const vfloat4& a, const vfloat4& b)
{
  return vminq_f32(a.m, b.m);
}

prt_inline vfloat4 max(const vfloat4& a, const vfloat4& b)
{
  return vmaxq_f32(a.m, b.m);
}

prt_inline vfloat4 vdot3(const vfloat4& a, const vfloat4& b)
{
  float32x4_t prod = vmulq_f32(a.m, b.m);
  // Zero out lane 3
  float d = vgetq_lane_f32(prod, 0) + vgetq_lane_f32(prod, 1) + vgetq_lane_f32(prod, 2);
  return vdupq_n_f32(d);
}

prt_inline vfloat4 vdot4(const vfloat4& a, const vfloat4& b)
{
  float32x4_t prod = vmulq_f32(a.m, b.m);
  float d = vaddvq_f32(prod);
  return vdupq_n_f32(d);
}

prt_inline vfloat4 vcross3(const vfloat4& a, const vfloat4& b)
{
  // a.yzx * b.zxy - a.zxy * b.yzx
  vfloat4 a_yzx = permute4<1,2,0,3>(a);
  vfloat4 b_yzx = permute4<1,2,0,3>(b);
  float32x4_t result = vsubq_f32(vmulq_f32(a.m, b_yzx.m), vmulq_f32(a_yzx.m, b.m));
  return permute4<1,2,0,3>(vfloat4(result));
}

// Reduction functions
// -------------------

prt_inline vfloat4 vreduceMin(const vfloat4& a)
{
  return vdupq_n_f32(vminvq_f32(a.m));
}

prt_inline vfloat4 vreduceMax(const vfloat4& a)
{
  return vdupq_n_f32(vmaxvq_f32(a.m));
}

prt_inline vfloat4 vreduceAdd(const vfloat4& a)
{
  return vdupq_n_f32(vaddvq_f32(a.m));
}

prt_inline float reduceMin(const vfloat4& a) { return vminvq_f32(a.m); }
prt_inline float reduceMax(const vfloat4& a) { return vmaxvq_f32(a.m); }
prt_inline float reduceAdd(const vfloat4& a) { return vaddvq_f32(a.m); }

prt_inline int selectMin(const vfloat4& a) { return bitScan(toIntMask(a == vreduceMin(a))); }
prt_inline int selectMax(const vfloat4& a) { return bitScan(toIntMask(a == vreduceMax(a))); }
prt_inline int selectAdd(const vfloat4& a) { return bitScan(toIntMask(a == vreduceAdd(a))); }

// Misc functions
// --------------

prt_inline vfloat4 sort(const vfloat4& a)
{
  vfloat4 r = a;
  // Simple sorting network for 4 elements
  // Compare-and-swap pairs
  auto swap = [](float& x, float& y) { if (x > y) { float t = x; x = y; y = t; } };
  swap(r[0], r[1]); swap(r[2], r[3]);
  swap(r[0], r[2]); swap(r[1], r[3]);
  swap(r[1], r[2]);
  return r;
}

prt_inline vfloat4 rsort(const vfloat4& a)
{
  vfloat4 r = a;
  auto swap = [](float& x, float& y) { if (x < y) { float t = x; x = y; y = t; } };
  swap(r[0], r[1]); swap(r[2], r[3]);
  swap(r[0], r[2]); swap(r[1], r[3]);
  swap(r[1], r[2]);
  return r;
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

} // namespace prt
