// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "vec4.h"

namespace prt {

template <class T>
struct Mat4
{
  Vec4<T> v[4]; // columns

  prt_inline Mat4() {}
  prt_inline Mat4(Zero) : v{zero, zero, zero, zero} {}
  prt_inline Mat4(One)  : v{{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0,0,1}} {}

  prt_inline Mat4(const Mat4<T>& b) : v{b[0], b[1], b[2], b[3]} {}

  template <class T1>
  prt_inline Mat4(const Mat4<T1>& b)
    : v{Vec4<T>(b[0]), Vec4<T>(b[1]), Vec4<T>(b[2]), Vec4<T>(b[3])} {}

  prt_inline Mat4(const Vec4<T>& v0, const Vec4<T>& v1, const Vec4<T>& v2, const Vec4<T>& v3)
    : v{v0, v1, v2, v3} {}

  prt_inline Mat4(T m00, T m01, T m02, T m03,
                  T m10, T m11, T m12, T m13,
                  T m20, T m21, T m22, T m23,
                  T m30, T m31, T m32, T m33)
    : v{{m00, m10, m20, m30},
        {m01, m11, m21, m31},
        {m02, m12, m22, m32},
        {m03, m13, m23, m33}} {}

  prt_inline Mat4<T>& operator =(const Mat4<T>& b)
  {
    for (int i = 0; i < 4; ++i)
      v[i] = b[i];
    return *this;
  }

  prt_inline Vec4<T>& operator [](size_t i) { return v[i]; }
  prt_inline const Vec4<T>& operator [](size_t i) const { return v[i]; }
};

// Typedefs
// --------

typedef Mat4<float> Mat4f;

template <class T, int N>
using Mat4v = Mat4<var<T,N>>;

typedef Mat4<vfloat>   Mat4vf;
typedef Mat4<vfloat4>  Mat4vf4;
typedef Mat4<vfloat8>  Mat4vf8;
typedef Mat4<vfloat16> Mat4vf16;

// Operators
// ---------

template <class T>
prt_inline Vec4<T> operator *(const Mat4<T>& a, const Vec4<T>& b)
{
  return a[0] * b.x + a[1] * b.y + a[2] * b.z + a[3] * b.w;
}

template <class T>
prt_inline Mat4<T> operator *(const Mat4<T>& a, const Mat4<T>& b)
{
  return {a * b[0], a * b[1], a * b[2], a * b[3]};
}

// Functions
// ---------

template <class T, class U>
prt_inline Vec4<T> xfmPoint(const Mat4<U>& m, const Vec3<T>& p)
{
  return Mat4<T>(m) * Vec4<T>(p, one);
}

template <class T>
prt_inline Vec3<T> projPoint(const Vec4<T>& p)
{
  return select(p.w != T(zero), p.xyz() / p.w, Vec3<T>(zero));
}

template <class T, class U>
prt_inline Vec3<T> projPoint(const Mat4<U>& m, const Vec3<T>& p)
{
  return projPoint(xfmPoint(m, p));
}

// Stream operators
// ----------------

template <class T>
prt_inline std::ostream& operator <<(std::ostream& osm, const Mat4<T>& a)
{
  osm << a[0] << ";" << a[1] << ";" << a[2] << ";" << a[3];
  return osm;
}

} // namespace prt
