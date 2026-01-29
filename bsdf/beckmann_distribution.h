// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "microfacet.h"

namespace prt {

// Beckmann microfacet distribution
// [Walter et al., 2007, "Microfacet Models for Refraction through Rough Surfaces"]
// [Heitz, 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"]
template <class T>
class BeckmannDistribution
{
public:
  T alpha;

  prt_inline BeckmannDistribution(T alpha) : alpha(alpha) {}

  prt_inline Vec3<T> sample(T& pdf, const Vec2<T>& s) const
  {
    T alpha2 = sqr(alpha);
    T tanTheta2 = -alpha2 * log(1.f-s.x);
    T cosTheta = rsqrt(1.f + tanTheta2);
    T sinTheta = cos2sin(cosTheta);

    T phi = 2.f*float(pi) * s.y;

    T cosTheta3 = sqr(cosTheta)*cosTheta;
    pdf = (1.f-s.x) * rcp(float(pi) * alpha2 * cosTheta3);

    T sinPhi, cosPhi;
    sincos(phi, sinPhi, cosPhi);

    T x = cosPhi * sinTheta;
    T y = sinPhi * sinTheta;
    T z = cosTheta;
    return Vec3<T>(x,y,z);
  }
};

} // namespace prt
