// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "math/mat2.h"
#include "math/mat3.h"
#include "sampling/shape_sampler.h"
#include "optics.h"
#include "ggx_distribution.h"
#include "sheen_distribution.h"
#include "albedo_tables.h"

namespace prt {

struct MicrofacetAlbedo
{
  static prt_inline float sample(float cosThetaO, const GGXDistribution<float>& microfacet, const Vec2f& s)
  {
    // Handle edge cases
    cosThetaO = max(cosThetaO, 1e-6f);

    // Make an outgoing vector
    Vec3f wo(cos2sin(cosThetaO), 0.f, cosThetaO);

    // Sample the microfacet normal
    float whPdf;
    Vec3f wh = microfacet.sampleVisible(wo, whPdf, s);

    float cosThetaOH = dot(wo, wh);

    // Sample the reflection
    Vec3f wi = reflect(wo, wh, cosThetaOH);
    float cosThetaI = wi.z;
    if (cosThetaI <= 0.f)
      return 0.f;

    float G = microfacet.evalG2(wo, wi);
    return G * rcpSafe(microfacet.evalG1(wo));
  }

  static float integrate(float cosThetaO, float roughness, int numSamples = 1024)
  {
    GGXDistribution<float> microfacet(roughnessToAlpha(roughness));

    int n = sqrt((float)numSamples);
    float sum = 0.f;
    for (int i = 0; i < n; i++)
    {
      for (int j = 0; j < n; j++)
      {
        Vec2f s = min((Vec2f(i, j) + 0.5f) / (float)n, Vec2f(1.f - 1e-6f));
        sum += sample(cosThetaO, microfacet, s);
      }
    }

    return min(sum / (n*n), 1.f);
  }
};

struct MicrofacetDielectricAlbedo
{
  static prt_inline float sample(float cosThetaO, float eta, const GGXDistribution<float>& microfacet, const Vec2f& s)
  {
    // Handle edge cases
    cosThetaO = max(cosThetaO, 1e-6f);

    // Make an outgoing vector
    Vec3f wo(cos2sin(cosThetaO), 0.f, cosThetaO);

    // Sample the microfacet normal
    float whPdf;
    Vec3f wh = microfacet.sampleVisible(wo, whPdf, s);

    float cosThetaOH = dot(wo, wh);

    // Fresnel term
    float cosThetaTH; // positive
    float F = fresnelDielectricEx(cosThetaOH, cosThetaTH, eta);

    float weight = 0.f;

    // Sample the reflection
    Vec3f wi = reflect(wo, wh, cosThetaOH);
    float cosThetaI = wi.z;
    if (cosThetaI > 0.f)
    {
      float G = microfacet.evalG2(wo, wi);
      weight += F * (G * rcpSafe(microfacet.evalG1(wo)));
    }

    // Sample the transmission
    // cosThetaTH = -cosThetaIH
    wi = refract(wo, wh, cosThetaOH, cosThetaTH, eta);
    cosThetaI = wi.z;
    if (cosThetaI < 0.f)
    {
      float G = microfacet.evalG2(wo, wi);
      weight += (1.f-F) * (G * rcpSafe(microfacet.evalG1(wo)));
    }

    return weight;
  }

