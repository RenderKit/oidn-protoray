// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <iostream>
#include "math.h"
#include "simd.h"
#include "vec2.h"
#include "vec3.h"

namespace prt {

template <class T>
struct Vec4
{
  typedef T Scalar;
  static const int size = 4;

  T x, y, z, w;

  prt_inline Vec4() {}

  prt_inline Vec4(Zero) : x(zero), y(zero), z(zero), w(zero) {}
  prt_inline Vec4(One) : x(one), y(one), z(one), w(one) {}

  prt_inline Vec4(const Vec4<T>& a) : x(a.x), y(a.y), z(a.z), w(a.w) {}

  template <class T1>
  prt_inline Vec4(const Vec4<T1>& a) : x(T(a.x)), y(T(a.y)), z(T(a.z)), w(T(a.w)) {}

  prt_inline Vec4(const T& x, const T& y, const T& z, const T& w) : x(x), y(y), z(z), w(w) {}

  prt_inline Vec4(const Vec3<T>& xyz, const T& w = 0) : x(xyz.x), y(xyz.y), z(xyz.z), w(w) {}

  prt_inline Vec4(const T& a) : x(a), y(a), z(a), w(a) {}

  prt_inline Vec4<T>& operator =(const Vec4<T>& a)
  {
    x = a.x;
    y = a.y;
    z = a.z;
    w = a.w;
    return *this;
  }

  prt_inline T& operator [](size_t i)
  {
    assert(i < 4);
    return (&x)[i];
  }

  prt_inline const T& operator [](size_t i) const
  {
    assert(i < 4);
    return (&x)[i];
  }

  prt_inline Vec2<T> xy() const
  {
    return Vec2<T>(x, y);
  }

  prt_inline Vec3<T> xyz() const
  {
    return Vec3<T>(x, y, z);
  }

  prt_inline Vec3<T> xyw() const
  {
    return Vec3<T>(x, y, w);
  }

  prt_inline Vec3<T> xzw() const
  {
    return Vec3<T>(x, z, w);
  }

  prt_inline const T* getData() const
  {
    return &x;
  }

