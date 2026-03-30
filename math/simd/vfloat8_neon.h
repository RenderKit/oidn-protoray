// Copyright 2026 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <arm_neon.h>
#include "../simd_common.h"
#include "vbool8_neon.h"
#include "vint8_neon.h"
#include "vuint8_neon.h"

namespace prt {

// vfloat8 - double-pumped using two float32x4_t
template <>
struct var<float,8>
{
  union
  {
    struct { float32x4_t lo, hi; };
    float v[8];
  };

  prt_inline var() {}

  prt_inline var(const vfloat8& a) : lo(a.lo), hi(a.hi) {}
  prt_inline var(float32x4_t a0, float32x4_t a1) : lo(a0), hi(a1) {}
  prt_inline var(float a) : lo(vdupq_n_f32(a)), hi(vdupq_n_f32(a)) {}
  prt_inline var(float a0, float a1, float a2, float a3, float a4, float a5, float a6, float a7)
  {
    float dlo[4] = {a0,a1,a2,a3};
    float dhi[4] = {a4,a5,a6,a7};
    lo = vld1q_f32(dlo);
    hi = vld1q_f32(dhi);
  }

  prt_inline var(Zero)   : lo(vdupq_n_f32(0.0f)), hi(vdupq_n_f32(0.0f)) {}
  prt_inline var(One)    : lo(vdupq_n_f32(1.0f)), hi(vdupq_n_f32(1.0f)) {}
  prt_inline var(PosMax) : lo(vdupq_n_f32(posMax)), hi(vdupq_n_f32(posMax)) {}
  prt_inline var(NegMax) : lo(vdupq_n_f32(negMax)), hi(vdupq_n_f32(negMax)) {}
  prt_inline var(PosInf) : lo(vdupq_n_f32(posInf)), hi(vdupq_n_f32(posInf)) {}
  prt_inline var(NegInf) : lo(vdupq_n_f32(negInf)), hi(vdupq_n_f32(negInf)) {}
  prt_inline var(Qnan)   : lo(vdupq_n_f32(qnan)), hi(vdupq_n_f32(qnan)) {}
  prt_inline var(Step)
  {
    float dlo[4] = {0.0f,1.0f,2.0f,3.0f};
    float dhi[4] = {4.0f,5.0f,6.0f,7.0f};
    lo = vld1q_f32(dlo);
    hi = vld1q_f32(dhi);
  }

