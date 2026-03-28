// Copyright 2026 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <arm_neon.h>
#include "../simd_common.h"
#include "vbool8_neon.h"

namespace prt {

// vint8 - double-pumped using two int32x4_t
template <>
struct var<int,8>
{
  union
  {
    struct { int32x4_t lo, hi; };
    int v[8];
  };

  prt_inline var() {}

  prt_inline var(const vint8& a) : lo(a.lo), hi(a.hi) {}
  prt_inline var(const vuint8& a);
  prt_inline var(int32x4_t a0, int32x4_t a1) : lo(a0), hi(a1) {}
  prt_inline var(int a) : lo(vdupq_n_s32(a)), hi(vdupq_n_s32(a)) {}
  prt_inline var(int a0, int a1, int a2, int a3, int a4, int a5, int a6, int a7)
  {
    int dlo[4] = {a0,a1,a2,a3};
    int dhi[4] = {a4,a5,a6,a7};
    lo = vld1q_s32(dlo);
    hi = vld1q_s32(dhi);
  }

  prt_inline var(Zero) : lo(vdupq_n_s32(0)), hi(vdupq_n_s32(0)) {}
  prt_inline var(One)  : lo(vdupq_n_s32(1)), hi(vdupq_n_s32(1)) {}
  prt_inline var(Step)
  {
    int dlo[4] = {0,1,2,3};
    int dhi[4] = {4,5,6,7};
    lo = vld1q_s32(dlo);
    hi = vld1q_s32(dhi);
  }

  prt_inline vint8& operator =(const vint8& a)
  {
    lo = a.lo;
    hi = a.hi;
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
  return vint8(vld1q_s32(ptr), vld1q_s32(ptr+4));
}

template <>
prt_inline vint8 load(const vbool8& mask, const int* ptr)
{
  vint8 r;
  for (int i = 0; i < 8; ++i)
    if (mask[i]) r[i] = ptr[i];
  return r;
}

template <>
prt_inline vint8 loadu(const int* ptr)
{
  return vint8(vld1q_s32(ptr), vld1q_s32(ptr+4));
}

template <>
prt_inline vint8 loadu(const vbool8& mask, const int* ptr)
{
  vint8 r;
  for (int i = 0; i < 8; ++i)
    if (mask[i]) r[i] = ptr[i];
  return r;
}

prt_inline vint8 expand(const vbool8& mask, const int* ptr)
{
  vint8 r;
  int offset = 0;
  for (int i = 0; i < 8; ++i)
    if (mask[i]) r[i] = ptr[offset++];
  return r;
}

prt_inline vint8 gather(const int* ptr, const vint8& idx)
{
  vint8 r;
  for (int i = 0; i < 8; ++i)
    r[i] = ptr[idx[i]];
  return r;
}

prt_inline vint8 gather(const vbool8& mask, const int* ptr, const vint8& idx)
{
  vint8 r(zero);
  for (int i = 0; i < 8; ++i)
    if (mask[i]) r[i] = ptr[idx[i]];
  return r;
}

// Store functions
// ---------------

prt_inline void store(int* ptr, const vint8& a)
{
  vst1q_s32(ptr, a.lo);
  vst1q_s32(ptr+4, a.hi);
}

prt_inline void store(const vbool8& mask, int* ptr, const vint8& a)
{
  for (int i = 0; i < 8; ++i)
    if (mask[i]) ptr[i] = a[i];
}

prt_inline void storeu(int* ptr, const vint8& a)
{
  vst1q_s32(ptr, a.lo);
  vst1q_s32(ptr+4, a.hi);
}

prt_inline void storeu(const vbool8& mask, int* ptr, const vint8& a)
{
  for (int i = 0; i < 8; ++i)
    if (mask[i]) ptr[i] = a[i];
}

prt_inline void compress(const vbool8& mask, int* ptr, const vint8& a)
{
  int offset = 0;
  for (int i = 0; i < 8; ++i)
    if (mask[i]) ptr[offset++] = a[i];
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
  return vint8(vbslq_s32(mask.lo, a.lo, b.lo), vbslq_s32(mask.hi, a.hi, b.hi));
}

// Arithmetic operators
// --------------------

prt_inline vint8 operator +(const vint8& a, const vint8& b)
{
  return vint8(vaddq_s32(a.lo, b.lo), vaddq_s32(a.hi, b.hi));
}

prt_inline vint8 operator -(const vint8& a, const vint8& b)
{
  return vint8(vsubq_s32(a.lo, b.lo), vsubq_s32(a.hi, b.hi));
}

prt_inline vint8 operator *(const vint8& a, const vint8& b)
{
  return vint8(vmulq_s32(a.lo, b.lo), vmulq_s32(a.hi, b.hi));
}

// Bitwise operators
// -----------------

prt_inline vint8 operator &(const vint8& a, const vint8& b)
{
  return vint8(vandq_s32(a.lo, b.lo), vandq_s32(a.hi, b.hi));
}

prt_inline vint8 operator |(const vint8& a, const vint8& b)
{
  return vint8(vorrq_s32(a.lo, b.lo), vorrq_s32(a.hi, b.hi));
}

prt_inline vint8 operator ^(const vint8& a, const vint8& b)
{
  return vint8(veorq_s32(a.lo, b.lo), veorq_s32(a.hi, b.hi));
}

prt_inline vint8 andn(const vint8& a, const vint8& b)
{
  return vint8(vbicq_s32(a.lo, b.lo), vbicq_s32(a.hi, b.hi));
}

prt_inline vint8 operator <<(const vint8& a, int b)
{
  int32x4_t sv = vdupq_n_s32(b);
  return vint8(vshlq_s32(a.lo, sv), vshlq_s32(a.hi, sv));
}

prt_inline vint8 operator >>(const vint8& a, int b)
{
  int32x4_t sv = vdupq_n_s32(-b);
  return vint8(vshlq_s32(a.lo, sv), vshlq_s32(a.hi, sv));
}

prt_inline vint8 shl(const vint8& a, int b)
{
  int32x4_t sv = vdupq_n_s32(b);
  return vint8(vshlq_s32(a.lo, sv), vshlq_s32(a.hi, sv));
}

prt_inline vint8 shr(const vint8& a, int b)
{
  int32x4_t sv = vdupq_n_s32(-b);
  return vint8(
    vreinterpretq_s32_u32(vshlq_u32(vreinterpretq_u32_s32(a.lo), sv)),
    vreinterpretq_s32_u32(vshlq_u32(vreinterpretq_u32_s32(a.hi), sv)));
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
  return vbool8(vceqq_s32(a.lo, b.lo), vceqq_s32(a.hi, b.hi));
}

prt_inline vbool8 operator <(const vint8& a, const vint8& b)
{
  return vbool8(vcltq_s32(a.lo, b.lo), vcltq_s32(a.hi, b.hi));
}

prt_inline vbool8 operator >(const vint8& a, const vint8& b)
{
  return vbool8(vcgtq_s32(a.lo, b.lo), vcgtq_s32(a.hi, b.hi));
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
  return vint8(vminq_s32(a.lo, b.lo), vminq_s32(a.hi, b.hi));
}

prt_inline vint8 max(const vint8& a, const vint8& b)
{
  return vint8(vmaxq_s32(a.lo, b.lo), vmaxq_s32(a.hi, b.hi));
}

} // namespace prt