  prt_inline T* getData()
  {
    return &x;
  }
};

// Typedefs
// --------

typedef Vec4<float> Vec4f;
typedef Vec4<double> Vec4d;
typedef Vec4<int> Vec4i;
typedef Vec4<uint8_t> Vec4uc;
typedef Vec4<bool> Vec4b;

template <class T, int N = simdSize>
using Vec4v = Vec4<var<T,N>>;

typedef Vec4<vfloat>   Vec4vf;
typedef Vec4<vfloat4>  Vec4vf4;
typedef Vec4<vfloat8>  Vec4vf8;
typedef Vec4<vfloat16> Vec4vf16;

typedef Vec4<vint>   Vec4vi;
typedef Vec4<vint4>  Vec4vi4;
typedef Vec4<vint8>  Vec4vi8;
typedef Vec4<vint16> Vec4vi16;

// Traits
// ------

template <class T>
struct ToFloat<Vec4<T>> { typedef Vec4<ToFloatT<T>> Type; };

template <class T>
struct ToInt<Vec4<T>> { typedef Vec4<ToIntT<T>> Type; };

template <class T>
struct ToUInt<Vec4<T>> { typedef Vec4<ToUIntT<T>> Type; };

template <class T>
struct ToDouble<Vec4<T>> { typedef Vec4<ToDoubleT<T>> Type; };

template <class T>
struct ToBool<Vec4<T>> { typedef Vec4<ToBoolT<T>> Type; };

// Conversion functions
// --------------------

template <class T>
prt_inline ToFloatT<Vec4<T>> toFloat(const Vec4<T>& a)
{
  return ToFloatT<Vec4<T>>(toFloat(a.x), toFloat(a.y), toFloat(a.z), toFloat(a.w));
}

template <class T>
prt_inline ToIntT<Vec4<T>> toInt(const Vec4<T>& a)
{
  return ToIntT<Vec4<T>>(toInt(a.x), toInt(a.y), toInt(a.z), toInt(a.w));
}

template <class T>
prt_inline ToDoubleT<Vec4<T>> toDouble(const Vec4<T>& a)
{
  return ToDoubleT<Vec4<T>>(toDouble(a.x), toDouble(a.y), toDouble(a.z), toDouble(a.w));
}

template <class T>
prt_inline ToFloatT<Vec4<T>> asFloat(const Vec4<T>& a)
{
  return ToFloatT<Vec4<T>>(asFloat(a.x), asFloat(a.y), asFloat(a.z), asFloat(a.w));
}

template <class T>
prt_inline ToIntT<Vec4<T>> asInt(const Vec4<T>& a)
{
  return ToIntT<Vec4<T>>(asInt(a.x), asInt(a.y), asInt(a.z), asInt(a.w));
}

template <class T>
prt_inline ToUIntT<Vec4<T>> asUInt(const Vec4<T>& a)
{
  return ToUIntT<Vec4<T>>(asUInt(a.x), asUInt(a.y), asUInt(a.z), asUInt(a.w));
}

template <class T, int N>
prt_inline Vec4<T> toScalar(const Vec4v<T,N>& a)
{
  return Vec4<T>(toScalar(a.x), toScalar(a.y), toScalar(a.z), toScalar(a.w));
}

// Selection functions
// -------------------

template <class T, class P>
prt_inline Vec4<T> select(const P& p, const Vec4<T>& a, const Vec4<T>& b)
{
  return Vec4<T>(select(p, a.x, b.x), select(p, a.y, b.y), select(p, a.z, b.z), select(p, a.w, b.w));
}

template <class T, class P>
prt_inline void set(const P& p, Vec4<T>& a, const Vec4<T>& b)
{
  set(p, a.x, b.x);
  set(p, a.y, b.y);
  set(p, a.z, b.z);
  set(p, a.w, b.w);
}

template <class T, class P>
prt_inline void set(const P& p, Vec4<T>* a, const Vec4<T>& b)
{
  set(p, &a->x, b.x);
  set(p, &a->y, b.y);
  set(p, &a->z, b.z);
  set(p, &a->w, b.w);
}

// Arithmetic operators
// --------------------

template <class T>
prt_inline Vec4<T> operator -(const Vec4<T>& a)
{
  return Vec4<T>(-a.x, -a.y, -a.z, -a.w);
}

template <class T>
prt_inline Vec4<T> operator +(const Vec4<T>& a, const Vec4<T>& b)
{
  return Vec4<T>(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

template <class T>
prt_inline Vec4<T> operator +(const Vec4<T>& a, const T& b)
{
  return Vec4<T>(a.x + b, a.y + b, a.z + b, a.w + b);
}

template <class T>
prt_inline Vec4<T> operator +(const T& a, const Vec4<T>& b)
{
  return Vec4<T>(a + b.x, a + b.y, a + b.z, a + b.w);
}

template <class T>
prt_inline Vec4<T> operator -(const Vec4<T>& a, const Vec4<T>& b)
{
  return Vec4<T>(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}

template <class T>
prt_inline Vec4<T> operator -(const Vec4<T>& a, const T& b)
{
  return Vec4<T>(a.x - b, a.y - b, a.z - b, a.w - b);
}

template <class T>
prt_inline Vec4<T> operator -(const T& a, const Vec4<T>& b)
{
  return Vec4<T>(a - b.x, a - b.y, a - b.z, a - b.w);
}

template <class T>
prt_inline Vec4<T> operator *(const Vec4<T>& a, const Vec4<T>& b)
{
  return Vec4<T>(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
}

template <class T>
prt_inline Vec4<T> operator *(const Vec4<T>& a, const T& b)
{
  return Vec4<T>(a.x * b, a.y * b, a.z * b, a.w * b);
}

template <class T>
prt_inline Vec4<T> operator *(const T& a, const Vec4<T>& b)
{
  return Vec4<T>(a * b.x, a * b.y, a * b.z, a * b.w);
}

template <class T>
prt_inline Vec4<T> operator /(const Vec4<T>& a, const Vec4<T>& b)
{
  return Vec4<T>(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w);
}

template <class T>
prt_inline Vec4<T> operator /(const Vec4<T>& a, const T& b)
{
  return Vec4<T>(a.x / b, a.y / b, a.z / b, a.w / b);
}

template <class T>
prt_inline Vec4<T> operator /(const T& a, const Vec4<T>& b)
{
  return Vec4<T>(a / b.x, a / b.y, a / b.z, a / b.w);
}

// Assignment operators
// --------------------

template <class T> prt_inline Vec4<T>& operator +=(Vec4<T>& a, const Vec4<T>& b) { return a = a + b; }
template <class T> prt_inline Vec4<T>& operator +=(Vec4<T>& a, const T& b)       { return a = a + b; }
template <class T> prt_inline Vec4<T>& operator -=(Vec4<T>& a, const Vec4<T>& b) { return a = a - b; }
template <class T> prt_inline Vec4<T>& operator -=(Vec4<T>& a, const T& b)       { return a = a - b; }
template <class T> prt_inline Vec4<T>& operator *=(Vec4<T>& a, const Vec4<T>& b) { return a = a * b; }
template <class T> prt_inline Vec4<T>& operator *=(Vec4<T>& a, const T& b)       { return a = a * b; }
template <class T> prt_inline Vec4<T>& operator /=(Vec4<T>& a, const Vec4<T>& b) { return a = a / b; }
template <class T> prt_inline Vec4<T>& operator /=(Vec4<T>& a, const T& b)       { return a = a / b; }

// Compare operators
// -----------------

template <class T>
prt_inline bool operator ==(const Vec4<T>& a, const Vec4<T>& b)
{
  return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}

template <class T>
prt_inline bool operator !=(const Vec4<T>& a, const Vec4<T>& b)
{
  return a.x != b.x || a.y != b.y || a.z != b.z || a.w != b.w;
}

// Math functions
// --------------

template <class T>
prt_inline Vec4<T> abs(const Vec4<T>& a)
{
  return Vec4<T>(abs(a.x), abs(a.y), abs(a.z), abs(a.w));
}

template <class T>
prt_inline Vec4<T> rcp(const Vec4<T>& a)
{
  return Vec4<T>(rcp(a.x), rcp(a.y), rcp(a.z), rcp(a.w));
}

template <class T>
prt_inline Vec4<T> rcpSafe(const Vec4<T>& a)
{
  return Vec4<T>(rcpSafe(a.x), rcpSafe(a.y), rcpSafe(a.z), rcpSafe(a.w));
}

template <class T>
prt_inline Vec4<T> sqrt(const Vec4<T>& a)
{
  return Vec4<T>(sqrt(a.x), sqrt(a.y), sqrt(a.z), sqrt(a.w));
}

template <class T>
prt_inline Vec4<T> floor(const Vec4<T>& a)
{
  return Vec4<T>(floor(a.x), floor(a.y), floor(a.z), floor(a.w));
}

template <class T>
prt_inline Vec4<T> min(const Vec4<T>& a, const Vec4<T>& b)
{
  return Vec4<T>(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z), min(a.w, b.w));
}

template <class T>
prt_inline Vec4<T> max(const Vec4<T>& a, const Vec4<T>& b)
{
  return Vec4<T>(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z), max(a.w, b.w));
}

template <class T>
prt_inline Vec4<T> clamp(const Vec4<T>& value, const T& minValue, const T& maxValue)
{
  return Vec4<T>(clamp(value.x, minValue, maxValue),
                 clamp(value.y, minValue, maxValue),
                 clamp(value.z, minValue, maxValue),
                 clamp(value.w, minValue, maxValue));
}

template <class T>
prt_inline Vec4<T> clampSafe(const Vec4<T>& value, const T& minValue, const T& maxValue)
{
  return Vec4<T>(clampSafe(value.x, minValue, maxValue),
                 clampSafe(value.y, minValue, maxValue),
                 clampSafe(value.z, minValue, maxValue),
                 clampSafe(value.w, minValue, maxValue));
}

template <class T>
prt_inline ToBoolT<Vec4<T>> isfinite(const Vec4<T>& a)
{
  return ToBoolT<Vec4<T>>(isfinite(a.x), isfinite(a.y), isfinite(a.z), isfinite(a.w));
}

// Vector math functions
// ---------------------

template <class T>
prt_inline T dot(const Vec4<T>& a, const Vec4<T>& b)
{
  return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

template <class T>
prt_inline T lengthSqr(const Vec4<T>& a)
{
  return dot(a, a);
}

template <class T>
prt_inline T length(const Vec4<T>& a)
{
  return sqrt(dot(a, a));
}

template <class T>
prt_inline T lengthRcp(const Vec4<T>& a)
{
  return rsqrt(dot(a, a));
}

template <class T>
prt_inline T lengthRcpSafe(const Vec4<T>& a)
{
  return rsqrtSafe(dot(a, a));
}

template <class T>
prt_inline Vec4<T> normalize(const Vec4<T>& a)
{
  return a * rsqrt(dot(a, a));
}

// Reduction functions
// -------------------

template <class T>
prt_inline T reduceMin(const Vec4<T>& a)
{
  return min(min(a.x, a.y), min(a.z, a.w));
}

template <class T>
prt_inline T reduceMax(const Vec4<T>& a)
{
  return max(max(a.x, a.y), max(a.z, a.w));
}

template <class T>
prt_inline T reduceAdd(const Vec4<T>& a)
{
  return a.x + a.y + a.z + a.w;
}

// Test functions
// --------------

template <class T>
prt_inline bool all(const Vec4<T>& a)
{
  return all(a.x) && all(a.y) && all(a.z) && all(a.w);
}

template <class T>
prt_inline bool any(const Vec4<T>& a)
{
  return any(a.x) || any(a.y) || any(a.z) || any(a.w);
}

template <class T>
prt_inline bool none(const Vec4<T>& a)
{
  return none(a.x) && none(a.y) && none(a.z) && none(a.w);
}

// Stream operators
// ----------------

template <class T>
prt_inline std::ostream& operator <<(std::ostream& osm, const Vec4<T>& a)
{
  osm << a.x << "," << a.y << "," << a.z << "," << a.w;
  return osm;
}

template <class T>
prt_inline std::istream& operator >>(std::istream& ism, Vec4<T>& a)
{
  ism >> a.x;
  if (ism.peek() == ',')
  {
    ism.ignore();
  }
  else
  {
    a.y = a.z = a.w = a.x;
    return ism;
  }

  ism >> a.y;
  if (ism.peek() == ',') ism.ignore();
  ism >> a.z;
  if (ism.peek() == ',') ism.ignore();
  ism >> a.w;
  return ism;
}

// SIMD functions
// --------------

template <class T, int N>
Vec4v<T,N> gather4(const var<bool,N>& mask, const T* ptr, const var<int,N>& idx)
{
  return Vec4v<T,N>(gather(mask, ptr,   idx),
                    gather(mask, ptr+1, idx),
                    gather(mask, ptr+2, idx),
                    gather(mask, ptr+3, idx));
}

} // namespace prt
