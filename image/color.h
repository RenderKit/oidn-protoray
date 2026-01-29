// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "math/vec3.h"
#include "math/vec4.h"

namespace prt {

template <class T>
prt_inline T average(const Vec3<T>& c)
{
  return (c.x + c.y + c.z) * (1.0f/3.0f);
}

// Linear RGB to luminance
template <class T>
prt_inline T luminance(const Vec3<T>& c)
{
  return 0.212671f*c.x + 0.715160f*c.y + 0.072169f*c.z;
}

// BT.709 linear RGB to/from XYZ tristimulus values
// ------------------------------------------------

template <class T>
prt_inline Vec3<T> rgbToXyz(const Vec3<T>& c)
{
  T x = 0.412453f * c[0] + 0.357580f * c[1] + 0.180423f * c[2];
  T y = 0.212671f * c[0] + 0.715160f * c[1] + 0.072169f * c[2];
  T z = 0.019334f * c[0] + 0.119193f * c[1] + 0.950227f * c[2];

  return Vec3<T>(x, y, z);
}

template <class T>
prt_inline Vec3<T> xyzToRgb(const Vec3<T>& c)
{
  T r =  3.240479f * c[0] - 1.537150f * c[1] - 0.498535f * c[2];
  T g = -0.969256f * c[0] + 1.875991f * c[1] + 0.041556f * c[2];
  T b =  0.055648f * c[0] - 0.204043f * c[1] + 1.057311f * c[2];

  return Vec3<T>(r, g, b);
}

// Linear RGB to/from HSV
// ----------------------

template <class T>
prt_inline Vec3<T> rgbToHsv(const Vec3<T>& c)
{
  Vec4<T> K = Vec4<T>(0.f, -1.f / 3.f, 2.f / 3.f, -1.f);
  Vec4<T> p = select(c.y < c.z, Vec4<T>(c.z, c.y, K.w, K.z), Vec4<T>(c.y, c.z, K.x, K.y));
  Vec4<T> q = select(c.x < p.x, Vec4<T>(p.x, p.y, p.w, c.x), Vec4<T>(c.x, p.y, p.z, p.x));

  T d = q.x - min(q.w, q.y);
  T e = 1e-10f;
  return Vec3<T>(abs(q.z + (q.w - q.y) / (6.f * d + e)), d / (q.x + e), q.x);
}

template <class T>
prt_inline Vec3<T> hsvToRgb(const Vec3<T>& c)
{
  Vec4<T> K = Vec4<T>(1.f, 2.f / 3.f, 1.f / 3.f, 3.f);
  Vec3<T> p = abs(frac(Vec3<T>(c.x) + Vec3<T>(K.x, K.y, K.z)) * T(6.f) - Vec3<T>(K.w));
  return c.z * lerp(Vec3<T>(K.x), clamp(p - Vec3<T>(K.x), T(0.f), T(1.f)), c.y);
}

// CIE XYZ color matching functions
// --------------------------------

static const int cieSize = 471;
extern const float cieX[cieSize];
extern const float cieY[cieSize];
extern const float cieZ[cieSize];
extern const float cieLambda[cieSize];

prt_inline Vec3f cieXyz(int i)
{
  return Vec3f(cieX[i], cieY[i], cieZ[i]);
}

} // namespace prt
