// Copyright 2025 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "vec2.h"

namespace prt {

template <class T>
struct Mat2
{
  Vec2<T> v[2]; // columns

  prt_inline Mat2() {}
  prt_inline Mat2(Zero) : v{zero, zero} {}
  prt_inline Mat2(One)  : v{{1,0}, {0,1}} {}

  prt_inline Mat2(const Mat2<T>& b) : v{b[0], b[1]} {}

  template <class T1>
  prt_inline Mat2(const Mat2<T1>& b)
    : v{Vec2<T>(b[0]), Vec2<T>(b[1])} {}

  prt_inline Mat2(const Vec2<T>& v0, const Vec2<T>& v1)
    : v{v0, v1} {}

  prt_inline Mat2(T m00, T m01,
                  T m10, T m11)
    : v{{m00, m10},
        {m01, m11}} {}

  prt_inline Mat2<T>& operator =(const Mat2<T>& b)
  {
    for (int i = 0; i < 2; ++i)
      v[i] = b[i];
    return *this;
  }

  prt_inline Vec2<T>& operator [](size_t i) { return v[i]; }
  prt_inline const Vec2<T>& operator [](size_t i) const { return v[i]; }
};

// Typedefs
// --------

typedef Mat2<float> Mat2f;

template <class T, int N>
using Mat2v = Mat2<var<T,N>>;

typedef Mat2<vfloat>   Mat2vf;
typedef Mat2<vfloat4>  Mat2vf4;
typedef Mat2<vfloat8>  Mat2vf8;
typedef Mat2<vfloat16> Mat2vf16;

// Operators
// ---------

template <class T>
prt_inline Vec2<T> operator *(const Mat2<T>& a, const Vec2<T>& b)
{
  return a[0] * b.x + a[1] * b.y;
}

template <class T>
prt_inline Mat2<T> operator *(const Mat2<T>& a, const Mat2<T>& b)
{
  return {a * b[0], a * b[1]};
}

// Stream operators
// ----------------

template <class T>
prt_inline std::ostream& operator <<(std::ostream& osm, const Mat2<T>& a)
{
  osm << a[0] << ";" << a[1];
  return osm;
}

} // namespace prt