  prt_inline vfloat8& operator =(const vfloat8& a)
  {
    lo = a.lo;
    hi = a.hi;
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
  return vfloat8(vld1q_f32(ptr), vld1q_f32(ptr+4));
}

template <>
prt_inline vfloat8 load(const vbool8& mask, const float* ptr)
{
  vfloat8 r;
  for (int i = 0; i < 8; ++i)
    r[i] = mask[i] ? ptr[i] : 0.0f;
  return r;
}

template <>
prt_inline vfloat8 loadu(const float* ptr)
{
  return vfloat8(vld1q_f32(ptr), vld1q_f32(ptr+4));
}

template <>
prt_inline vfloat8 loadu(const vbool8& mask, const float* ptr)
{
  vfloat8 r;
  for (int i = 0; i < 8; ++i)
    r[i] = mask[i] ? ptr[i] : 0.0f;
  return r;
}

prt_inline vfloat8 expand(const vbool8& mask, const float* ptr)
{
  vfloat8 r;
  int offset = 0;
  for (int i = 0; i < 8; ++i)
    if (mask[i]) r[i] = ptr[offset++];
  return r;
}

prt_inline vfloat8 gather(const float* ptr, const vint8& idx)
{
  vfloat8 r;
  for (int i = 0; i < 8; ++i)
    r[i] = ptr[idx[i]];
  return r;
}

prt_inline vfloat8 gather(const vbool8& mask, const float* ptr, const vint8& idx)
{
  vfloat8 r;
  for (int i = 0; i < 8; ++i)
    r[i] = mask[i] ? ptr[idx[i]] : 0.0f;
  return r;
}

// Store functions
// ---------------

prt_inline void store(float* ptr, const vfloat8& a)
{
  vst1q_f32(ptr, a.lo);
  vst1q_f32(ptr+4, a.hi);
}

prt_inline void store(const vbool8& mask, float* ptr, const vfloat8& a)
{
  for (int i = 0; i < 8; ++i)
    if (mask[i]) ptr[i] = a[i];
}

prt_inline void storeu(float* ptr, const vfloat8& a)
{
  vst1q_f32(ptr, a.lo);
  vst1q_f32(ptr+4, a.hi);
}

prt_inline void storeu(const vbool8& mask, float* ptr, const vfloat8& a)
{
  for (int i = 0; i < 8; ++i)
    if (mask[i]) ptr[i] = a[i];
}

prt_inline void compress(const vbool8& mask, float* ptr, const vfloat8& a)
{
  int offset = 0;
  for (int i = 0; i < 8; ++i)
    if (mask[i]) ptr[offset++] = a[i];
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
  return vgetq_lane_f32(a.lo, 0);
}

prt_inline vint8 toInt(const vfloat8& a)
{
  return vint8(vcvtq_s32_f32(a.lo), vcvtq_s32_f32(a.hi));
}

prt_inline vint8 asInt(const vfloat8& a)
{
  return vint8(vreinterpretq_s32_f32(a.lo), vreinterpretq_s32_f32(a.hi));
}

prt_inline vuint8 asUInt(const vfloat8& a)
{
  return vuint8(vreinterpretq_u32_f32(a.lo), vreinterpretq_u32_f32(a.hi));
}

prt_inline vfloat8 toFloat(const vint8& a)
{
  return vfloat8(vcvtq_f32_s32(a.lo), vcvtq_f32_s32(a.hi));
}

prt_inline vfloat8 toFloat(const vuint8& a)
{
  return vfloat8(vcvtq_f32_u32(a.lo), vcvtq_f32_u32(a.hi));
}

prt_inline vfloat8 asFloat(const vint8& a)
{
  return vfloat8(vreinterpretq_f32_s32(a.lo), vreinterpretq_f32_s32(a.hi));
}

prt_inline int toIntMask(const vfloat8& a)
{
  return toIntMask(vbool8(vreinterpretq_u32_f32(a.lo), vreinterpretq_u32_f32(a.hi)));
}

// Select function
// ---------------

prt_inline vfloat8 select(const vbool8& mask, const vfloat8& a, const vfloat8& b)
{
  return vfloat8(vbslq_f32(mask.lo, a.lo, b.lo), vbslq_f32(mask.hi, a.hi, b.hi));
}

// Arithmetic operators
// --------------------

prt_inline vfloat8 operator +(const vfloat8& a)
{
  return a;
}

prt_inline vfloat8 operator -(const vfloat8& a)
{
  return vfloat8(vnegq_f32(a.lo), vnegq_f32(a.hi));
}

prt_inline vfloat8 operator +(const vfloat8& a, const vfloat8& b)
{
  return vfloat8(vaddq_f32(a.lo, b.lo), vaddq_f32(a.hi, b.hi));
}

prt_inline vfloat8 operator -(const vfloat8& a, const vfloat8& b)
{
  return vfloat8(vsubq_f32(a.lo, b.lo), vsubq_f32(a.hi, b.hi));
}

prt_inline vfloat8 operator *(const vfloat8& a, const vfloat8& b)
{
  return vfloat8(vmulq_f32(a.lo, b.lo), vmulq_f32(a.hi, b.hi));
}

prt_inline vfloat8 operator /(const vfloat8& a, const vfloat8& b)
{
  return vfloat8(vdivq_f32(a.lo, b.lo), vdivq_f32(a.hi, b.hi));
}

// Bitwise operators
// -----------------

prt_inline vfloat8 operator &(const vfloat8& a, const vfloat8& b)
{
  return vfloat8(
    vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(a.lo), vreinterpretq_u32_f32(b.lo))),
    vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(a.hi), vreinterpretq_u32_f32(b.hi))));
}

prt_inline vfloat8 operator |(const vfloat8& a, const vfloat8& b)
{
  return vfloat8(
    vreinterpretq_f32_u32(vorrq_u32(vreinterpretq_u32_f32(a.lo), vreinterpretq_u32_f32(b.lo))),
    vreinterpretq_f32_u32(vorrq_u32(vreinterpretq_u32_f32(a.hi), vreinterpretq_u32_f32(b.hi))));
}

prt_inline vfloat8 operator ^(const vfloat8& a, const vfloat8& b)
{
  return vfloat8(
    vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(a.lo), vreinterpretq_u32_f32(b.lo))),
    vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(a.hi), vreinterpretq_u32_f32(b.hi))));
}

prt_inline vfloat8 andn(const vfloat8& a, const vfloat8& b)
{
  return vfloat8(
    vreinterpretq_f32_u32(vbicq_u32(vreinterpretq_u32_f32(a.lo), vreinterpretq_u32_f32(b.lo))),
    vreinterpretq_f32_u32(vbicq_u32(vreinterpretq_u32_f32(a.hi), vreinterpretq_u32_f32(b.hi))));
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
  return vbool8(vceqq_f32(a.lo, b.lo), vceqq_f32(a.hi, b.hi));
}

