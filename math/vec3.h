// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <iostream>
#include "math.h"
#include "simd.h"
#include "vec2.h"

namespace prt {

template <class T>
struct Vec3
{
  typedef T Scalar;
  static const int size = 3;

  T x, y, z;

  prt_inline Vec3() {}

  prt_inline Vec3(Zero) : x(zero), y(zero), z(zero) {}
  prt_inline Vec3(One) : x(one), y(one), z(one) {}

  prt_inline Vec3(const Vec3<T>& a) : x(a.x), y(a.y), z(a.z) {}

  template <class T1>
  prt_inline Vec3(const Vec3<T1>& a) : x(T(a.x)), y(T(a.y)), z(T(a.z)) {}

  prt_inline Vec3(const T& x, const T& y, const T& z) : x(x), y(y), z(z) {}

  prt_inline Vec3(const T& a) : x(a), y(a), z(a) {}

  prt_inline Vec3<T>& operator =(const Vec3<T>& a)
  {
    x = a.x;
    y = a.y;
    z = a.z;
    return *this;
  }

  prt_inline T& operator [](size_t i)
  {
    assert(i < 3);
    return (&x)[i];
  }

  prt_inline const T& operator [](size_t i) const
  {
    assert(i < 3);
    return (&x)[i];
  }

  prt_inline const T* getData() const
  {
    return &x;
  }

  prt_inline T* getData()
  {
    return &x;
  }

