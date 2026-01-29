// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "math/vec3.h"

namespace prt {

// Reflects an incident direction I at a normal N
template <class T>
prt_inline Vec3<T> reflect(const Vec3<T>& I, const Vec3<T>& N)
{
  T cosI = dot(I, N);
  return 2.f*cosI*N - I;
}

// Reflects an incident direction I at a normal N
// cosI - cosine between I and N
template <class T>
prt_inline Vec3<T> reflect(const Vec3<T>& I, const Vec3<T>& N, T cosI)
{
  return 2.f*cosI*N - I;
}

// Refracts an incident direction I at a normal N
// cosI - cosine between I and N
// cosT - cosine between the transmitted direction and -N
// eta  - relative refraction index (etaI/etaT)
template <class T>
prt_inline Vec3<T> refract(const Vec3<T>& I, const Vec3<T>& N, T cosI, T cosT, T eta)
{
  return eta*(cosI*N - I) - cosT*N;
}

// Refracts an incident direction I at a normal N
// cosI - cosine between I and N
// eta  - relative refraction index (etaI/etaT)
prt_inline Vec3f refract(const Vec3f& I, const Vec3f& N, float cosI, float eta)
{
  float k = 1.f - eta*eta*(1.f-cosI*cosI);
  if (k < 0.f) return zero; // total internal reflection
  float cosT = sqrt(k);
  return eta*(cosI*N - I) - cosT*N;
}

// Refracts an incident direction I at a normal N
// cosI   - cosine between I and N
// eta    - relative refraction index (etaI/etaT)
// return - cosine between the transmitted direction and -N
template <class T>
prt_inline T refract(T cosI, T eta)
{
  T k = 1.f - eta*eta*(1.f-cosI*cosI);
  return sqrt(max(k, 0.f));
}

// Fresnel coefficient for dielectrics
// cosI - cosine between the incident direction and the surface normal (positive)
// cosT - cosine between the transmitted direction and the surface normal (positive)
// eta  - relative refraction index (etaI/etaT)
template <class T>
prt_inline T fresnelDielectric(T cosI, T cosT, T eta)
{
  T Rper = (eta*cosI -     cosT) * rcp(eta*cosI +     cosT);
  T Rpar = (    cosI - eta*cosT) * rcp(    cosI + eta*cosT);
  return 0.5f * (Rpar*Rpar + Rper*Rper);
}

// Fresnel coefficient for dielectrics
// cosI - cosine between the incident direction and the surface normal (positive)
// eta  - relative refraction index (etaI/etaT)
prt_inline float fresnelDielectric(float cosI, float eta)
{
  cosI = max(cosI, 0.f);
  float k = 1.f - eta*eta*(1.f-cosI*cosI);
  if (k < 0.f) return 1.f; // total internal reflection
  float cosT = sqrt(k);
  return fresnelDielectric(cosI, cosT, eta);
}

prt_inline vfloat fresnelDielectric(vbool m, vfloat cosI, vfloat eta)
{
  vfloat result = 1.f;

  cosI = max(cosI, 0.f);
  vfloat k = 1.f - eta*eta*(1.f-cosI*cosI);
  m &= (k >= 0.f);
  if (any(m))
  {
    vfloat cosT = sqrt(k);
    set(m, result, fresnelDielectric(cosI, cosT, eta));
  }

  return result;
}

// Fresnel coefficient for dielectrics (also returns cosT)
// cosI - cosine between the incident direction and the surface normal (positive)
// cosT - cosine between the transmitted direction and the surface normal (positive)
// eta  - relative refraction index (etaI/etaT)
prt_inline float fresnelDielectricEx(float cosI, float& cosT, float eta)
{
  cosI = abs(cosI);
  float k = 1.f - eta*eta*(1.f-cosI*cosI);
  if (k < 0.f)
  {
    // Total internal reflection
    cosT = 0.f;
    return 1.f;
  }

  cosT = sqrt(k);
  return fresnelDielectric(cosI, cosT, eta);
}

prt_inline vfloat fresnelDielectricEx(vbool m, vfloat cosI, vfloat& cosT, vfloat eta)
{
  // Total internal reflection
  vfloat result = 1.f;
  cosT = 0.f;

  cosI = abs(cosI);
  vfloat k = 1.f - eta*eta*(1.f-cosI*cosI);

  m &= (k >= 0.f);
  if (any(m))
  {
    set(m, cosT, sqrt(k));
    set(m, result, fresnelDielectric(cosI, cosT, eta));
  }

  return result;
}

// F_{\mathit{avg}} = 2 \int_0^1 F(\mu) \mu d\mu
// \mu = \cos(\theta)
// Fit from [Kulla and Conty, 2017, "Revisiting Physically Based Shading at Imageworks"]
prt_inline float fresnelDielectricAvg(float eta)
{
  float rcpEta = rcp(eta);

  if (rcpEta >= 1.f)
    return (rcpEta - 1.f) / (4.08567f + 1.00071f*rcpEta);

  float rcpEta2 = sqr(rcpEta);
  float rcpEta3 = rcpEta2 * rcpEta;
  return 0.997118f + 0.1014f*rcpEta - 0.965241f*rcpEta2 - 0.130607f*rcpEta3;
}

prt_inline vfloat fresnelDielectricAvg(vbool m, vfloat eta)
{
  vfloat result = zero;
  vfloat rcpEta = rcp(eta);

  vbool mGe = m & (rcpEta >= 1.f);
  if (any(mGe))
    result = (rcpEta - 1.f) / (4.08567f + 1.00071f*rcpEta);

  vbool mLt = andn(m, mGe);
  if (any(mLt))
  {
    vfloat rcpEta2 = sqr(rcpEta);
    vfloat rcpEta3 = rcpEta2 * rcpEta;
    set(mLt, result, 0.997118f + 0.1014f*rcpEta - 0.965241f*rcpEta2 - 0.130607f*rcpEta3);
  }

  return result;
}

// Fresnel coefficient for conductors
// cosI - cosine between the incident direction and the surface normal (positive)
template <class T, class V>
prt_inline V fresnelConductor(T cosI, V eta, V k)
{
  cosI = abs(cosI);

  V tmp = sqr(eta) + sqr(k);
  V Rpar = (tmp * sqr(cosI) - T(2.f) * eta * cosI + T(1.f))
           * rcp(tmp * sqr(cosI) + T(2.f) * eta * cosI + T(1.f));
  V Rper = (tmp - T(2.f) * eta * cosI + sqr(cosI))
           * rcp(tmp + T(2.f) * eta * cosI + sqr(cosI));
  return T(0.5f) * (Rpar + Rper);
}

template <class T>
class FresnelConductor
{
private:
  Vec3<T> eta;
  Vec3<T> k;

public:
  prt_inline FresnelConductor(const Vec3<T>& eta, const Vec3<T>& k)
    : eta(eta), k(k) {}