prt_inline vbool8 operator !=(const vfloat8& a, const vfloat8& b)
{
  return vbool8(vmvnq_u32(vceqq_f32(a.lo, b.lo)), vmvnq_u32(vceqq_f32(a.hi, b.hi)));
}

prt_inline vbool8 operator <(const vfloat8& a, const vfloat8& b)
{
  return vbool8(vcltq_f32(a.lo, b.lo), vcltq_f32(a.hi, b.hi));
}

prt_inline vbool8 operator >(const vfloat8& a, const vfloat8& b)
{
  return vbool8(vcgtq_f32(a.lo, b.lo), vcgtq_f32(a.hi, b.hi));
}

prt_inline vbool8 operator <=(const vfloat8& a, const vfloat8& b)
{
  return vbool8(vcleq_f32(a.lo, b.lo), vcleq_f32(a.hi, b.hi));
}

prt_inline vbool8 operator >=(const vfloat8& a, const vfloat8& b)
{
  return vbool8(vcgeq_f32(a.lo, b.lo), vcgeq_f32(a.hi, b.hi));
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
  return vfloat8(vabsq_f32(a.lo), vabsq_f32(a.hi));
}

prt_inline vfloat8 rcp(const vfloat8& a)
{
  float32x4_t rlo = vrecpeq_f32(a.lo);
  rlo = vmulq_f32(vrecpsq_f32(a.lo, rlo), rlo);
  rlo = vmulq_f32(vrecpsq_f32(a.lo, rlo), rlo);
  float32x4_t rhi = vrecpeq_f32(a.hi);
  rhi = vmulq_f32(vrecpsq_f32(a.hi, rhi), rhi);
  rhi = vmulq_f32(vrecpsq_f32(a.hi, rhi), rhi);
  return vfloat8(rlo, rhi);
}

prt_inline vfloat8 sqrt(const vfloat8& a)
{
  return vfloat8(vsqrtq_f32(a.lo), vsqrtq_f32(a.hi));
}

prt_inline vfloat8 rsqrt(const vfloat8& a)
{
  float32x4_t rlo = vrsqrteq_f32(a.lo);
  rlo = vmulq_f32(vrsqrtsq_f32(vmulq_f32(a.lo, rlo), rlo), rlo);
  rlo = vmulq_f32(vrsqrtsq_f32(vmulq_f32(a.lo, rlo), rlo), rlo);
  float32x4_t rhi = vrsqrteq_f32(a.hi);
  rhi = vmulq_f32(vrsqrtsq_f32(vmulq_f32(a.hi, rhi), rhi), rhi);
  rhi = vmulq_f32(vrsqrtsq_f32(vmulq_f32(a.hi, rhi), rhi), rhi);
  return vfloat8(rlo, rhi);
}

prt_inline vfloat8 floor(const vfloat8& a)
{
  return vfloat8(vrndmq_f32(a.lo), vrndmq_f32(a.hi));
}

prt_inline vfloat8 ceil(const vfloat8& a)
{
  return vfloat8(vrndpq_f32(a.lo), vrndpq_f32(a.hi));
}

prt_inline vfloat8 round(const vfloat8& a)
{
  return vfloat8(vrndnq_f32(a.lo), vrndnq_f32(a.hi));
}

prt_inline vfloat8 min(const vfloat8& a, const vfloat8& b)
{
  return vfloat8(vminq_f32(a.lo, b.lo), vminq_f32(a.hi, b.hi));
}

prt_inline vfloat8 max(const vfloat8& a, const vfloat8& b)
{
  return vfloat8(vmaxq_f32(a.lo, b.lo), vmaxq_f32(a.hi, b.hi));
}

prt_inline vfloat8 vdot3(const vfloat8& a, const vfloat8& b)
{
  // Per-lane dot product (treating each group of 4 as xyz+w)
  float32x4_t prodLo = vmulq_f32(a.lo, b.lo);
  float32x4_t prodHi = vmulq_f32(a.hi, b.hi);
  float dLo = vgetq_lane_f32(prodLo, 0) + vgetq_lane_f32(prodLo, 1) + vgetq_lane_f32(prodLo, 2);
  float dHi = vgetq_lane_f32(prodHi, 0) + vgetq_lane_f32(prodHi, 1) + vgetq_lane_f32(prodHi, 2);
  return vfloat8(vdupq_n_f32(dLo), vdupq_n_f32(dHi));
}

