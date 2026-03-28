// Copyright 2015 Intel Corporation
// Copyright 2011-2022 Blender Foundation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/constants.h"
#include "math_common.h"
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#include "math_neon.h"
#else
#include "math_avx2.h"
#endif

namespace prt {

using std::abs;
using std::sqrt;
using std::pow;
using std::exp;
using std::log;
using std::sin;
using std::cos;
using std::acos;
using std::tan;
using std::atan;
using std::atan2;
using std::isfinite;

template <class T>
prt_inline T sqr(const T& x)
{
  return x * x;
}

template <class T>
prt_inline T degToRad(T x)
{
  return x * (float(pi)/T(180.f));
}

template <class T>
prt_inline T radToDeg(T x)
{
  return x * (T(180.f)/float(pi));
}

template <class T>
prt_inline void sincos(const T& x, T& sinResult, T& cosResult)
{
  // FIXME
  sinResult = sin(x);
  cosResult = cos(x);
}

template <class T>
prt_inline T sin2cos(const T& x)
{
  return sqrt(max(T(one)-x*x, T(zero)));
}

template <class T>
prt_inline T cos2sin(const T& x)
{
  return sin2cos(x);
}

template <class T>
prt_inline T clamp(const T& value, const T& minValue, const T& maxValue)
{
  return min(max(value, minValue), maxValue);
}

// Clamp between 0 and 1
template <class T>
prt_inline T clamp(const T& value)
{
  return min(max(value, T(0)), T(1));
}

// Safe version of clamp: returns minValue if value is NaN
template <class T>
prt_inline T clampSafe(const T& value, const T& minValue, const T& maxValue)
{
  return min(select(minValue < value, value, minValue), maxValue);
}

template <class T, class S>
prt_inline T lerp(const T& a, const T& b, const S& s)
{
  return a + s * (b - a);
}

template <class T>
prt_inline T frac(const T& x)
{
  return x - floor(x);
}

template <class T>
prt_inline T boxStep(const T& min, const T& val)
{
  return select(val < min, T(zero), T(one));
}

template <class T>
prt_inline T smoothStep(const T& edge0, const T& edge1, const T& x)
{
  T t = clamp((x - edge0) / (edge1 - edge0), T(0.0f), T(1.0f));
  return t * t * (T(3.0f) - T(2.0f) * t);
}

// Does not return infinity for 0
prt_inline float rcpSafe(float x)
{
  return x == 0.f ? posMax : rcp(x);
}

// Does not return infinity for 0
prt_inline float rsqrtSafe(float x)
{
  return x == 0.f ? posMax : rsqrt(x);
}

// Does not return NaN
template <class T>
prt_inline T sqrtSafe(const T& x)
{
  return sqrt(max(x, T(zero)));
}

// Does not return NaN
template <class T>
prt_inline T acosSafe(const T& x)
{
  return acos(clamp(x, -T(one), T(one)));
}

prt_inline int shl(int a, int b)
{
  return uint(a) << uint(b);
}

prt_inline int shr(int a, int b)
{
  return uint(a) >> uint(b);
}

prt_inline uint shl(uint a, int b)
{
  return uint(a) << uint(b);
}

prt_inline uint shr(uint a, int b)
{
  return uint(a) >> uint(b);
}

prt_inline int bitTestAndComplement(int value, int index)
{
#if defined(_WIN32)
  long r = value;
  _bittestandcomplement(&r, index);
  return r;
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
  return value ^ (1 << index);
#else
  int r = 0;
  asm("btc %1,%0" : "=r"(r) : "r"(index), "0"(value) : "flags");
  return r;
#endif
}

prt_inline int bitScanAndComplement(int& value)
{
  int i = bitScan(value);
  value &= value-1; // BLSR
  return i;
}

prt_inline uint countLeadingZeros(const uint x)
{
  assert(x != 0);
#ifdef _MSC_VER
  unsigned long leading_zero = 0;
  _BitScanReverse(&leading_zero, x);
  return (31 - leading_zero);
#else
  return __builtin_clz(x);
#endif
}

prt_inline uint32_t reverseBits(uint32_t x)
{
  // Use a native instruction if it exists
#if defined(__aarch64__) || (defined(_M_ARM64) && !defined(_MSC_VER))
  // Assume the rbit is always available on 64bit ARM architecture
  __asm__("rbit %w0, %w1" : "=r"(x) : "r"(x));
  return x;
#elif __has_builtin(__builtin_bitreverse32)
  return __builtin_bitreverse32(x);
#else
  // Flip pairwise
  x = ((x & 0x55555555) << 1) | ((x & 0xAAAAAAAA) >> 1);
  // Flip pairs
  x = ((x & 0x33333333) << 2) | ((x & 0xCCCCCCCC) >> 2);
  // Flip nibbles
  x = ((x & 0x0F0F0F0F) << 4) | ((x & 0xF0F0F0F0) >> 4);
  // Flip bytes. CPUs have an instruction for that, pretty fast one.
  #ifdef _MSC_VER
  return _byteswap_ulong(x);
  #elif defined(__INTEL_COMPILER)
  return (uint32_t)_bswap((int)x);
  #else
  // Assuming gcc or clang
  return __builtin_bswap32(x);
  #endif
#endif
}

prt_inline bool isPowerOfTwo(int x)
{
  return (x & (x-1)) == 0;
}

prt_inline uint nextPowerOfTwo(uint x)
{
  return x == 0 ? 1 : 1 << (32 - countLeadingZeros(x));
}

template <class UInt>
prt_inline UInt expandBits(UInt x)
{
  x &= 0x0000ffff;
  x = (x ^ (x << 8)) & 0x00ff00ff;
  x = (x ^ (x << 4)) & 0x0f0f0f0f;
  x = (x ^ (x << 2)) & 0x33333333;
  x = (x ^ (x << 1)) & 0x55555555;
  return x;
}

template <class UInt>
prt_inline UInt morton2D(const UInt x, const UInt y)
{
  return (expandBits(x) << 1) | expandBits(y);
}

// Conversion functions
// --------------------

template <class T>
prt_inline float toFloat(const T& x) { return T(x); }

prt_inline float toFloatUnorm(uint x) { return toFloat(x) * 0x1.0p-32f; }

prt_inline float toFloatUnormExcl(uint x)
{
  /* NOTE: we divide by 4294967808 instead of 2^32 because the latter
   * leads to a [0.0, 1.0] mapping instead of [0.0, 1.0) due to floating
   * point rounding error. 4294967808 unfortunately leaves (precisely)
   * one unused ULP between the max number this outputs and 1.0, but
   * that's the best you can do with this construction. */
  return toFloat(x) * (1.0f / 4294967808.0f);
}

template <class T>
prt_inline int toInt(const T& x) { return T(x); }

template <class T>
prt_inline double toDouble(const T& x) { return T(x); }

prt_inline float asFloat(float x) { return x; }
prt_inline float asFloat(int x) { return bitwise_cast<float>(x); }
prt_inline int asInt(int x) { return x; }
prt_inline int asInt(float x) { return bitwise_cast<int>(x); }
prt_inline uint asUInt(uint x) { return x; }
prt_inline uint asUInt(float x) { return bitwise_cast<uint>(x); }

prt_inline int toIntSafe(float x)
{
  int xi = toInt(max(x, -2147483648.0f));
  xi = select(x >= 2147483648.0f, 2147483647, xi);
  return xi;
}

prt_inline uint16_t toHalf(float x)
{
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
  float16_t h = (float16_t)x;
  return bitwise_cast<uint16_t>(h);
#else
  return _cvtss_sh(x, _MM_FROUND_CUR_DIRECTION);
#endif
}

// Reduction functions
// -------------------

template <class T>
prt_inline T reduceMin(const T& x) { return x; }

template <class T>
prt_inline T reduceMax(const T& x) { return x; }

template <class T>
prt_inline T reduceAdd(const T& x) { return x; }

// Sequences
// ---------

inline float halton(int base, int index)
{
  float result = 0.f;
  float f = 1.f;
  while (index > 0)
  {
    f = f / float(base);
    result += f * float(index % base);
    index = index / base;
  }
  return result;
}

} // namespace prt
