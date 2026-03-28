// Copyright 2026 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <arm_neon.h>
#include "../simd_common.h"
#include "vbool8_neon.h"
#include "vint8_neon.h"

namespace prt {

// vuint8 - double-pumped using two uint32x4_t
template <>
struct var<uint,8>
{
  union
  {
    struct { uint32x4_t lo, hi; };
    uint v[8];
  };

  prt_inline var() {}

  prt_inline var(const vuint8& a) : lo(a.lo), hi(a.hi) {}
  prt_inline var(const vint8& a) : lo(vreinterpretq_u32_s32(a.lo)), hi(vreinterpretq_u32_s32(a.hi)) {}
  prt_inline var(uint32x4_t a0, uint32x4_t a1) : lo(a0), hi(a1) {}
  prt_inline var(uint a) : lo(vdupq_n_u32(a)), hi(vdupq_n_u32(a)) {}
  prt_inline var(uint a0, uint a1, uint a2, uint a3, uint a4, uint a5, uint a6, uint a7)
  {
    uint dlo[4] = {a0,a1,a2,a3};
    uint dhi[4] = {a4,a5,a6,a7};
    lo = vld1q_u32(dlo);
    hi = vld1q_u32(dhi);
  }

  prt_inline var(Zero) : lo(vdupq_n_u32(0)), hi(vdupq_n_u32(0)) {}
  prt_inline var(One)  : lo(vdupq_n_u32(1)), hi(vdupq_n_u32(1)) {}
  prt_inline var(Step)
  {
    uint dlo[4] = {0,1,2,3};
    uint dhi[4] = {4,5,6,7};
    lo = vld1q_u32(dlo);
    hi = vld1q_u32(dhi);
  }