prt_inline vfloat8 vdot4(const vfloat8& a, const vfloat8& b)
{
  float32x4_t prodLo = vmulq_f32(a.lo, b.lo);
  float32x4_t prodHi = vmulq_f32(a.hi, b.hi);
  float dLo = vaddvq_f32(prodLo);
  float dHi = vaddvq_f32(prodHi);
  return vfloat8(vdupq_n_f32(dLo), vdupq_n_f32(dHi));
}

prt_inline vfloat8 vcross3(const vfloat8& a, const vfloat8& b)
{
  // Process lo and hi halves separately
  vfloat4 aLo(a.lo), aHi(a.hi), bLo(b.lo), bHi(b.hi);
  vfloat4 rLo = vcross3(aLo, bLo);
  vfloat4 rHi = vcross3(aHi, bHi);
  return vfloat8(rLo.m, rHi.m);
}

// Reduction functions
// -------------------

prt_inline vfloat8 vreduceMin(const vfloat8& a)
{
  float minVal = std::min(vminvq_f32(a.lo), vminvq_f32(a.hi));
  return vfloat8(vdupq_n_f32(minVal), vdupq_n_f32(minVal));
}

prt_inline vfloat8 vreduceMax(const vfloat8& a)
{
  float maxVal = std::max(vmaxvq_f32(a.lo), vmaxvq_f32(a.hi));
  return vfloat8(vdupq_n_f32(maxVal), vdupq_n_f32(maxVal));
}

prt_inline vfloat8 vreduceAdd(const vfloat8& a)
{
  float sum = vaddvq_f32(a.lo) + vaddvq_f32(a.hi);
  return vfloat8(vdupq_n_f32(sum), vdupq_n_f32(sum));
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
  vfloat8 r = a;
  // Simple sorting network for 8 elements
  auto cmpswap = [](float& x, float& y) { if (x > y) { float t = x; x = y; y = t; } };
  // Batcher's odd-even merge sort for 8 elements
  cmpswap(r[0], r[1]); cmpswap(r[2], r[3]); cmpswap(r[4], r[5]); cmpswap(r[6], r[7]);
  cmpswap(r[0], r[2]); cmpswap(r[1], r[3]); cmpswap(r[4], r[6]); cmpswap(r[5], r[7]);
  cmpswap(r[1], r[2]); cmpswap(r[5], r[6]);
  cmpswap(r[0], r[4]); cmpswap(r[1], r[5]); cmpswap(r[2], r[6]); cmpswap(r[3], r[7]);
  cmpswap(r[2], r[4]); cmpswap(r[3], r[5]);
  cmpswap(r[1], r[2]); cmpswap(r[3], r[4]); cmpswap(r[5], r[6]);
  return r;
}

prt_inline vfloat8 rsort(const vfloat8& a)
{
  vfloat8 r = a;
  auto cmpswap = [](float& x, float& y) { if (x < y) { float t = x; x = y; y = t; } };
  cmpswap(r[0], r[1]); cmpswap(r[2], r[3]); cmpswap(r[4], r[5]); cmpswap(r[6], r[7]);
  cmpswap(r[0], r[2]); cmpswap(r[1], r[3]); cmpswap(r[4], r[6]); cmpswap(r[5], r[7]);
  cmpswap(r[1], r[2]); cmpswap(r[5], r[6]);
  cmpswap(r[0], r[4]); cmpswap(r[1], r[5]); cmpswap(r[2], r[6]); cmpswap(r[3], r[7]);
  cmpswap(r[2], r[4]); cmpswap(r[3], r[5]);
  cmpswap(r[1], r[2]); cmpswap(r[3], r[4]); cmpswap(r[5], r[6]);
  return r;
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
  // SoA to AoS transpose for 8 vec3s -> 3 vfloat8s
  // Extract x, y, z components from each vec4
  y0 = vfloat8(
    vfloat4(x0[0], x1[0], x2[0], x3[0]).m,
    vfloat4(x4[0], x5[0], x6[0], x7[0]).m);
  y1 = vfloat8(
    vfloat4(x0[1], x1[1], x2[1], x3[1]).m,
    vfloat4(x4[1], x5[1], x6[1], x7[1]).m);
  y2 = vfloat8(
    vfloat4(x0[2], x1[2], x2[2], x3[2]).m,
    vfloat4(x4[2], x5[2], x6[2], x7[2]).m);
}

} // namespace prt