  prt_inline Vec2<T> xy() const { return Vec2<T>(x, y); }
};

// Typedefs
// --------

typedef Vec3<float> Vec3f;
typedef Vec3<double> Vec3d;
typedef Vec3<int> Vec3i;
typedef Vec3<uint8_t> Vec3uc;
typedef Vec3<bool> Vec3b;

template <class T, int N = simdSize>
using Vec3v = Vec3<var<T,N>>;

typedef Vec3<vfloat>   Vec3vf;
typedef Vec3<vfloat4>  Vec3vf4;
typedef Vec3<vfloat8>  Vec3vf8;
typedef Vec3<vfloat16> Vec3vf16;

typedef Vec3<vint>   Vec3vi;
typedef Vec3<vint4>  Vec3vi4;
typedef Vec3<vint8>  Vec3vi8;
typedef Vec3<vint16> Vec3vi16;

// Traits
// ------

template <class T>
struct ToFloat<Vec3<T>> { typedef Vec3<ToFloatT<T>> Type; };

template <class T>
struct ToInt<Vec3<T>> { typedef Vec3<ToIntT<T>> Type; };

template <class T>
struct ToUInt<Vec3<T>> { typedef Vec3<ToUIntT<T>> Type; };

template <class T>
struct ToDouble<Vec3<T>> { typedef Vec3<ToDoubleT<T>> Type; };

template <class T>
struct ToBool<Vec3<T>> { typedef Vec3<ToBoolT<T>> Type; };

// Conversion functions
// --------------------

template <class T>
prt_inline ToFloatT<Vec3<T>> toFloat(const Vec3<T>& a)
{
  return ToFloatT<Vec3<T>>(toFloat(a.x), toFloat(a.y), toFloat(a.z));
}

template <class T>
prt_inline ToIntT<Vec3<T>> toInt(const Vec3<T>& a)
{
  return ToIntT<Vec3<T>>(toInt(a.x), toInt(a.y), toInt(a.z));
}

template <class T>
prt_inline ToDoubleT<Vec3<T>> toDouble(const Vec3<T>& a)
{
  return ToDoubleT<Vec3<T>>(toDouble(a.x), toDouble(a.y), toDouble(a.z));
}

template <class T>
prt_inline ToFloatT<Vec3<T>> asFloat(const Vec3<T>& a)
{
  return ToFloatT<Vec3<T>>(asFloat(a.x), asFloat(a.y), asFloat(a.z));
}

template <class T>
prt_inline ToIntT<Vec3<T>> asInt(const Vec3<T>& a)
{
  return ToIntT<Vec3<T>>(asInt(a.x), asInt(a.y), asInt(a.z));
}

template <class T>
prt_inline ToUIntT<Vec3<T>> asUInt(const Vec3<T>& a)
{
  return ToUIntT<Vec3<T>>(asUInt(a.x), asUInt(a.y), asUInt(a.z));
}

template <class T, int N>
prt_inline Vec3<T> toScalar(const Vec3v<T,N>& a)
{
  return Vec3<T>(toScalar(a.x), toScalar(a.y), toScalar(a.z));
}

// Selection functions
// -------------------

template <class T, class P>
prt_inline Vec3<T> select(const P& p, const Vec3<T>& a, const Vec3<T>& b)
{
  return Vec3<T>(select(p, a.x, b.x), select(p, a.y, b.y), select(p, a.z, b.z));
}

template <class T, class P>
prt_inline void set(const P& p, Vec3<T>& a, const Vec3<T>& b)
{
  set(p, a.x, b.x);
  set(p, a.y, b.y);
  set(p, a.z, b.z);
}

template <class T, class P>
prt_inline void set(const P& p, Vec3<T>* a, const Vec3<T>& b)
{
  set(p, &a->x, b.x);
  set(p, &a->y, b.y);
  set(p, &a->z, b.z);
}

// Arithmetic operators
// --------------------

template <class T>
prt_inline Vec3<T> operator -(const Vec3<T>& a)
{
  return Vec3<T>(-a.x, -a.y, -a.z);
}

template <class T>
prt_inline Vec3<T> operator +(const Vec3<T>& a, const Vec3<T>& b)
{
  return Vec3<T>(a.x + b.x, a.y + b.y, a.z + b.z);
}

template <class T>
prt_inline Vec3<T> operator +(const Vec3<T>& a, const T& b)
{
  return Vec3<T>(a.x + b, a.y + b, a.z + b);
}

template <class T>
prt_inline Vec3<T> operator +(const T& a, const Vec3<T>& b)
{
  return Vec3<T>(a + b.x, a + b.y, a + b.z);
}

template <class T>
prt_inline Vec3<T> operator -(const Vec3<T>& a, const Vec3<T>& b)
{
  return Vec3<T>(a.x - b.x, a.y - b.y, a.z - b.z);
}

template <class T>
prt_inline Vec3<T> operator -(const Vec3<T>& a, const T& b)
{
  return Vec3<T>(a.x - b, a.y - b, a.z - b);
}

template <class T>
prt_inline Vec3<T> operator -(const T& a, const Vec3<T>& b)
{
  return Vec3<T>(a - b.x, a - b.y, a - b.z);
}

template <class T>
prt_inline Vec3<T> operator *(const Vec3<T>& a, const Vec3<T>& b)
{
  return Vec3<T>(a.x * b.x, a.y * b.y, a.z * b.z);
}

template <class T>
prt_inline Vec3<T> operator *(const Vec3<T>& a, const T& b)
{
  return Vec3<T>(a.x * b, a.y * b, a.z * b);
}

template <class T>
prt_inline Vec3<T> operator *(const T& a, const Vec3<T>& b)
{
  return Vec3<T>(a * b.x, a * b.y, a * b.z);
}

template <class T>
prt_inline Vec3<T> operator /(const Vec3<T>& a, const Vec3<T>& b)
{
  return Vec3<T>(a.x / b.x, a.y / b.y, a.z / b.z);
}

template <class T>
prt_inline Vec3<T> operator /(const Vec3<T>& a, const T& b)
{
  return Vec3<T>(a.x / b, a.y / b, a.z / b);
}

template <class T>
prt_inline Vec3<T> operator /(const T& a, const Vec3<T>& b)
{
  return Vec3<T>(a / b.x, a / b.y, a / b.z);
}

// Assignment operators
// --------------------

template <class T> prt_inline Vec3<T>& operator +=(Vec3<T>& a, const Vec3<T>& b) { return a = a + b; }
template <class T> prt_inline Vec3<T>& operator +=(Vec3<T>& a, const T& b)       { return a = a + b; }
template <class T> prt_inline Vec3<T>& operator -=(Vec3<T>& a, const Vec3<T>& b) { return a = a - b; }
template <class T> prt_inline Vec3<T>& operator -=(Vec3<T>& a, const T& b)       { return a = a - b; }
template <class T> prt_inline Vec3<T>& operator *=(Vec3<T>& a, const Vec3<T>& b) { return a = a * b; }
template <class T> prt_inline Vec3<T>& operator *=(Vec3<T>& a, const T& b)       { return a = a * b; }
template <class T> prt_inline Vec3<T>& operator /=(Vec3<T>& a, const Vec3<T>& b) { return a = a / b; }
template <class T> prt_inline Vec3<T>& operator /=(Vec3<T>& a, const T& b)       { return a = a / b; }

// Compare operators
// -----------------

template <class T>
prt_inline bool operator ==(const Vec3<T>& a, const Vec3<T>& b)
{
  return a.x == b.x && a.y == b.y && a.z == b.z;
}

template <class T>
prt_inline bool operator !=(const Vec3<T>& a, const Vec3<T>& b)
{
  return a.x != b.x || a.y != b.y || a.z != b.z;
}

// Math functions
// --------------

template <class T>
prt_inline Vec3<T> abs(const Vec3<T>& a)
{
  return Vec3<T>(abs(a.x), abs(a.y), abs(a.z));
}

template <class T>
prt_inline Vec3<T> rcp(const Vec3<T>& a)
{
  return Vec3<T>(rcp(a.x), rcp(a.y), rcp(a.z));
}

template <class T>
prt_inline Vec3<T> rcpSafe(const Vec3<T>& a)
{
  return Vec3<T>(rcpSafe(a.x), rcpSafe(a.y), rcpSafe(a.z));
}

template <class T>
prt_inline Vec3<T> sqrt(const Vec3<T>& a)
{
  return Vec3<T>(sqrt(a.x), sqrt(a.y), sqrt(a.z));
}

template <class T>
prt_inline Vec3<T> exp(const Vec3<T>& a)
{
  return Vec3<T>(exp(a.x), exp(a.y), exp(a.z));
}

template <class T>
prt_inline Vec3<T> floor(const Vec3<T>& a)
{
  return Vec3<T>(floor(a.x), floor(a.y), floor(a.z));
}

template <class T>
prt_inline Vec3<T> min(const Vec3<T>& a, const Vec3<T>& b)
{
  return Vec3<T>(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z));
}

