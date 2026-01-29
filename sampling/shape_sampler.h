// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/common.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/basis3.h"

namespace prt {

template <class T>
prt_inline Vec3<T> cartesian(T phi, T sinTheta, T cosTheta)
{
  T sinPhi, cosPhi;
  sincos(phi, sinPhi, cosPhi);
  return Vec3<T>(cosPhi * sinTheta,
                 sinPhi * sinTheta,
                 cosTheta);
}

template <class T>
prt_inline Vec3<T> uniformSampleSphere(T& pdf, const Vec2<T>& s)
{
  T cosTheta = 1.f - 2.f*s.x;
  T sinTheta = 2.f * sqrt(s.x * (1.f-s.x));

  T phi = 2.f*float(pi) * s.y;

  pdf = 1.f/(4.f*float(pi));

  T sinPhi, cosPhi;
  sincos(phi, sinPhi, cosPhi);

  T x = cosPhi * sinTheta;
  T y = sinPhi * sinTheta;
  T z = cosTheta;
  return Vec3<T>(x,y,z);
}

template <class T>
prt_inline Vec3<T> uniformSampleSphere(const Vec2<T>& s)
{
  T pdf;
  return uniformSampleSphere(pdf, s);
}

prt_inline float uniformSampleSpherePdf()
{
  return 1.f/(4.f*float(pi));
}

template <class T>
prt_inline Vec3<T> uniformSampleHemisphere(const Vec2<T>& s)
{
  const T phi = 2.f*float(pi) * s.x;
  const T cosTheta = s.y;
  const T sinTheta = cos2sin(s.y);
  return cartesian(phi, sinTheta, cosTheta);
}

prt_inline float uniformSampleHemispherePdf()
{
  return 1.f/(2.f*float(pi));
}

template <class T>
prt_inline Vec3<T> cosineSampleHemisphere(T& pdf, const Vec2<T>& s)
{
  T cosTheta = sqrt(s.x);
  T sinTheta = sqrt(1.f-s.x);

  T phi = 2.f*float(pi) * s.y;

  pdf = cosTheta * (1.f/float(pi));

  T sinPhi, cosPhi;
  sincos(phi, sinPhi, cosPhi);

  T x = cosPhi * sinTheta;
  T y = sinPhi * sinTheta;
  T z = cosTheta;
  return Vec3<T>(x,y,z);
}

template <class T>
prt_inline Vec3<T> cosineSampleHemisphere(const Vec2<T>& s)
{
  T pdf;
  return cosineSampleHemisphere(pdf, s);
}

template <class T>
prt_inline T cosineSampleHemispherePdf(T cosTheta)
{
  return cosTheta * (1.f/float(pi));
}

template <class T>
prt_inline Vec3<T> uniformSampleCone(T& pdf, const Vec2<T>& s, T angle)
{
  T cosTheta = 1.f - s.x*(1.f-cos(angle));
  T sinTheta = cos2sin(cosTheta);

  T phi = 2.f*float(pi) * s.y;

  pdf = rcp(4.f*float(pi)*sqr(sin(0.5f*angle)));

  T sinPhi, cosPhi;
  sincos(phi, sinPhi, cosPhi);

  T x = cosPhi * sinTheta;
  T y = sinPhi * sinTheta;
  T z = cosTheta;
  return Vec3<T>(x,y,z);
}

template <class T>
prt_inline T uniformSampleConePdf(T cosTheta, T angle)
{
  return select(cosTheta < cos(angle), T(0.f), rcp(4.f*float(pi)*sqr(sin(0.5f*angle))));
}

/*
template <class T>
prt_inline Vec3<T> uniformSampleTriangle(const Vec2<T>& s, const Vec3<T>& v0, const Vec3<T>& v1, const Vec3<T>& v2)
{
  T rx = sqrt(s.x);
  return v2 + (1.f-rx)*(v0-v2) + (s.y*rx)*(v1-v2);
}
*/

template <class T>
prt_inline Vec3<T> uniformSampleTriangle(const Vec2<T>& s, const Vec3<T>& v0, const Vec3<T>& v1, const Vec3<T>& v2)
{
  // [Heitz, 2019, "A Low-Distortion Map Between Triangle and Square"]
  T u = s.x;
  T v = s.y;
  if (v > u)
  {
    u *= 0.5f;
    v -= u;
  }
  else
  {
    v *= 0.5f;
    u -= v;
  }

  return v2 + u*(v0-v2) + v*(v1-v2);
}

template <class T, int N>
prt_inline Vec3v<T,N> uniformSampleTriangle(const Vec2v<T,N>& s, const Vec3v<T,N>& v0, const Vec3v<T,N>& v1, const Vec3v<T,N>& v2)
{
  // [Heitz, 2019, "A Low-Distortion Map Between Triangle and Square"]
  var<T,N> u = s.x;
  var<T,N> v = s.y;
  var<bool,N> mask = v > u;
  set(mask, u, u * 0.5f);
  set(mask, v, v - u);
  set(!mask, v, v * 0.5f);
  set(!mask, u, u - v);

  return v2 + u*(v0-v2) + v*(v1-v2);
}

template <class T>
prt_inline Vec2<T> uniformSampleDisk(const Vec2<T>& s)
{
  T r = sqrt(s.x);
  T theta = 2.f*float(pi) * s.y;

  T sinTheta, cosTheta;
  sincos(theta, sinTheta, cosTheta);

  return Vec2<T>(r*cosTheta, r*sinTheta);
}

} // namespace prt