  static float integrate(float cosThetaO, float eta, float roughness, int numSamples = 1024)
  {
    GGXDistribution<float> microfacet(roughnessToAlpha(roughness));

    int n = sqrt((float)numSamples);
    float sum = 0.f;
    for (int i = 0; i < n; i++)
    {
      for (int j = 0; j < n; j++)
      {
        Vec2f s = min((Vec2f(i, j) + 0.5f) / (float)n, Vec2f(1.f - 1e-6f));
        sum += sample(cosThetaO, eta, microfacet, s);
      }
    }

    return min(sum / (n*n), 1.f);
  }
};

struct MicrofacetDielectricReflectionAlbedo
{
  static prt_inline float sample(float cosThetaO, float eta, const GGXDistribution<float>& microfacet, const Vec2f& s)
  {
    // Handle edge cases
    cosThetaO = max(cosThetaO, 1e-6f);

    // Make an outgoing vector
    Vec3f wo(cos2sin(cosThetaO), 0.f, cosThetaO);

    // Sample the microfacet normal
    float whPdf;
    Vec3f wh = microfacet.sampleVisible(wo, whPdf, s);

    float cosThetaOH = dot(wo, wh);

    // Fresnel term
    float F = fresnelDielectric(cosThetaOH, eta);

    // Sample the reflection
    Vec3f wi = reflect(wo, wh, cosThetaOH);
    float cosThetaI = wi.z;
    if (cosThetaI <= 0.f)
      return 0.f;

    float G = microfacet.evalG2(wo, wi);
    return F * (G * rcpSafe(microfacet.evalG1(wo)));
  }

  static float integrate(float cosThetaO, float eta, float roughness, int numSamples = 1024)
  {
    GGXDistribution<float> microfacet(roughnessToAlpha(roughness));

    int n = sqrt((float)numSamples);
    float sum = 0.f;
    for (int i = 0; i < n; i++)
    {
      for (int j = 0; j < n; j++)
      {
        Vec2f s = min((Vec2f(i, j) + 0.5f) / (float)n, Vec2f(1.f - 1e-6f));
        sum += sample(cosThetaO, eta, microfacet, s);
      }
    }

    return min(sum / (n*n), 1.f);
  }
};

struct MicrofacetDielectricReflectionECAlbedo
{
  static prt_inline float sample(float cosThetaO, float eta, float roughness, const Vec2f& s)
  {
    // Handle edge cases
    cosThetaO = max(cosThetaO, 1e-6f);

    // Make an outgoing vector
    Vec3f wo(cos2sin(cosThetaO), 0.f, cosThetaO);

    // Sample the microfacet normal
    GGXDistribution<float> microfacet(roughnessToAlpha(roughness));
    float whPdf;
    Vec3f wh = microfacet.sampleVisible(wo, whPdf, s);

    float cosThetaOH = dot(wo, wh);

    // Sample the reflection
    Vec3f wi = reflect(wo, wh, cosThetaOH);
    float cosThetaI = wi.z;
    if (cosThetaI <= 0.f)
      return 0.f;

    float F = fresnelDielectric(cosThetaOH, eta);
    float G = microfacet.evalG2(wo, wi);

    // Energy compensation
    float Eavg = MicrofacetAlbedoTable::evalAvg(roughness);
    float Favg = fresnelDielectricAvg(eta);
    float fmsScale = sqr(Favg) * Eavg / (1.f - Favg * (1.f - Eavg)); // Stephen Hill's tweak

    float Eo = MicrofacetAlbedoTable::eval(cosThetaO, roughness);
    float Ei = MicrofacetAlbedoTable::eval(cosThetaI, roughness);
    float fms = fmsScale * ((1.f - Eo) * (1.f - Ei) * rcp(float(pi) * (1.f - Eavg)) * cosThetaI);

    float pdf = whPdf * rcp(4.f*abs(cosThetaOH));
    return F * (G * rcpSafe(microfacet.evalG1(wo))) + (fms * rcp(pdf));
  }

