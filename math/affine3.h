// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "basis3.h"
#include "box3.h"
#include "mat4.h"

namespace prt {

template <class T>
struct Affine3
{
  Basis3<T> l;
  Vec3<T> p;

  prt_inline Affine3() {}
  prt_inline Affine3(One) : l(one), p(zero) {}

  prt_inline Affine3(const Affine3<T>& b) : l(b.l), p(b.p) {}

  template <class T1>
  prt_inline Affine3(const Affine3<T1>& b) : l(b.l), p(b.p) {}

  prt_inline Affine3(const Basis3<T>& l, const Vec3<T>& p) : l(l), p(p) {}

  prt_inline Affine3(const Vec3<T>& lx, const Vec3<T>& ly, const Vec3<T>& lz, const Vec3<T>& p)
    : l(lx, ly, lz), p(p) {}

  prt_inline Affine3<T>& operator =(const Affine3<T>& b)
  {
    l = b.l;
    p = b.p;
    return *this;
  }

  prt_inline Mat4<T> toGlobalMat() const
  {
    return {l.U.x, l.V.x, l.N.x, p.x,
            l.U.y, l.V.y, l.N.y, p.y,
            l.U.z, l.V.z, l.N.z, p.z,
            zero,  zero,  zero,  one};
  }

  prt_inline Mat4<T> toLocalMat() const
  {
    return {l.U.x, l.U.y, l.U.z, -dot(l.U, p),
            l.V.x, l.V.y, l.V.z, -dot(l.V, p),
            l.N.x, l.N.y, l.N.z, -dot(l.N, p),
            zero,  zero,  zero,  one};
  }
};

// Typedefs
// --------

typedef Affine3<float> Affine3f;

template <class T, int N>
using Affine3v = Affine3<var<T,N>>;

typedef Affine3<vfloat>   Affine3vf;
typedef Affine3<vfloat4>  Affine3vf4;
typedef Affine3<vfloat8>  Affine3vf8;
typedef Affine3<vfloat16> Affine3vf16;

// Operators
// ---------

template <class T>
prt_inline bool operator ==(const Affine3<T>& a, const Affine3<T>& b)
{
  return a.l == b.l && a.p == b.p;
}

template <class T>
prt_inline bool operator !=(const Affine3<T>& a, const Affine3<T>& b)
{
  return a.l != b.l || a.p != b.p;
}

template <class T>
prt_inline Affine3<T> operator *(const Affine3<T>& a, const Affine3<T>& b)
{
  return Affine3<T>(a.l * b.l, a.l * b.p + a.p);
}

// Functions
// ---------

template <class T, class U>
prt_inline Vec3<T> xfmPoint(const Affine3<U>& a, const Vec3<T>& p)
{
  return xfmPoint(a.l, p) + a.p;
}

template <class T, class U>
prt_inline Vec3<T> xfmVector(const Affine3<U>& a, const Vec3<T>& v)
{
  return xfmVector(a.l, v);
}

template <class T, class U>
prt_inline Vec3<T> xfmNormal(const Affine3<U>& a, const Vec3<T>& v)
{
  return xfmNormal(a.l, v);
}

template <class T>
prt_inline Box3<T> xfmBox(const Affine3<T>& a, const Box3<T>& b)
{
  Box3<T> res = empty;
  for (int i = 0; i < 2; ++i)
  {
    for (int j = 0; j < 2; ++j)
    {
      for (int k = 0; k < 2; ++k)
        res.grow(xfmPoint(a, Vec3<T>(b[i].x, b[j].y, b[k].z)));
    }
  }
  return res;
}

// Stream operators
// ----------------

template <class T>
prt_inline std::ostream& operator <<(std::ostream& osm, const Affine3<T>& a)
{
  osm << a.l << ";" << a.p;
  return osm;
}

} // namespace prt
