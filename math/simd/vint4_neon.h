// Copyright 2026 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <arm_neon.h>
#include "../simd_common.h"
#include "vbool4_neon.h"

namespace prt {

// vint4
template <>
struct var<int,4>
{
  union
  {
    int32x4_t m;
    int v[4];
  };

  prt_inline var() {}

  prt_inline var(const vint4& a) : m(a.m) {}
  prt_inline var(const vuint4& a);
  prt_inline var(int32x4_t a) : m(a) {}
  prt_inline var(int a) : m(vdupq_n_s32(a)) {}
  prt_inline var(int a0, int a1, int a2, int a3) { int d[4] = {a0,a1,a2,a3}; m = vld1q_s32(d); }

  prt_inline var(Zero) : m(vdupq_n_s32(0)) {}
  prt_inline var(One)  : m(vdupq_n_s32(1)) {}
  prt_inline var(Step)  { int d[4] = {0,1,2,3}; m = vld1q_s32(d); }

  prt_inline vint4& operator =(const vint4& a)
  {
    m = a.m;
    return *this;
  }

  prt_inline const int& operator [](size_t i) const
  {
    assert(i < 4);
    return v[i];
  }

  prt_inline int& operator [](size_t i)
  {
    assert(i < 4);
    return v[i];
  }
};

// Load functions
// --------------

template <>
prt_inline vint4 load(const int* ptr)
{
  return vld1q_s32(ptr);
}

template <>
prt_inline vint4 load(const vbool4& mask, const int* ptr)
{
  vint4 r;
  for (int i = 0; i < 4; ++i)
    if (mask[i]) r[i] = ptr[i];
  return r;
}

template <>
prt_inline vint4 loadu(const int* ptr)
{
  return vld1q_s32(ptr);
}

prt_inline vint4 gather(const int* ptr, const vint4& idx)
{
  vint4 r;
  for (int i = 0; i < 4; ++i)
    r[i] = ptr[idx[i]];
  return r;
}

prt_inline vint4 gather(const vbool4& mask, const int* ptr, const vint4& idx)
{
  vint4 r;
  for (int i = 0; i < 4; ++i)
    if (mask[i]) r[i] = ptr[idx[i]];
  return r;
}

// Store functions
// ---------------

prt_inline void store(int* ptr, const vint4& a)
{
  vst1q_s32(ptr, a.m);
}

prt_inline void store(const vbool4& mask, int* ptr, const vint4& a)
{
  for (int i = 0; i < 4; ++i)
    if (mask[i]) ptr[i] = a[i];
}

prt_inline void compress(const vbool4& mask, int* ptr, const vint4& a)
{
  int offset = 0;
  for (int i = 0; i < 4; ++i)
    if (mask[i]) ptr[offset++] = a[i];
}

prt_inline void scatter(int* ptr, const vint4& idx, const vint4& a)
{
  for (int i = 0; i < 4; ++i)
    ptr[idx[i]] = a[i];
}

prt_inline void scatter(const vbool4& mask, int* ptr, const vint4& idx, const vint4& a)
{
  for (int i = 0; i < 4; ++i)
    if (mask[i]) ptr[idx[i]] = a[i];
}

// Select function
// ---------------

prt_inline vint4 select(const vbool4& mask, const vint4& a, const vint4& b)
{
  return vbslq_s32(mask.m, a.m, b.m);
}

// Shuffle functions
// -----------------

template <int i>
prt_inline int extract(const vint4& a)
{
  return vgetq_lane_s32(a.m, i);
}

template <int i>
prt_inline vint4 broadcast(const vint4& a)
{
  return vdupq_laneq_s32(a.m, i);
}

template <int i0, int i1, int i2, int i3>
prt_inline vint4 permute4(const vint4& a)
{
  vint4 r;
  r[0] = a[i0]; r[1] = a[i1]; r[2] = a[i2]; r[3] = a[i3];
  return r;
}

// Arithmetic operators
// --------------------

prt_inline vint4 operator +(const vint4& a, const vint4& b)
{
  return vaddq_s32(a.m, b.m);
}

prt_inline vint4 operator -(const vint4& a, const vint4& b)
{
  return vsubq_s32(a.m, b.m);
}

prt_inline vint4 operator *(const vint4& a, const vint4& b)
{
  return vmulq_s32(a.m, b.m);
}

// Bitwise operators
// -----------------

prt_inline vint4 operator &(const vint4& a, const vint4& b)
{
  return vandq_s32(a.m, b.m);
}

prt_inline vint4 operator |(const vint4& a, const vint4& b)
{
  return vorrq_s32(a.m, b.m);
}

prt_inline vint4 operator ^(const vint4& a, const vint4& b)
{
  return veorq_s32(a.m, b.m);
}

prt_inline vint4 andn(const vint4& a, const vint4& b)
{
  return vbicq_s32(a.m, b.m); // a & ~b
}

prt_inline vint4 operator <<(const vint4& a, int b)
{
  return vshlq_s32(a.m, vdupq_n_s32(b));
}

prt_inline vint4 operator >>(const vint4& a, int b)
{
  return vshlq_s32(a.m, vdupq_n_s32(-b)); // arithmetic right shift
}

prt_inline vint4 shl(const vint4& a, int b)
{
  return vshlq_s32(a.m, vdupq_n_s32(b));
}

prt_inline vint4 shr(const vint4& a, int b)
{
  return vreinterpretq_s32_u32(vshlq_u32(vreinterpretq_u32_s32(a.m), vdupq_n_s32(-b))); // logical right shift
}

// Assignment operators
// --------------------

prt_inline vint4& operator +=(vint4& a, const vint4& b) { return a = a + b; }
prt_inline vint4& operator -=(vint4& a, const vint4& b) { return a = a - b; }
prt_inline vint4& operator *=(vint4& a, const vint4& b) { return a = a * b; }
prt_inline vint4& operator &=(vint4& a, const vint4& b) { return a = a & b; }
prt_inline vint4& operator |=(vint4& a, const vint4& b) { return a = a | b; }
prt_inline vint4& operator ^=(vint4& a, const vint4& b) { return a = a ^ b; }

prt_inline vint4& operator <<=(vint4& a, int b) { return a = a << b; }
prt_inline vint4& operator >>=(vint4& a, int b) { return a = a >> b; }

// Compare operators
// -----------------

prt_inline vbool4 operator ==(const vint4& a, const vint4& b)
{
  return vceqq_s32(a.m, b.m);
}

prt_inline vbool4 operator <(const vint4& a, const vint4& b)
{
  return vcltq_s32(a.m, b.m);
}

prt_inline vbool4 operator >(const vint4& a, const vint4& b)
{
  return vcgtq_s32(a.m, b.m);
}

prt_inline vbool4 operator !=(const vint4& a, const vint4& b)
{
  return !(a == b);
}

prt_inline vbool4 operator <=(const vint4& a, const vint4& b)
{
  return vcleq_s32(a.m, b.m);
}

prt_inline vbool4 operator >=(const vint4& a, const vint4& b)
{
  return vcgeq_s32(a.m, b.m);
}

// Compare functions
// -----------------

prt_inline vbool4 cmpEq(const vint4& a, const vint4& b) { return a == b; }
prt_inline vbool4 cmpNe(const vint4& a, const vint4& b) { return a != b; }
prt_inline vbool4 cmpLt(const vint4& a, const vint4& b) { return a <  b; }
prt_inline vbool4 cmpLe(const vint4& a, const vint4& b) { return a <= b; }
prt_inline vbool4 cmpGt(const vint4& a, const vint4& b) { return a >  b; }
prt_inline vbool4 cmpGe(const vint4& a, const vint4& b) { return a >= b; }

prt_inline vbool4 cmpEq(const vbool4& mask, const vint4& a, const vint4& b) { return (a == b) & mask; }
prt_inline vbool4 cmpNe(const vbool4& mask, const vint4& a, const vint4& b) { return (a != b) & mask; }
prt_inline vbool4 cmpLt(const vbool4& mask, const vint4& a, const vint4& b) { return (a <  b) & mask; }
prt_inline vbool4 cmpLe(const vbool4& mask, const vint4& a, const vint4& b) { return (a <= b) & mask; }
prt_inline vbool4 cmpGt(const vbool4& mask, const vint4& a, const vint4& b) { return (a >  b) & mask; }
prt_inline vbool4 cmpGe(const vbool4& mask, const vint4& a, const vint4& b) { return (a >= b) & mask; }

// Math functions
// --------------

prt_inline vint4 min(const vint4& a, const vint4& b)
{
  return vminq_s32(a.m, b.m);
}

prt_inline vint4 max(const vint4& a, const vint4& b)
{
  return vmaxq_s32(a.m, b.m);
}

} // namespace prt
