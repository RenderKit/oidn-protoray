// Copyright 2026 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <arm_neon.h>
#include "../simd_common.h"
#include "vbool4_neon.h"
#include "vint4_neon.h"

namespace prt {

// vuint4
template <>
struct var<uint,4>
{
  union
  {
    uint32x4_t m;
    uint v[4];
  };

  prt_inline var() {}

  prt_inline var(const vuint4& a) : m(a.m) {}
  prt_inline var(const vint4& a) : m(vreinterpretq_u32_s32(a.m)) {}
  prt_inline var(uint32x4_t a) : m(a) {}
  prt_inline var(uint a) : m(vdupq_n_u32(a)) {}
  prt_inline var(uint a0, uint a1, uint a2, uint a3) { uint d[4] = {a0,a1,a2,a3}; m = vld1q_u32(d); }

  prt_inline var(Zero) : m(vdupq_n_u32(0)) {}
  prt_inline var(One)  : m(vdupq_n_u32(1)) {}
  prt_inline var(Step) { uint d[4] = {0,1,2,3}; m = vld1q_u32(d); }

  prt_inline vuint4& operator =(const vuint4& a)
  {
    m = a.m;
    return *this;
  }

  prt_inline const uint& operator [](size_t i) const
  {
    assert(i < 4);
    return v[i];
  }