  static float integrate(float cosThetaO, float eta, float roughness, int numSamples = 1024)
  {
    int n = sqrt((float)numSamples);
    float sum = 0.f;
    for (int i = 0; i < n; i++)
    {
      for (int j = 0; j < n; j++)
      {
        Vec2f s = min((Vec2f(i, j) + 0.5f) / (float)n, Vec2f(1.f - 1e-6f));
        sum += sample(cosThetaO, eta, roughness, s);
      }
    }

    return min(sum / (n*n), 1.f);
  }
};

struct MicrofacetConductorArtisticECAlbedo
{
  static prt_inline float sample(float cosThetaO, float r, float g, float roughness, const Vec2f& s)
  {
    // Handle edge cases
    cosThetaO = max(cosThetaO, 1e-6f);

    // Make an outgoing vector
    Vec3f wo(cos2sin(cosThetaO), 0.f, cosThetaO);

    // Sample the microfacet normal
    GGXDistribution<float> microfacet(roughnessToAlpha(roughness));
    float whPdf;
    Vec3f wh = microfacet.sampleVisible(wo, whPdf, s);

    float cosThetaOH = dot(wo, wh);

    // Sample the reflection
    Vec3f wi = reflect(wo, wh, cosThetaOH);
    float cosThetaI = wi.z;
    if (cosThetaI <= 0.f)
      return 0.f;

    float eta, k;
    fresnelConductorArtisticToIOR(r, g, eta, k);
    float F = fresnelConductor(cosThetaOH, eta, k);

    float G = microfacet.evalG2(wo, wi);

    // Energy compensation
    float Eavg = MicrofacetAlbedoTable::evalAvg(roughness);
    float Favg = fresnelConductorArtisticAvg(r, g);
    float fmsScale = sqr(Favg) * Eavg / (1.f - Favg * (1.f - Eavg)); // Stephen Hill's tweak

    float Eo = MicrofacetAlbedoTable::eval(cosThetaO, roughness);
    float Ei = MicrofacetAlbedoTable::eval(cosThetaI, roughness);
    float fms = fmsScale * ((1.f - Eo) * (1.f - Ei) * rcp(float(pi) * (1.f - Eavg)) * cosThetaI);

    float pdf = whPdf * rcp(4.f*abs(cosThetaOH));
    return F * (G * rcpSafe(microfacet.evalG1(wo))) + (fms * rcp(pdf));
  }

  static float integrate(float cosThetaO, float r, float g, float roughness, int numSamples = 1024)
  {
    int n = sqrt((float)numSamples);
    float sum = 0.f;
    for (int i = 0; i < n; i++)
    {
      for (int j = 0; j < n; j++)
      {
        Vec2f s = min((Vec2f(i, j) + 0.5f) / (float)n, Vec2f(1.f - 1e-6f));
        sum += sample(cosThetaO, r, g, roughness, s);
      }
    }

    return min(sum / (n*n), 1.f);
  }
};

struct MicrofacetSheenAlbedo
{
  static prt_inline float sample(float cosThetaO, const SheenDistribution<float>& microfacet, const Vec2f& s)
  {
    // Handle edge cases
    cosThetaO = max(cosThetaO, 1e-6f);

    // Make an outgoing vector
    Vec3f wo(cos2sin(cosThetaO), 0.f, cosThetaO);

    // Sample the reflection
    Vec3f wi = uniformSampleHemisphere(s);
    float cosThetaI = wi.z;

    // Compute the microfacet normal
    Vec3f wh = normalize(wo + wi);
    float cosThetaH = wh.z;
    float cosThetaOH = dot(wo, wh);

    // Evaluate the reflection
    float D = microfacet.eval(cosThetaH);
    float G = microfacet.evalG2(cosThetaO, cosThetaI);

    float pdf = uniformSampleHemispherePdf();
    return D * G * rcp(4.f*cosThetaO * pdf);
  }

  static float integrate(float cosThetaO, float roughness, int numSamples = 1024)
  {
    SheenDistribution<float> microfacet(roughnessToAlpha(roughness));

    int n = sqrt((float)numSamples);
    float sum = 0.f;
    for (int i = 0; i < n; i++)
    {
      for (int j = 0; j < n; j++)
      {
        Vec2f s = min((Vec2f(i, j) + 0.5f) / (float)n, Vec2f(1.f - 1e-6f));
        sum += sample(cosThetaO, microfacet, s);
      }
    }

    return min(sum / (n*n), 1.f);
  }
};

} // namespace prt
