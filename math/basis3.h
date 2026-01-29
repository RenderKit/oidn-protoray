// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "vec3.h"

namespace prt {

template <class T>
struct Basis3
{
  Vec3<T> U;
  Vec3<T> V;
  Vec3<T> N;

  prt_inline Basis3() {}

  prt_inline Basis3(One)
    : U(one, zero, zero),
      V(zero, one, zero),
      N(zero, zero, one) {}

  prt_inline Basis3(const Basis3<T>& b) : U(b.U), V(b.V), N(b.N) {}

  template <class T1>
  prt_inline Basis3(const Basis3<T1>& b) : U(b.U), V(b.V), N(b.N) {}

  prt_inline Basis3(const Vec3<T>& U, const Vec3<T>& V, const Vec3<T>& N)
    : U(U), V(V), N(N) {}

  prt_inline Basis3<T>& operator =(const Basis3<T>& b)
  {
    U = b.U;
    V = b.V;
    N = b.N;
    return *this;
  }

  prt_inline Basis3<T> operator *(T b) const
  {
    return Basis3<T>(U * b, V * b, N * b);
  }

  prt_inline Basis3<T> operator /(T b) const
  {
    return Basis3<T>(U / b, V / b, N / b);
  }

  prt_inline T det() const
  {
    return dot(U, cross(V, N));
  }

  prt_inline Basis3<T> transposed() const
  {
    return Basis3<T>(Vec3<T>(U.x, V.x, N.x),
                     Vec3<T>(U.y, V.y, N.y),
                     Vec3<T>(U.z, V.z, N.z));
  }

  prt_inline Basis3<T> adjoint() const
  {
    return Basis3<T>(cross(V, N), cross(N, U), cross(U, V)).transposed();
  }

  prt_inline Basis3<T> inverse() const
  {
    return adjoint() * rcp(det());
  }

  prt_inline Vec3<T> toGlobal(const Vec3<T>& a) const
  {
    return a.x * U + a.y * V + a.z * N;
  }

  prt_inline Vec3<T> toLocal(const Vec3<T>& a) const
  {
    return Vec3<T>(dot(a, U), dot(a, V), dot(a, N));
  }

  prt_inline Basis3<T> rotateU(T angle) const
  {
    Basis3<T> res;
    res.V = rotate(V, U, angle);
    res.N = cross(U, res.V);
    res.U = U;
    return res;
  }

  prt_inline Basis3<T> rotateV(T angle) const
  {
    Basis3<T> res;
    res.N = rotate(N, V, angle);
    res.U = cross(V, res.N);
    res.V = V;
    return res;
  }

  prt_inline Basis3<T> rotateN(T angle) const
  {
    Basis3<T> res;
    res.U = rotate(U, N, angle);
    res.V = cross(N, res.U);
    res.N = N;
    return res;
  }
};

// Typedefs
// --------

typedef Basis3<float> Basis3f;

template <class T, int N>
using Basis3v = Basis3<var<T,N>>;

typedef Basis3<vfloat>   Basis3vf;
typedef Basis3<vfloat4>  Basis3vf4;
typedef Basis3<vfloat8>  Basis3vf8;
typedef Basis3<vfloat16> Basis3vf16;

// Operators
// ---------

template <class T>
prt_inline bool operator ==(const Basis3<T>& a, const Basis3<T>& b)
{
  return a.U == b.U && a.V == b.V && a.N == b.N;
}

template <class T>
prt_inline bool operator !=(const Basis3<T>& a, const Basis3<T>& b)
{
  return a.U != b.U || a.V != b.V || a.N != b.N;
}

template <class T>
prt_inline Vec3<T> operator *(const Basis3<T>& basis, const Vec3<T>& a)
{
  return a.x * basis.U + a.y * basis.V + a.z * basis.N;
}

template <class T>
prt_inline Basis3<T> operator *(const Basis3<T>& a, const Basis3<T>& b)
{
  return Basis3<T>(a * b.U, a * b.V, a * b.N);
}

// Functions
// ---------

template <class T, class U>
prt_inline Vec3<T> xfmPoint(const Basis3<U>& b, const Vec3<T>& p)
{
  return p.x * Vec3<T>(b.U) + p.y * Vec3<T>(b.V) + p.z * Vec3<T>(b.N);
}

template <class T, class U>
prt_inline Vec3<T> xfmVector(const Basis3<U>& b, const Vec3<T>& v)
{
  return v.x * Vec3<T>(b.U) + v.y * Vec3<T>(b.V) + v.z * Vec3<T>(b.N);
}

template <class T, class U>
prt_inline Vec3<T> xfmNormal(const Basis3<U>& b, const Vec3<T>& v)
{
  return xfmVector(b.inverse().transposed(), v);
}

// w must be normalized!
template <class T>
prt_inline void makeFrame(Vec3<T>& U, Vec3<T>& V, const Vec3<T>& N)
{
  Vec3<T> U0 = Vec3<T>(zero, N.z, -N.y);
  Vec3<T> U1 = Vec3<T>(-N.z, zero, N.x);
  U = normalize(select(lengthSqr(U0) > lengthSqr(U1), U0, U1));
  V = normalize(cross(N, U));
}

template <class T>
prt_inline Basis3<T> makeFrame(const Vec3<T>& N)
{
  Basis3<T> frame;
  makeFrame(frame.U, frame.V, N);
  frame.N = N;
  return frame;
}

// Stream operators
// ----------------

template <class T>
prt_inline std::ostream& operator <<(std::ostream& osm, const Basis3<T>& a)
{
  osm << a.U << ";" << a.V << ";" << a.N;
  return osm;
}

} // namespace prt
