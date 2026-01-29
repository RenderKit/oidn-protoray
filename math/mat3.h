// Copyright 2025 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "vec3.h"

namespace prt {

template <class T>
struct Mat3
{
  Vec3<T> v[3]; // columns

  prt_inline Mat3() {}
  prt_inline Mat3(Zero) : v{zero, zero, zero} {}
  prt_inline Mat3(One)  : v{{1,0,0}, {0,1,0}, {0,0,1}} {}

  prt_inline Mat3(const Mat3<T>& b) : v{b[0], b[1], b[2]} {}

  template <class T1>
  prt_inline Mat3(const Mat3<T1>& b)
    : v{Vec3<T>(b[0]), Vec3<T>(b[1]), Vec3<T>(b[2])} {}

  prt_inline Mat3(const Vec3<T>& v0, const Vec3<T>& v1, const Vec3<T>& v2)
    : v{v0, v1, v2} {}

  prt_inline Mat3(T m00, T m01, T m02,
                  T m10, T m11, T m12,
                  T m20, T m21, T m22)
    : v{{m00, m10, m20},
        {m01, m11, m21},
        {m02, m12, m22}} {}

  prt_inline Mat3<T>& operator =(const Mat3<T>& b)
  {
    for (int i = 0; i < 3; ++i)
      v[i] = b[i];
    return *this;
  }

  prt_inline Vec3<T>& operator [](size_t i) { return v[i]; }
  prt_inline const Vec3<T>& operator [](size_t i) const { return v[i]; }
};

// Typedefs
// --------

typedef Mat3<float> Mat3f;

template <class T, int N>
using Mat3v = Mat3<var<T,N>>;

typedef Mat3<vfloat>   Mat3vf;
typedef Mat3<vfloat4>  Mat3vf4;
typedef Mat3<vfloat8>  Mat3vf8;
typedef Mat3<vfloat16> Mat3vf16;

// Operators
// ---------

template <class T>
prt_inline Vec3<T> operator *(const Mat3<T>& a, const Vec3<T>& b)
{
  return a[0] * b.x + a[1] * b.y + a[2] * b.z;
}

template <class T>
prt_inline Mat3<T> operator *(const Mat3<T>& a, const Mat3<T>& b)
{
  return {a * b[0], a * b[1], a * b[2]};
}

// Stream operators
// ----------------

template <class T>
prt_inline std::ostream& operator <<(std::ostream& osm, const Mat3<T>& a)
{
  osm << a[0] << ";" << a[1] << ";" << a[2];
  return osm;
}

} // namespace prt
