// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "math.h"
#include "vec2.h"

namespace prt {

// Adaptive Simpson quadrature
template <class Func, class T>
inline T integrate1DStep(Func&& f, float a, float b, float c, const T& fa, const T& fb, const T& fc, const T& q0, int& n, float epsilon, int nMax)
{
  // Check for termination
  if (n >= nMax || c <= a || c >= b)
    return q0;

  // Evaluate the function inside subintervals [a, c] and [c, b]
  float d = (a + c) / 2.0f;
  float e = (c + b) / 2.0f;
  auto fd = f(d);
  auto fe = f(e);
  n += 2;

  // Apply 3-point Simpson's rule for subintervals [a, c] and [c, b]
  auto q1 = (c - a) / 6.0f * (fa + 4.0f*fd + fc);
  auto q2 = (b - c) / 6.0f * (fc + 4.0f*fe + fb);
  auto q = q1 + q2;

  // Recurse if the error is above the threshold
  // Always recurse at least once
  if (reduceMax(abs(q - q0)) > epsilon || n == 5)
  {
    q1 = integrate1DStep(f, a, c, d, fa, fc, fd, q1, n, epsilon, nMax);
    q2 = integrate1DStep(f, c, b, e, fc, fb, fe, q2, n, epsilon, nMax);
    q = q1 + q2;
  }

  return q;
}

template <class Func>
inline ResultOfT<Func(float)> integrate1D(Func&& f, float a, float b, float epsilon = 1e-6f, int nMax = 10000)
{
  // Subdivide the interval [a, b]
  float c = (a + b) / 2.0f;
  auto fa = f(a);
  auto fb = f(b);
  auto fc = f(c);
  int n = 3;

  // Try to fix infinities at the endpoints
  if (!all(isfinite(fa)))
    fa = f(a + float(ulp) * (b - a));
  if (!all(isfinite(fb)))
    fb = f(b - float(ulp) * (b - a));

  // Apply 3-point Simpson's rule
  auto q = (b - a) / 6.0f * (fa + 4.0f*fc + fb);

  return integrate1DStep(f, a, b, c, fa, fb, fc, q, n, epsilon, nMax);
}

template <class Func>
inline ResultOfT<Func(Vec2f)> integrate2D(Func&& f, const Vec2f& a, const Vec2f& b, float epsilon = 1e-6f, int nMax = 10000)
{
  auto ifx = [=](float x)
  {
    auto fx = [=](float y) { return f(Vec2f(x, y)); };
    return integrate1D(fx, a.y, b.y, epsilon, nMax);
  };

  return integrate1D(ifx, a.x, b.x, epsilon, nMax);
}

} // namespace prt