  prt_inline Vec3<T> eval(T cosI) const
  {
    return fresnelConductor(cosI, eta, k);
  }

  prt_inline Vec3<T> evalAvg() const
  {
    return zero; // not supported!
  }

  prt_inline Vec3<T> getAlbedo() const
  {
    return T(1.f); // FIXME
  }
};

// [Gulbrandsen, 2014, "Artist Friendly Metallic Fresnel"]
template <class T>
prt_inline void fresnelConductorArtisticToIOR(T r, T g, T& eta, T& k)
{
  r = min(r, T(0.99f));

  const T n_min = (T(1.f) - r) / (T(1.f) + r);
  const T n_max = (T(1.f) + sqrt(r)) / (T(1.f) - sqrt(r));
  const T n = g * n_min + (T(1.f) - g) * n_max;
  const T k2 = (sqr(n + T(1.f)) * r - sqr(n - T(1.f))) / (T(1.f) - r);

  eta = n;
  k = sqrt(max(k2, T(0.f)));
}

template <class T>
prt_inline T fresnelConductorArtisticAvg(T r, T g)
{
  // Fit from [Kulla and Conty, 2017, "Revisiting Physically Based Shading at Imageworks"]
  const T r2 = r*r;
  const T r3 = r2*r;
  const T g2 = g*g;
  const T g3 = g2*g;

  return T(0.087237f) + T(0.0230685f)*g - T(0.0864902f)*g2 + T(0.0774594f)*g3
         + T(0.782654f)*r - T(0.136432f)*r2 + T(0.278708f)*r3
         + T(0.19744f)*g*r + T(0.0360605f)*g2*r - T(0.2586f)*g*r2;
}

template <class T>
class FresnelConductorArtistic
{
private:
  Vec3<T> eta, k;
  Vec3<T> r, g;

public:
  prt_inline FresnelConductorArtistic(const Vec3<T>& reflectivity, const Vec3<T>& edgeTint)
  {
    r = reflectivity;
    g = edgeTint;
    fresnelConductorArtisticToIOR(r, g, eta, k);
  }

  prt_inline Vec3<T> eval(T cosI) const
  {
    return fresnelConductor(cosI, eta, k);
  }

  prt_inline Vec3<T> evalAvg() const
  {
    return fresnelConductorArtisticAvg(r, g);
  }

  prt_inline Vec3<T> getAlbedo() const
  {
    return r;
  }

  prt_inline Vec3<T> getR() const { return r; }
  prt_inline Vec3<T> getG() const { return g; }
};

} // namespace prt