  prt_inline vuint8& operator =(const vuint8& a)
  {
    lo = a.lo;
    hi = a.hi;
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

prt_inline vint8::var(const vuint8& a) : lo(vreinterpretq_s32_u32(a.lo)), hi(vreinterpretq_s32_u32(a.hi)) {}

// Load functions
// --------------

template <>
prt_inline vuint8 load(const uint* ptr)
{
  return vuint8(vld1q_u32(ptr), vld1q_u32(ptr+4));
}

template <>
prt_inline vuint8 load(const vbool8& mask, const uint* ptr)
{
  vuint8 r;
  for (int i = 0; i < 8; ++i)
    if (mask[i]) r[i] = ptr[i];
  return r;
}

template <>
prt_inline vuint8 loadu(const uint* ptr)
{
  return vuint8(vld1q_u32(ptr), vld1q_u32(ptr+4));
}

template <>
prt_inline vuint8 loadu(const vbool8& mask, const uint* ptr)
{
  vuint8 r;
  for (int i = 0; i < 8; ++i)
    if (mask[i]) r[i] = ptr[i];
  return r;
}

prt_inline vuint8 expand(const vbool8& mask, const uint* ptr)
{
  vuint8 r;
  int offset = 0;
  for (int i = 0; i < 8; ++i)
    if (mask[i]) r[i] = ptr[offset++];
  return r;
}

prt_inline vuint8 gather(const uint* ptr, const vint8& idx)
{
  vuint8 r;
  for (int i = 0; i < 8; ++i)
    r[i] = ptr[idx[i]];
  return r;
}

prt_inline vuint8 gather(const vbool8& mask, const uint* ptr, const vint8& idx)
{
  vuint8 r(zero);
  for (int i = 0; i < 8; ++i)
    if (mask[i]) r[i] = ptr[idx[i]];
  return r;
}

// Store functions
// ---------------

prt_inline void store(uint* ptr, const vuint8& a)
{
  vst1q_u32(ptr, a.lo);
  vst1q_u32(ptr+4, a.hi);
}

prt_inline void store(const vbool8& mask, uint* ptr, const vuint8& a)
{
  for (int i = 0; i < 8; ++i)
    if (mask[i]) ptr[i] = a[i];
}

prt_inline void storeu(uint* ptr, const vuint8& a)
{
  vst1q_u32(ptr, a.lo);
  vst1q_u32(ptr+4, a.hi);
}

prt_inline void storeu(const vbool8& mask, uint* ptr, const vuint8& a)
{
  for (int i = 0; i < 8; ++i)
    if (mask[i]) ptr[i] = a[i];
}

prt_inline void compress(const vbool8& mask, uint* ptr, const vuint8& a)
{
  int offset = 0;
  for (int i = 0; i < 8; ++i)
    if (mask[i]) ptr[offset++] = a[i];
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
  return vuint8(vbslq_u32(mask.lo, a.lo, b.lo), vbslq_u32(mask.hi, a.hi, b.hi));
}

// Arithmetic operators
// --------------------

prt_inline vuint8 operator +(const vuint8& a, const vuint8& b)
{
  return vuint8(vaddq_u32(a.lo, b.lo), vaddq_u32(a.hi, b.hi));
}

prt_inline vuint8 operator -(const vuint8& a, const vuint8& b)
{
  return vuint8(vsubq_u32(a.lo, b.lo), vsubq_u32(a.hi, b.hi));
}

prt_inline vuint8 operator *(const vuint8& a, const vuint8& b)
{
  return vuint8(vmulq_u32(a.lo, b.lo), vmulq_u32(a.hi, b.hi));
}

// Bitwise operators
// -----------------

prt_inline vuint8 operator &(const vuint8& a, const vuint8& b)
{
  return vuint8(vandq_u32(a.lo, b.lo), vandq_u32(a.hi, b.hi));
}

prt_inline vuint8 operator |(const vuint8& a, const vuint8& b)
{
  return vuint8(vorrq_u32(a.lo, b.lo), vorrq_u32(a.hi, b.hi));
}

prt_inline vuint8 operator ^(const vuint8& a, const vuint8& b)
{
  return vuint8(veorq_u32(a.lo, b.lo), veorq_u32(a.hi, b.hi));
}

prt_inline vuint8 andn(const vuint8& a, const vuint8& b)
{
  return vuint8(vbicq_u32(a.lo, b.lo), vbicq_u32(a.hi, b.hi));
}

prt_inline vuint8 operator <<(const vuint8& a, int b)
{
  int32x4_t sv = vdupq_n_s32(b);
  return vuint8(vshlq_u32(a.lo, sv), vshlq_u32(a.hi, sv));
}

prt_inline vuint8 operator >>(const vuint8& a, int b)
{
  int32x4_t sv = vdupq_n_s32(-b);
  return vuint8(vshlq_u32(a.lo, sv), vshlq_u32(a.hi, sv));
}

prt_inline vuint8 shl(const vuint8& a, int b)
{
  int32x4_t sv = vdupq_n_s32(b);
  return vuint8(vshlq_u32(a.lo, sv), vshlq_u32(a.hi, sv));
}

prt_inline vuint8 shr(const vuint8& a, int b)
{
  int32x4_t sv = vdupq_n_s32(-b);
  return vuint8(vshlq_u32(a.lo, sv), vshlq_u32(a.hi, sv));
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
  return vbool8(vceqq_u32(a.lo, b.lo), vceqq_u32(a.hi, b.hi));
}

prt_inline vbool8 operator !=(const vuint8& a, const vuint8& b)
{
  return !(a == b);
}

// Compare functions
// -----------------

prt_inline vbool8 cmpEq(const vuint8& a, const vuint8& b) { return a == b; }
prt_inline vbool8 cmpNe(const vuint8& a, const vuint8& b) { return a != b; }

prt_inline vbool8 cmpEq(const vbool8& mask, const vuint8& a, const vuint8& b) { return (a == b) & mask; }
prt_inline vbool8 cmpNe(const vbool8& mask, const vuint8& a, const vuint8& b) { return (a != b) & mask; }

// Math functions
// --------------

prt_inline vuint8 min(const vuint8& a, const vuint8& b)
{
  return vuint8(vminq_u32(a.lo, b.lo), vminq_u32(a.hi, b.hi));
}

prt_inline vuint8 max(const vuint8& a, const vuint8& b)
{
  return vuint8(vmaxq_u32(a.lo, b.lo), vmaxq_u32(a.hi, b.hi));
}

} // namespace prt
