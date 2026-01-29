// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "microfacet.h"

namespace prt {

// GGX (Trowbridge-Reitz) microfacet distribution
// [Walter et al., 2007, "Microfacet Models for Refraction through Rough Surfaces"]
// [Heitz, 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"]
// [Heitz and d'Eon, 2014, "Importance Sampling Microfacet-Based BSDFs using the Distribution of Visible Normals"]
// [Heitz, 2017, "A Simpler and Exact Sampling Routine for the GGX Distribution of Visible Normals"]
template <class T>
class GGXDistribution
{
public:
  Vec2<T> alpha;

  prt_inline GGXDistribution(const Vec2<T>& alpha) : alpha(alpha) {}

  // D(\omega_m) = \frac{1}{\pi \alpha_x \alpha_y \cos^4\theta_m \left(1 + \tan^2\theta_m \left(\frac{\cos^2\phi_m}{\alpha_x^2} + \frac{\sin^2\phi_m}{\alpha_y^2}\right)\right)^2}
  prt_inline T eval(const Vec3<T>& wh) const
  {
    T cosTheta = wh.z;
    T cosTheta2 = sqr(cosTheta);

    T e = (sqr(wh.x / alpha.x) + sqr(wh.y / alpha.y)) / cosTheta2;
    return rcp(float(pi) * alpha.x * alpha.y * sqr(cosTheta2 * (1.f + e)));
  }

  // p(\omega_m) = D(\omega_m) \cos\theta_m
  prt_inline T eval(const Vec3<T>& wh, T& pdf) const
  {
    T cosTheta = wh.z;
    T D = eval(wh);
    pdf = D * abs(cosTheta);
    return D;
  }

  prt_inline T evalVisible(const Vec3<T>& wh, const Vec3<T>& wo, T cosThetaOH, T& pdf) const
  {
    T cosThetaO = wo.z;
    T D = eval(wh);
    pdf = evalG1(wo) * abs(cosThetaOH) * D / abs(cosThetaO);
    return D;
  }

  // Fast visible normal sampling (wo must be in the upper hemisphere)
  // [Heitz, 2017, "A Simpler and Exact Sampling Routine for the GGX Distribution of Visible Normals"]
  prt_inline Vec3<T> sampleVisible(const Vec3<T>& wo, T& pdf, const Vec2<T>& s) const
  {
    // Stretch wo
    Vec3<T> V = normalize(Vec3<T>(alpha.x * wo.x, alpha.y * wo.y, wo.z));

    // Orthonormal basis
    Vec3<T> T1 = select(V.z < 0.9999f, normalize(cross(V, Vec3<T>(0,0,1))), Vec3<T>(1,0,0));
    Vec3<T> T2 = cross(T1, V);

    // Sample point with polar coordinates (r, phi)
    T a = 1.f / (1.f + V.z);
    T r = sqrt(s.x);
    T phi = select(s.y<a, s.y/a * float(pi), float(pi) + (s.y-a)/(1.f-a) * float(pi));
    T P1 = r*cos(phi);
    T P2 = r*sin(phi)*select(s.y<a, 1.f, V.z);

    // Compute normal
    Vec3<T> wh = P1*T1 + P2*T2 + sqrt(max(0.f, 1.f - P1*P1 - P2*P2))*V;

    // Unstretch
    wh = normalize(Vec3<T>(alpha.x * wh.x, alpha.y * wh.y, max(0.f, wh.z)));

    // Compute pdf
    T cosThetaO = wo.z;
    pdf = evalG1(wo) * abs(dot(wo, wh)) * eval(wh) / abs(cosThetaO);
    return wh;
  }

  // Smith Lambda function [Heitz, 2014]
  // \Lambda(\omega_o) = \frac{-1 + \sqrt{1+\frac{1}{a^2}}}{2}
  // a = \frac{1}{\alpha_o \tan\theta_o}
  // \alpha_o = \sqrt{cos^2\phi_o \alpha_x^2 + sin^2\phi_o \alpha_y^2}
  prt_inline T evalLambda(const Vec3<T>& wo) const
  {
    T cosThetaO = wo.z;
    T cosThetaO2 = sqr(cosThetaO);
    T invA2 = (sqr(wo.x * alpha.x) + sqr(wo.y * alpha.y)) / cosThetaO2;
    return 0.5f * (-1.f + sqrt(1.f+invA2));
  }

  prt_inline T evalG1(const Vec3<T>& wo) const
  {
    return rcp(1.f + evalLambda(wo));
  }

  // Smith's height-correlated masking-shadowing function
  // [Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"]
  prt_inline T evalG2(const Vec3<T>& wo, const Vec3<T>& wi) const
  {
    return rcp(1.f + evalLambda(wo) + evalLambda(wi));
  }
};

} // namespace prt