template <class T>
prt_inline Vec3<T> max(const Vec3<T>& a, const Vec3<T>& b)
{
  return Vec3<T>(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z));
}

template <class T>
prt_inline Vec3<T> clamp(const Vec3<T>& value, const T& minValue, const T& maxValue)
{
  return Vec3<T>(clamp(value.x, minValue, maxValue),
                 clamp(value.y, minValue, maxValue),
                 clamp(value.z, minValue, maxValue));
}

template <class T>
prt_inline Vec3<T> clampSafe(const Vec3<T>& value, const T& minValue, const T& maxValue)
{
  return Vec3<T>(clampSafe(value.x, minValue, maxValue),
                 clampSafe(value.y, minValue, maxValue),
                 clampSafe(value.z, minValue, maxValue));
}

template <class T>
prt_inline Vec3<T> pow(const Vec3<T>& a, const T& b)
{
  return Vec3<T>(pow(a.x, b), pow(a.y, b), pow(a.z, b));
}

template <class T>
prt_inline ToBoolT<Vec3<T>> isfinite(const Vec3<T>& a)
{
  return ToBoolT<Vec3<T>>(isfinite(a.x), isfinite(a.y), isfinite(a.z));
}

// Vector math functions
// ---------------------

template <class T>
prt_inline T dot(const Vec3<T>& a, const Vec3<T>& b)
{
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

template <class T>
prt_inline T lengthSqr(const Vec3<T>& a)
{
  return dot(a, a);
}

template <class T>
prt_inline T length(const Vec3<T>& a)
{
  return sqrt(dot(a, a));
}

template <class T>
prt_inline T lengthRcp(const Vec3<T>& a)
{
  return rsqrt(dot(a, a));
}

template <class T>
prt_inline T lengthRcpSafe(const Vec3<T>& a)
{
  return rsqrtSafe(dot(a, a));
}

template <class T>
prt_inline Vec3<T> normalize(const Vec3<T>& a)
{
  return a * rsqrt(dot(a, a));
}

template <class T>
prt_inline Vec3<T> normalizeSafe(const Vec3<T>& a)
{
  const T l2 = dot(a, a);
  return select(l2 > minVectorLengthSqr, a * rsqrt(l2), a);
}

template <class T>
prt_inline Vec3<T> cross(const Vec3<T>& a, const Vec3<T>& b)
{
  return Vec3<T>(a.y * b.z - a.z * b.y,
                 a.z * b.x - a.x * b.z,
                 a.x * b.y - a.y * b.x);
}

template <class T>
prt_inline Vec3<T> rotate(const Vec3<T>& vec, const Vec3<T>& axis, const T& angle)
{
  return vec * cos(angle) + axis * dot(axis, vec) * (T(1) - cos(angle)) + cross(vec, axis) * sin(angle);
}

template <class T>
prt_inline Vec3<T> slerp(const Vec3<T>& a, const Vec3<T>& b, const T& t)
{
  const T aLen = length(a);
  const T bLen = length(b);
  const T tLen = lerp(aLen, bLen, t);

  const Vec3<T> aNorm = a * rcp(aLen);
  const Vec3<T> bNorm = b * rcp(bLen);

  const T dotAB = dot(aNorm, bNorm);
  if (abs(dotAB) > 0.9995f)
    return normalize(lerp(aNorm, bNorm, t)) * tLen;

  const T theta = acosSafe(dotAB);
  const T thetaT = theta * t;
  const T sinTheta = sin(theta);

  const T aWeight = sin(theta - thetaT) / sinTheta;
  const T bWeight = sin(thetaT) / sinTheta;

  return (aWeight * aNorm + bWeight * bNorm) * tLen;
}

// Reduction functions
// -------------------

template <class T>
prt_inline T reduceMin(const Vec3<T>& a)
{
  return min(min(a.x, a.y), a.z);
}

template <class T>
prt_inline T reduceMax(const Vec3<T>& a)
{
  return max(max(a.x, a.y), a.z);
}

template <class T>
prt_inline T reduceAdd(const Vec3<T>& a)
{
  return a.x + a.y + a.z;
}

template <class T>
prt_inline int selectMin(const Vec3<T>& a)
{
  int axis = a.x < a.y ? 0 : 1;
  axis = a[axis] < a.z ? axis : 2;
  return axis;
}

template <class T>
prt_inline int selectMax(const Vec3<T>& a)
{
  int axis = a.x > a.y ? 0 : 1;
  axis = a[axis] > a.z ? axis : 2;
  return axis;
}

// Test functions
// --------------

template <class T>
prt_inline bool all(const Vec3<T>& a)
{
  return all(a.x) && all(a.y) && all(a.z);
}

template <class T>
prt_inline bool any(const Vec3<T>& a)
{
  return any(a.x) || any(a.y) || any(a.z);
}

template <class T>
prt_inline bool none(const Vec3<T>& a)
{
  return none(a.x) && none(a.y) && none(a.z);
}

// Stream operators
// ----------------

template <class T>
prt_inline std::ostream& operator <<(std::ostream& osm, const Vec3<T>& a)
{
  osm << a.x << "," << a.y << "," << a.z;
  return osm;
}

template <class T>
prt_inline std::istream& operator >>(std::istream& ism, Vec3<T>& a)
{
  ism >> a.x;
  if (ism.peek() == ',')
  {
    ism.ignore();
  }
  else
  {
    a.y = a.z = a.x;
    return ism;
  }

  ism >> a.y;
  if (ism.peek() == ',') ism.ignore();
  ism >> a.z;
  return ism;
}

// SIMD functions
// --------------

template <class T, int N = simdSize>
prt_inline Vec3v<T,N> load3(const T* ptr0, const T* ptr1, const T* ptr2)
{
  return Vec3v<T,N>(load(ptr0), load(ptr1), load(ptr2));
}

template <class T, int N = simdSize>
prt_inline Vec3v<T,N> loadu3(const T* ptr0, const T* ptr1, const T* ptr2)
{
  return Vec3v<T,N>(loadu(ptr0), loadu(ptr1), loadu(ptr2));
}

template <class T, int N = simdSize>
prt_inline void prefetch3L1(const T* ptr0, const T* ptr1, const T* ptr2)
{
  prefetchL1(ptr0);
  prefetchL1(ptr1);
  prefetchL1(ptr2);
}

template <class T, int N>
prt_inline Vec3v<T,N> gather3(const var<bool,N>& mask, const T* ptr, const var<int,N>& idx)
{
  return Vec3v<T,N>(gather(mask, ptr,   idx),
                    gather(mask, ptr+1, idx),
                    gather(mask, ptr+2, idx));
}

template <class T, int N>
prt_inline Vec3v<T,N> gather3(const var<bool,N>& mask, const T* ptr0, const T* ptr1, const T* ptr2, const var<int,N>& idx)
{
  return Vec3v<T,N>(gather(mask, ptr0, idx),
                    gather(mask, ptr1, idx),
                    gather(mask, ptr2, idx));
}

template <class T, int N>
prt_inline void store3(T* ptr0, T* ptr1, T* ptr2, const Vec3v<T,N>& v)
{
  store(ptr0, v[0]);
  store(ptr1, v[1]);
  store(ptr2, v[2]);
}

template <class T, int N>
prt_inline void storeu3(T* ptr0, T* ptr1, T* ptr2, const Vec3v<T,N>& v)
{
  storeu(ptr0, v[0]);
  storeu(ptr1, v[1]);
  storeu(ptr2, v[2]);
}

template <class T, int N>
prt_inline void compress3(vbool mask, T* ptr0, T* ptr1, T* ptr2, const Vec3v<T,N>& v)
{
  compress(mask, ptr0, v[0]);
  compress(mask, ptr1, v[1]);
  compress(mask, ptr2, v[2]);
}

template <int N, int W1, class T>
prt_inline Vec3v<T,N> vunpack3(const var<T,W1>& v, size_t i)
{
  return Vec3v<T,N>(var<T,N>(v[i*4]), var<T,N>(v[i*4+1]), var<T,N>(v[i*4+2]));
}

prt_inline vfloat4 packVec3f(const Vec3f& v)
{
  return vfloat4(v.x, v.y, v.z, 0.0f);
}

prt_inline Vec3f unpackVec3f(const vfloat4& v)
{
  return Vec3f(extract<0>(v), extract<1>(v), extract<2>(v));
}

} // namespace prt
