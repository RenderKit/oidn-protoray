// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <immintrin.h>
#include "../simd_common.h"

namespace prt {

// vbool16
template <>
struct var<bool,16>
{
  __mmask16 m;

  prt_inline var() {}

  prt_inline var(const vbool16& a) : m(a.m) {}
  prt_inline var(__mmask16 a) : m(a) {}
  prt_inline var(int a) : m(_mm512_int2mask(a)) {}

  prt_inline var(Zero) : m(_mm512_int2mask(0)) {}
  prt_inline var(One)  : m(_mm512_int2mask(0xffff)) {}

  prt_inline vbool16& operator =(const vbool16& a)
  {
    m = a.m;
    return *this;
  }

  prt_inline operator int() const
  {
    return _mm512_mask2int(m);
  }

  prt_inline bool operator [](size_t i) const
  {
    assert(i < 16);
    return (_mm512_mask2int(m) >> int(i)) & 1;
  }
};

// Conversions
// -----------

prt_inline int toIntMask(const vbool16& a)
{
  return _mm512_mask2int(a.m);
}

// Select function
// ---------------

prt_inline vbool16 select(const vbool16& mask, const vbool16& a, const vbool16& b)
{
  return _mm512_kor(_mm512_kand(mask.m, a.m), _mm512_kandn(mask.m, b.m));
}

// Bitwise operators
// -----------------

prt_inline vbool16 operator !(const vbool16& a)
{
  return _mm512_knot(a.m);
}

prt_inline vbool16 operator &(const vbool16& a, const vbool16& b)
{
  return _mm512_kand(a.m, b.m);
}

prt_inline vbool16 operator |(const vbool16& a, const vbool16& b)
{
  return _mm512_kor(a.m, b.m);
}

prt_inline vbool16 operator ^(const vbool16& a, const vbool16& b)
{
  return _mm512_kxor(a.m, b.m);
}

prt_inline vbool16 andn(const vbool16& a, const vbool16& b)
{
  return _mm512_kandn(b.m, a.m);
}

// Assignment operators
// --------------------

prt_inline vbool16& operator &=(vbool16& a, const vbool16& b) { return a = a & b; }
prt_inline vbool16& operator |=(vbool16& a, const vbool16& b) { return a = a | b; }
prt_inline vbool16& operator ^=(vbool16& a, const vbool16& b) { return a = a ^ b; }

// Test functions
// --------------

prt_inline bool all(const vbool16& a)
{
  return _mm512_kortestc(a.m, a.m);
}

prt_inline bool any(const vbool16& a)
{
  return a.m != 0;
}

prt_inline bool none(const vbool16& a)
{
  return a.m == 0;
}

// Get/set functions
// -----------------

prt_inline bool get(const vbool16& a, size_t i)
{
  assert(i < 16);
  return (toIntMask(a) >> int(i)) & 1;
}

prt_inline void set(vbool16& a, size_t i)
{
  assert(i < 16);
  a |= 1 << int(i);
}

prt_inline void clear(vbool16& a, size_t i)
{
  assert(i < 16);
  a = andn(a, 1 << int(i));
}

} // namespace prt
