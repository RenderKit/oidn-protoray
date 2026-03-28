// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <climits>
#include <memory>
#include <functional>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <utility>
#include <optional>

// x86 intrinsics
#if defined(_MSC_VER)
#include <intrin.h>
#elif defined(__INTEL_COMPILER)
#include <ia32intrin.h>
#endif

#ifndef DEBUG
// Release
#if defined(_MSC_VER)
#define prt_inline __forceinline
#else
#define prt_inline inline __attribute__((always_inline))
#endif
#else
// Debug
#define prt_inline inline
#endif

#if defined(_MSC_VER)
#define LIKELY(expr) expr
#define UNLIKELY(expr) expr
#else
#define LIKELY(expr) __builtin_expect(expr,true)
#define UNLIKELY(expr) __builtin_expect(expr,false)
#endif

#define __STDC_LIMIT_MACROS

#ifdef PRT_PROFILE_MODE
#define prt_profile(expr) expr
#else
#define prt_profile(expr)
#endif

namespace prt {

typedef unsigned int uint;

using namespace ::std::placeholders;

// FIXME
using std::swap;

class Uncopyable
{
protected:
  Uncopyable() {}
  ~Uncopyable() {}

private:
  Uncopyable(const Uncopyable&);
  const Uncopyable& operator =(const Uncopyable&);
};

template <class T>
using ResultOfT = typename std::result_of<T>::type;

enum class Access
{
  Read,
  Write,
  ReadWrite,
  ReadWriteDiscard
};

// Select function
// ---------------

prt_inline int    select(bool p, int    a, int    b) { return p ? a : b; }
prt_inline uint   select(bool p, uint   a, uint   b) { return p ? a : b; }
prt_inline float  select(bool p, float  a, float  b) { return p ? a : b; }
prt_inline double select(bool p, double a, double b) { return p ? a : b; }

prt_inline void set(bool p, int&    a, int    b) { if (p) a = b; }
prt_inline void set(bool p, uint&   a, uint   b) { if (p) a = b; }
prt_inline void set(bool p, float&  a, float  b) { if (p) a = b; }
prt_inline void set(bool p, double& a, double b) { if (p) a = b; }

prt_inline void set(bool p, int*    a, int    b) { if (p) *a = b; }
prt_inline void set(bool p, uint*   a, uint   b) { if (p) *a = b; }
prt_inline void set(bool p, float*  a, float  b) { if (p) *a = b; }
prt_inline void set(bool p, double* a, double b) { if (p) *a = b; }

// Min/max functions
// -----------------

template <class T>
prt_inline T min(const T& a, const T& b)
{
  return (a < b) ? a : b;
}

template <class T>
prt_inline T min(const T& a, const T& b, const T& c)
{
  return min(min(a, b), c);
}

template <class T>
prt_inline T min(const T& a, const T& b, const T& c, const T& d)
{
  return min(min(a, b), min(c, d));
}

template <class T>
prt_inline T max(const T& a, const T& b)
{
  return (a > b) ? a : b;
}

template <class T>
prt_inline T max(const T& a, const T& b, const T& c)
{
  return max(max(a, b), c);
}

template <class T>
prt_inline T max(const T& a, const T& b, const T& c, const T& d)
{
  return max(max(a, b), max(c, d));
}

// Test functions
// --------------

prt_inline bool all(bool a) { return a; }
prt_inline bool any(bool a) { return a; }
prt_inline bool none(bool a) { return !a; }


template <class OutType, class InType>
prt_inline OutType bitwise_cast(const InType& value)
{
  static_assert(sizeof(InType) == sizeof(OutType), "bitwise_cast: sizes must match");
  OutType result;
  memcpy(&result, &value, sizeof(result));
  return result;
}

} // namespace prt
