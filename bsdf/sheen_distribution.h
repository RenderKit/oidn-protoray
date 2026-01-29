// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "microfacet.h"

namespace prt {

// Cylindrical microfiber distribution for sheen
// [Conty and Kulla, 2017, "Production Friendly Microfacet Sheen BRDF"]
// [Kulla and Conty, 2017, "Revisiting Physically Based Shading at Imageworks"]
template <class T>
class SheenDistribution
{
public:
  T r; // in (0, 1]

  prt_inline SheenDistribution(T r) : r(r) {}

  prt_inline T eval(T cosTheta) const
  {
    T sinTheta = cos2sin(cosTheta);
    T invr = rcp(r);
    return (2.f + invr) * pow(sinTheta, invr) * (1.f/(2.f*float(pi)));
  }

  prt_inline T evalLambda(T cosTheta) const
  {
    return select(cosTheta < 0.5f,
                  exp(evalL(cosTheta)),
                  exp(2.f * evalL(0.5f) - evalL(1.f - cosTheta)));
  }

  // Smith's height-correlated masking-shadowing function
  // [Heitz, 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"]
  prt_inline T evalG2(T cosThetaO, T cosThetaI) const
  {
    //return rcp(1.f + evalLambda(cosThetaO) + evalLambda(cosThetaI));
    return rcp(1.f + evalLambda(cosThetaO) + evalLambdaI(cosThetaI));
  }

private:
  // Helper function for computing Lambda
  prt_inline T evalL(T x) const
  {
    const T a0 = 25.3245f;
    const T a1 = 21.5473f;
    const T b0 = 3.32435f;
    const T b1 = 3.82987f;
    const T c0 = 0.16801f;
    const T c1 = 0.19823f;
    const T d0 = -1.27393f;
    const T d1 = -1.97760f;
    const T e0 = -4.85967f;
    const T e1 = -4.32054f;

    T w0 = sqr(1.f - r);
    T w1 = 1.f - w0;

    T a = w0*a0 + w1*a1;
    T b = w0*b0 + w1*b1;
    T c = w0*c0 + w1*c1;
    T d = w0*d0 + w1*d1;
    T e = w0*e0 + w1*e1;

    return a / (1.f + b * pow(x, c)) + d * x + e;
  }

  // Non-physical artistic adjustment for a softer light terminator
  prt_inline T evalLambdaI(T cosTheta) const
  {
    T x = 1.f + 2.f * sqr(sqr(sqr(1.f - cosTheta)));
    return pow(evalLambda(cosTheta), x);
  }
};

} // namespace prt
