// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "vec3.h"

namespace prt {

// Spherical harmonics coefficients
template <int N, class T>
struct SH
{
  T c[N]; // coefficients

  prt_inline SH() {}

  prt_inline SH(Zero)
  {
    for (int i = 0; i < N; ++i)
      c[i] = zero;
  }

  prt_inline SH(const SH<N,T>& a)
  {
    for (int i = 0; i < N; ++i)
      c[i] = a.c[i];
  }

  template <class T1>
  prt_inline SH(const SH<N, T1>& a)
  {
    for (int i = 0; i < N; ++i)
      c[i] = T(a.c[i]);
  }

  prt_inline SH<N,T>& operator =(const SH<N,T>& a)
  {
    for (int i = 0; i < N; ++i)
      c[i] = a.c[i];
    return *this;
  }

  prt_inline T& operator [](size_t i)
  {
    assert(i < N);
    return c[i];
  }

  prt_inline const T& operator [](size_t i) const
  {
    assert(i < N);
    return c[i];
  }
};

// Typedefs
// --------

template <class T>
using SHL1 = SH<4, T>;

// Arithmetic operators
// --------------------

template <int N, class T>
prt_inline SH<N,T> operator +(const SH<N,T>& a, const SH<N,T>& b)
{
  SH<N,T> res;
  for (int i = 0; i < N; ++i)
    res[i] = a[i] + b[i];
  return res;
}

template <int N, class T>
prt_inline SH<N,T> operator *(const SH<N,T>& a, const SH<N,T>& b)
{
  SH<N,T> res;
  for (int i = 0; i < N; ++i)
    res[i] = a[i] * b[i];
  return res;
}

template <int N, class T>
prt_inline SH<N,T> operator *(const SH<N,T>& a, const T& b)
{
  SH<N,T> res;
  for (int i = 0; i < N; ++i)
    res[i] = a[i] * b;
  return res;
}

template <int N, class T>
prt_inline SH<N,T> operator *(const T& a, const SH<N,T>& b)
{
  SH<N,T> res;
  for (int i = 0; i < N; ++i)
    res[i] = a * b[i];
  return res;
}

// Assignment operators
// --------------------

template <int N, class T> prt_inline SH<N, T>& operator +=(SH<N, T>& a, const SH<N, T>& b) { return a = a + b; }
template <int N, class T> prt_inline SH<N, T>& operator *=(SH<N, T>& a, const SH<N, T>& b) { return a = a * b; }
template <int N, class T> prt_inline SH<N, T>& operator *=(SH<N, T>& a, const T& b)        { return a = a * b; }

// Evaluation functions
// --------------------

// Evaluates simplified SH basis functions
// [Hazel, 2015, "Spherical Harmonics for Lighting"]
template <class T>
SHL1<T> evalSHL1(const Vec3<T>& dir)
{
  SHL1<T> sh;
  sh[0] = 1.f;
  sh[1] = dir.x;
  sh[2] = dir.y;
  sh[3] = dir.z;
  return sh;
}

template <class T, class V>
SHL1<V> evalSHL1(const Vec3<T>& dir, const V& value)
{
  SHL1<V> sh;
  sh[0] = value;
  sh[1] = value * dir.x;
  sh[2] = value * dir.y;
  sh[3] = value * dir.z;
  return sh;
}

} // namespace prt