  prt_inline uint& operator [](size_t i)
  {
    assert(i < 4);
    return v[i];
  }
};

prt_inline vint4::var(const vuint4& a) : m(vreinterpretq_s32_u32(a.m)) {}

// Load functions
// --------------

template <>
prt_inline vuint4 load(const uint* ptr)
{
  return vld1q_u32(ptr);
}

template <>
prt_inline vuint4 load(const vbool4& mask, const uint* ptr)
{
  vuint4 r;
  for (int i = 0; i < 4; ++i)
    r[i] = mask[i] ? ptr[i] : 0;
  return r;
}

template <>
prt_inline vuint4 loadu(const uint* ptr)
{
  return vld1q_u32(ptr);
}

prt_inline vuint4 gather(const uint* ptr, const vint4& idx)
{
  vuint4 r;
  for (int i = 0; i < 4; ++i)
    r[i] = ptr[idx[i]];
  return r;
}

prt_inline vuint4 gather(const vbool4& mask, const uint* ptr, const vint4& idx)
{
  vuint4 r;
  for (int i = 0; i < 4; ++i)
    r[i] = mask[i] ? ptr[idx[i]] : 0;
  return r;
}

// Store functions
// ---------------

prt_inline void store(uint* ptr, const vuint4& a)
{
  vst1q_u32(ptr, a.m);
}

prt_inline void store(const vbool4& mask, uint* ptr, const vuint4& a)
{
  for (int i = 0; i < 4; ++i)
    if (mask[i]) ptr[i] = a[i];
}

prt_inline void compress(const vbool4& mask, uint* ptr, const vuint4& a)
{
  uint offset = 0;
  for (int i = 0; i < 4; ++i)
    if (mask[i]) ptr[offset++] = a[i];
}

prt_inline void scatter(uint* ptr, const vint4& idx, const vuint4& a)
{
  for (int i = 0; i < 4; ++i)
    ptr[idx[i]] = a[i];
}

prt_inline void scatter(const vbool4& mask, uint* ptr, const vint4& idx, const vuint4& a)
{
  for (int i = 0; i < 4; ++i)
    if (mask[i]) ptr[idx[i]] = a[i];
}

// Select function
// ---------------

prt_inline vuint4 select(const vbool4& mask, const vuint4& a, const vuint4& b)
{
  return vbslq_u32(mask.m, a.m, b.m);
}

// Shuffle functions
// -----------------

template <int i>
prt_inline uint extract(const vuint4& a)
{
  return vgetq_lane_u32(a.m, i);
}

template <int i>
prt_inline vuint4 broadcast(const vuint4& a)
{
  return vdupq_laneq_u32(a.m, i);
}

template <int i0, int i1, int i2, int i3>
prt_inline vuint4 permute4(const vuint4& a)
{
  vuint4 r;
  r[0] = a[i0]; r[1] = a[i1]; r[2] = a[i2]; r[3] = a[i3];
  return r;
}

// Arithmetic operators
// --------------------

prt_inline vuint4 operator +(const vuint4& a, const vuint4& b)
{
  return vaddq_u32(a.m, b.m);
}

prt_inline vuint4 operator -(const vuint4& a, const vuint4& b)
{
  return vsubq_u32(a.m, b.m);
}

prt_inline vuint4 operator *(const vuint4& a, const vuint4& b)
{
  return vmulq_u32(a.m, b.m);
}

// Bitwise operators
// -----------------

prt_inline vuint4 operator &(const vuint4& a, const vuint4& b)
{
  return vandq_u32(a.m, b.m);
}

prt_inline vuint4 operator |(const vuint4& a, const vuint4& b)
{
  return vorrq_u32(a.m, b.m);
}

prt_inline vuint4 operator ^(const vuint4& a, const vuint4& b)
{
  return veorq_u32(a.m, b.m);
}

prt_inline vuint4 andn(const vuint4& a, const vuint4& b)
{
  return vbicq_u32(a.m, b.m); // a & ~b
}

prt_inline vuint4 operator <<(const vuint4& a, int b)
{
  return vshlq_u32(a.m, vdupq_n_s32(b));
}

prt_inline vuint4 operator >>(const vuint4& a, int b)
{
  return vshlq_u32(a.m, vdupq_n_s32(-b)); // logical right shift
}

prt_inline vuint4 shl(const vuint4& a, int b)
{
  return vshlq_u32(a.m, vdupq_n_s32(b));
}

prt_inline vuint4 shr(const vuint4& a, int b)
{
  return vshlq_u32(a.m, vdupq_n_s32(-b)); // logical right shift
}

// Assignment operators
// --------------------

prt_inline vuint4& operator +=(vuint4& a, const vuint4& b) { return a = a + b; }
prt_inline vuint4& operator -=(vuint4& a, const vuint4& b) { return a = a - b; }
prt_inline vuint4& operator *=(vuint4& a, const vuint4& b) { return a = a * b; }
prt_inline vuint4& operator &=(vuint4& a, const vuint4& b) { return a = a & b; }
prt_inline vuint4& operator |=(vuint4& a, const vuint4& b) { return a = a | b; }
prt_inline vuint4& operator ^=(vuint4& a, const vuint4& b) { return a = a ^ b; }

prt_inline vuint4& operator <<=(vuint4& a, int b) { return a = a << b; }
prt_inline vuint4& operator >>=(vuint4& a, int b) { return a = a >> b; }

// Compare operators
// -----------------

prt_inline vbool4 operator ==(const vuint4& a, const vuint4& b)
{
  return vceqq_u32(a.m, b.m);
}

prt_inline vbool4 operator !=(const vuint4& a, const vuint4& b)
{
  return !(a == b);
}

// Compare functions
// -----------------

prt_inline vbool4 cmpEq(const vuint4& a, const vuint4& b) { return a == b; }
prt_inline vbool4 cmpNe(const vuint4& a, const vuint4& b) { return a != b; }

prt_inline vbool4 cmpEq(const vbool4& mask, const vuint4& a, const vuint4& b) { return (a == b) & mask; }
prt_inline vbool4 cmpNe(const vbool4& mask, const vuint4& a, const vuint4& b) { return (a != b) & mask; }

// Math functions
// --------------

prt_inline vuint4 min(const vuint4& a, const vuint4& b)
{
  return vminq_u32(a.m, b.m);
}

prt_inline vuint4 max(const vuint4& a, const vuint4& b)
{
  return vmaxq_u32(a.m, b.m);
}

} // namespace prt
