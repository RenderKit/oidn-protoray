// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/logging.h"
#include "sampling/shape_sampler.h"
#include "optics.h"
#include "bsdf.h"
#include "ggx_distribution.h"
#include "albedo_tables.h"

namespace prt {

// Microfacet dielectric BSDF with the Smith microsurface model and approximate multiple scattering.
// [Walter et al., 2007, "Microfacet Models for Refraction through Rough Surfaces"]
// [Kulla and Conty, 2017, "Revisiting Physically Based Shading at Imageworks"]
// [Jakob et al., 2014, "A Comprehensive Framework for Rendering Layered Materials", Extended Technical Report]
class RoughDielectricBsdf : public Bsdf
{
private:
  float eta; // etaO / etaI
  GGXDistribution<float> microfacet;
  float roughness;

  // Energy compensation [Kulla and Conty, 2017]
  float EavgEta, EavgRcpEta;
  float FavgEta, FavgRcpEta;
  float fmsRatio;

public:
  prt_inline RoughDielectricBsdf(const Basis3f* frame, float etaExtInt, float roughness, float anisotropy)
    : Bsdf(frame, BsdfType::GlossyReflection | BsdfType::GlossyTransmission),
      eta(etaExtInt),
      microfacet(roughnessToAlpha(roughness, anisotropy)),
      roughness(roughness)
  {
    // Energy compensation
    EavgEta = MicrofacetDielectricAlbedoTable::evalAvg(eta, roughness);
    EavgRcpEta = MicrofacetDielectricAlbedoTable::evalAvg(rcp(eta), roughness);
    FavgEta = fresnelDielectricAvg(eta);
    FavgRcpEta = fresnelDielectricAvg(rcp(eta));
    float a = (1.f - FavgEta) * rcp(1.f - EavgRcpEta);
    float b = (1.f - FavgRcpEta) * rcp(1.f - EavgEta)/* * sqr(eta)*/; // ignore solid angle compression
    float x = b * rcp(a + b);
    fmsRatio = (1.f - x * (1.f - FavgEta));
  }

  // Single-scattering lobe
  float evalSingle(const ShadingContext& ctx, const Vec3f& wo, const Vec3f& wi, float& pdf) const
  {
    float cosThetaO = dot(wo, getN());
    if (cosThetaO <= 0.f)
    {
      pdf = zero;
      return zero;
    }

    float cosThetaI = dot(wi, getN());
    if (cosThetaI * dot(wi, ctx.Ng) <= 0.f)
    {
      pdf = zero;
      return zero;
    }

    bool isRefl = cosThetaI > 0.f;

    // Compute the microfacet normal
    Vec3f wh;

    if (isRefl)
    {
      // Reflection
      wh = wi + wo;
    }
    else
    {
      // Transmission
      wh = eta*wo + wi;
    }

    wh = normalize(wh);
    float cosThetaH = dot(wh, getN());
    // Ensure that the micronormal is in the same hemisphere as the macronormal
    if (cosThetaH < 0.f)
      wh = -wh;

    float cosThetaOH = dot(wo, wh);
    float cosThetaIH = dot(wi, wh);

    // Fresnel term
    float F = fresnelDielectric(cosThetaOH, eta);

    float value;

    if (isRefl)
    {
      // Reflection
      pdf = F * rcp(4.f*abs(cosThetaOH));
      value = F * rcp(4.f*cosThetaO);
    }
    else
    {
      // Transmission
      pdf = (1.f-F) * rcp(sqr(eta)) * abs(cosThetaIH) * rcp(sqr(cosThetaOH + rcp(eta)*cosThetaIH));
      value = pdf * abs(cosThetaOH) * rcp(cosThetaO); // ignore solid angle compression
    }

    Vec3f wo0 = getFrame().toLocal(wo);
    Vec3f wi0 = getFrame().toLocal(wi);
    Vec3f wh0 = getFrame().toLocal(wh);

    float whPdf;
    float D = microfacet.evalVisible(wh0, wo0, cosThetaOH, whPdf);
    float G = microfacet.evalG2(wo0, wi0);

    pdf *= whPdf;
    return value * D * G;
  }

  Vec3f eval(const ShadingContext& ctx, const Vec3f& wo, const Vec3f& wi, float& pdf) const override
  {
    float cosThetaO = dot(wo, getN());
    if (cosThetaO <= 0.f)
    {
      pdf = zero;
      return zero;
    }

    float cosThetaI = dot(wi, getN());
    if (cosThetaI * dot(wi, ctx.Ng) <= 0.f)
    {
      pdf = zero;
      return zero;
    }

    bool isRefl = cosThetaI > 0.f;

    // Evaluate the energy compensation lobe [Kulla and Conty, 2017]
    float Eo = MicrofacetDielectricAlbedoTable::eval(cosThetaO, eta, roughness);

    float fmsPdf;
    float fmsValue;

    if (isRefl)
    {
      // Reflection
      float Ei = MicrofacetDielectricAlbedoTable::eval(cosThetaI, eta, roughness);
      fmsPdf = fmsRatio * cosineSampleHemispherePdf(cosThetaI);
      fmsValue = fmsRatio * (1.f - Eo) * (1.f - Ei) * rcp(float(pi) * (1.f - EavgEta)) * cosThetaI;
    }
    else
    {
      // Transmission
      float Ei = MicrofacetDielectricAlbedoTable::eval(abs(cosThetaI), rcp(eta), roughness);
      fmsPdf = (1.f - fmsRatio) * cosineSampleHemispherePdf(abs(cosThetaI));
      fmsValue = (1.f - fmsRatio) * (1.f - Eo) * (1.f - Ei) * rcp(float(pi) * (1.f - EavgRcpEta)) * abs(cosThetaI);
    }

    // Evaluate the single-scattering lobe
    float singlePdf;
    float singleValue = evalSingle(ctx, wo, wi, singlePdf);

    // Compute the final result
    float singlePickProb = Eo;
    pdf = singlePickProb * singlePdf + (1.f - singlePickProb) * fmsPdf;
    return singleValue + fmsValue;
  }

  Vec3f sample(const ShadingContext& ctx, const Vec3f& wo, Vec3f& wi, float& pdf, int& type, const BsdfSample& s) const override
  {
    float cosThetaO = dot(wo, getN());
    if (cosThetaO <= 0.f)
      return zero;

    // Energy compensation
    float Eo = MicrofacetDielectricAlbedoTable::eval(cosThetaO, eta, roughness);

    float singlePickProb = Eo;
    bool isSingle = (s.c <= singlePickProb);

    if (isSingle)
    {
      // Sample the single-scattering lobe
      Vec3f wo0 = getFrame().toLocal(wo);

      // Sample the microfacet normal
      float whPdf;
      Vec3f wh = getFrame().toGlobal(microfacet.sampleVisible(wo0, whPdf, s.v));
      float cosThetaOH = dot(wo, wh);

      // Fresnel term
      float cosThetaTH; // positive
      float F = fresnelDielectricEx(cosThetaOH, cosThetaTH, eta);

      // Sample the reflection or the transmission
      float sc1 = s.c * rcp(singlePickProb);
      bool isRefl = (sc1 <= F);

      if (isRefl)
      {
        // Reflection
        wi = reflect(wo, wh, cosThetaOH);
        if (dot(wi, getN()) <= 0.f)
          return zero;
      }
      else
      {
        // Transmission
        wi = refract(wo, wh, cosThetaOH, cosThetaTH, eta);
        if (dot(wi, getN()) >= 0.f)
          return zero;
      }
    }
    else
    {
      // Sample the energy compensation lobe
      float sc1 = (s.c - singlePickProb) * rcp(1.f - singlePickProb);
      bool isRefl = (sc1 <= fmsRatio);
      wi = getFrame().toGlobal(cosineSampleHemisphere(s.v));

      if (!isRefl)
      {
        // Transmission
        wi = -wi;
      }
    }

    float cosThetaI = dot(wi, getN());
    if (cosThetaI * dot(wi, ctx.Ng) <= 0.f)
      return zero;

    bool isRefl = cosThetaI > 0.f;
    type = isRefl ? BsdfType::GlossyReflection : BsdfType::GlossyTransmission;

    // Evaluate the energy compensation lobe
    float fmsPdf;
    float fmsValue;

    if (isRefl)
    {
      // Reflection
      float Ei = MicrofacetDielectricAlbedoTable::eval(cosThetaI, eta, roughness);
      fmsPdf = fmsRatio * cosineSampleHemispherePdf(cosThetaI);
      fmsValue = fmsRatio * (1.f - Eo) * (1.f - Ei) * rcp(float(pi) * (1.f - EavgEta)) * cosThetaI;
    }
    else
    {
      // Transmission
      float Ei = MicrofacetDielectricAlbedoTable::eval(abs(cosThetaI), rcp(eta), roughness);
      fmsPdf = (1.f - fmsRatio) * cosineSampleHemispherePdf(abs(cosThetaI));
      fmsValue = (1.f - fmsRatio) * (1.f - Eo) * (1.f - Ei) * rcp(float(pi) * (1.f - EavgRcpEta)) * abs(cosThetaI);
    }

    // Evaluate the single-scattering lobe
    float singlePdf;
    float singleValue = evalSingle(ctx, wo, wi, singlePdf);

    // Compute the final result
    pdf = singlePickProb * singlePdf + (1.f - singlePickProb) * fmsPdf;
    return (singleValue + fmsValue) * rcp(pdf);
  }
};

class RoughDielectricBsdfSimd : public BsdfSimd
{
private:
  vfloat eta; // etaO / etaI
  GGXDistribution<vfloat> microfacet;
  vfloat roughness;

  // Energy compensation [Kulla and Conty, 2017]
  vfloat EavgEta, EavgRcpEta;
  vfloat FavgEta, FavgRcpEta;
  vfloat fmsRatio;

public:
  prt_inline RoughDielectricBsdfSimd(vbool m, const Basis3vf* frame, vfloat etaExtInt, vfloat roughness, vfloat anisotropy)
    : BsdfSimd(frame, BsdfType::GlossyReflection | BsdfType::GlossyTransmission),
      eta(etaExtInt),
      microfacet(roughnessToAlpha(roughness, anisotropy)),
      roughness(roughness)
  {
    // Energy compensation
    EavgEta = MicrofacetDielectricAlbedoTable::evalAvg(m, eta, roughness);
    EavgRcpEta = MicrofacetDielectricAlbedoTable::evalAvg(m, rcp(eta), roughness);
    FavgEta = fresnelDielectricAvg(m, eta);
    FavgRcpEta = fresnelDielectricAvg(m, rcp(eta));
    vfloat a = (1.f - FavgEta) * rcp(1.f - EavgRcpEta);
    vfloat b = (1.f - FavgRcpEta) * rcp(1.f - EavgEta)/* * sqr(eta)*/; // ignore solid angle compression
    vfloat x = b * rcp(a + b);
    fmsRatio = (1.f - x * (1.f - FavgEta));
  }

  // Single-scattering lobe
  vfloat evalSingle(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo, const Vec3vf& wi, vfloat& pdf) const
  {
    vfloat value = zero;
    pdf = zero;

    vfloat cosThetaO = dot(wo, getN());
    m &= cosThetaO > 0.f;
    if (none(m))
      return value;

    vfloat cosThetaI = dot(wi, getN());
    m &= cosThetaI * dot(wi, ctx.Ng) > 0.f;
    if (none(m))
      return value;

    vbool mRefl = m & (cosThetaI > 0.f);
    vbool mTrans = andn(m, mRefl);

    // Compute the microfacet normal
    Vec3vf wh = zero;

    if (any(mRefl))
    {
      // Reflection
      set(mRefl, wh, wi + wo);
    }

    if (any(mTrans))
    {
      // Transmission
      set(mTrans, wh, eta*wo + wi);
    }

    wh = normalize(wh);
    vfloat cosThetaH = dot(wh, getN());
    // Ensure that the micronormal is in the same hemisphere as the macronormal
    vbool mCosThetaHNeg = m & (cosThetaH < 0.f);
    if (any(mCosThetaHNeg))
      set(mCosThetaHNeg, wh, -wh);

    vfloat cosThetaOH = dot(wo, wh);
    vfloat cosThetaIH = dot(wi, wh);

    // Fresnel term
    vfloat cosThetaTH; // positive
    vfloat F = fresnelDielectric(m, cosThetaOH, eta);

    if (any(mRefl))
    {
      // Reflection
      set(mRefl, pdf, F * rcp(4.f*abs(cosThetaOH)));
      set(mRefl, value, F * rcp(4.f*cosThetaO));
    }

    if (any(mTrans))
    {
      // Transmission
      set(mTrans, pdf, (1.f-F) * rcp(sqr(eta)) * abs(cosThetaIH) * rcp(sqr(cosThetaOH + rcp(eta)*cosThetaIH)));
      set(mTrans, value, pdf * abs(cosThetaOH) * rcp(cosThetaO)); // ignore solid angle compression
    }

    Vec3vf wo0 = getFrame().toLocal(wo);
    Vec3vf wi0 = getFrame().toLocal(wi);
    Vec3vf wh0 = getFrame().toLocal(wh);

    vfloat whPdf;
    vfloat D = microfacet.evalVisible(wh0, wo0, cosThetaOH, whPdf);
    vfloat G = microfacet.evalG2(wo0, wi0);

    set(m, pdf, pdf * whPdf);
    set(m, value, value * D * G);
    return value;
  }

  Vec3vf eval(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo, const Vec3vf& wi, vfloat& pdf) const override
  {
    vfloat value = zero;
    pdf = zero;

    vfloat cosThetaO = dot(wo, getN());
    m &= cosThetaO > 0.f;
    if (none(m))
      return value;

    vfloat cosThetaI = dot(wi, getN());
    m &= cosThetaI * dot(wi, ctx.Ng) > 0.f;
    if (none(m))
      return value;

    vbool mRefl = m & (cosThetaI > 0.f);
    vbool mTrans = andn(m, mRefl);

    // Evaluate the energy compensation lobe [Kulla and Conty, 2017]
    vfloat Eo = MicrofacetDielectricAlbedoTable::eval(m, cosThetaO, eta, roughness);

    vfloat fmsPdf = zero;
    vfloat fmsValue = zero;

    if (any(mRefl))
    {
      // Reflection
      vfloat Ei = MicrofacetDielectricAlbedoTable::eval(mRefl, cosThetaI, eta, roughness);
      set(mRefl, fmsPdf, fmsRatio * cosineSampleHemispherePdf(cosThetaI));
      set(mRefl, fmsValue, fmsRatio * (1.f - Eo) * (1.f - Ei) * rcp(float(pi) * (1.f - EavgEta)) * cosThetaI);
    }

    if (any(mTrans))
    {
      // Transmission
      vfloat Ei = MicrofacetDielectricAlbedoTable::eval(mTrans, abs(cosThetaI), rcp(eta), roughness);
      set(mTrans, fmsPdf, (1.f - fmsRatio) * cosineSampleHemispherePdf(abs(cosThetaI)));
      set(mTrans, fmsValue, (1.f - fmsRatio) * (1.f - Eo) * (1.f - Ei) * rcp(float(pi) * (1.f - EavgRcpEta)) * abs(cosThetaI));
    }

    // Evaluate the single-scattering lobe
    vfloat singlePdf;
    vfloat singleValue = evalSingle(m, ctx, wo, wi, singlePdf);

    // Compute the final result
    vfloat singlePickProb = Eo;
    set(m, pdf, singlePickProb * singlePdf + (1.f - singlePickProb) * fmsPdf);
    set(m, value, singleValue + fmsValue);
    return value;
  }

  Vec3vf sample(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo, Vec3vf& wi, vfloat& pdf, vint& type, const BsdfSampleSimd& s) const override
  {
    vfloat cosThetaO = dot(wo, getN());
    m &= cosThetaO > 0.f;
    if (none(m))
      return zero;

    // Energy compensation
    vfloat Eo = MicrofacetDielectricAlbedoTable::eval(m, cosThetaO, eta, roughness);

    vfloat singlePickProb = Eo;
    vbool mSingle = m & (s.c <= singlePickProb);
    vbool mFms = andn(m, mSingle);

    if (any(mSingle))
    {
      // Sample the single-scattering lobe
      Vec3vf wo0 = getFrame().toLocal(wo);

      // Sample the microfacet normal
      vfloat whPdf;
      Vec3vf wh = getFrame().toGlobal(microfacet.sampleVisible(wo0, whPdf, s.v));
      vfloat cosThetaOH = dot(wo, wh);

      // Fresnel term
      vfloat cosThetaTH; // positive
      vfloat F = fresnelDielectricEx(mSingle, cosThetaOH, cosThetaTH, eta);

      // Sample the reflection or the transmission
      vfloat sc1 = s.c * rcp(singlePickProb);
      vbool mRefl = mSingle & (sc1 <= F);
      vbool mTrans = andn(mSingle, mRefl);
      wi = zero;

      if (any(mRefl))
      {
        // Reflection
        set(mRefl, wi, reflect(wo, wh, cosThetaOH));
        mRefl &= dot(wi, getN()) > 0.f;
      }

      if (any(mTrans))
      {
        // Transmission
        set(mTrans, wi, refract(wo, wh, cosThetaOH, cosThetaTH, eta));
        mTrans &= dot(wi, getN()) < 0.f;
      }

      mSingle = mRefl | mTrans;
    }

    if (any(mFms))
    {
      // Sample the energy compensation lobe
      vfloat sc1 = (s.c - singlePickProb) * rcp(1.f - singlePickProb);
      vbool mRefl = mFms & (sc1 <= fmsRatio);
      vbool mTrans = andn(mFms, mRefl);
      Vec3vf dir = getFrame().toGlobal(cosineSampleHemisphere(s.v));

      set(mRefl, wi, dir);
      set(mTrans, wi, -dir);
    }

    m = mSingle | mFms;

    vfloat cosThetaI = dot(wi, getN());
    m &= cosThetaI * dot(wi, ctx.Ng) > 0.f;
    if (none(m))
      return zero;

    vbool mRefl = m & (cosThetaI > 0.f);
    vbool mTrans = andn(m, mRefl);
    type = select(mRefl, vint(BsdfType::GlossyReflection), vint(BsdfType::GlossyTransmission));

    // Evaluate the energy compensation lobe
    vfloat fmsPdf = zero;
    vfloat fmsValue = zero;

    if (any(mRefl))
    {
      // Reflection
      vfloat Ei = MicrofacetDielectricAlbedoTable::eval(mRefl, cosThetaI, eta, roughness);
      set(mRefl, fmsPdf, fmsRatio * cosineSampleHemispherePdf(cosThetaI));
      set(mRefl, fmsValue, fmsRatio * (1.f - Eo) * (1.f - Ei) * rcp(float(pi) * (1.f - EavgEta)) * cosThetaI);
    }

    if (any(mTrans))
    {
      // Transmission
      vfloat Ei = MicrofacetDielectricAlbedoTable::eval(mTrans, abs(cosThetaI), rcp(eta), roughness);
      set(mTrans, fmsPdf, (1.f - fmsRatio) * cosineSampleHemispherePdf(abs(cosThetaI)));
      set(mTrans, fmsValue, (1.f - fmsRatio) * (1.f - Eo) * (1.f - Ei) * rcp(float(pi) * (1.f - EavgRcpEta)) * abs(cosThetaI));
    }

    // Evaluate the single-scattering lobe
    vfloat singlePdf;
    vfloat singleValue = evalSingle(m, ctx, wo, wi, singlePdf);

    // Compute the final result
    vfloat weight = zero;
    pdf = zero;

    set(m, pdf, singlePickProb * singlePdf + (1.f - singlePickProb) * fmsPdf);
    set(m, weight, (singleValue + fmsValue) * rcp(pdf));
    return weight;
  }

  Vec3vf getAlbedo(vbool m) const override
  {
    return vfloat(1.f);
  }

  Vec3vf getSpecularAlbedo(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo) const override
  {
    vfloat cosThetaO = dot(wo, getN());
    m &= cosThetaO > 0.f;
    if (none(m))
      return zero;

    vfloat Ro = MicrofacetDielectricReflectionECAlbedoTable::eval(m, cosThetaO, eta, roughness);
    return select(m, Ro, zero);
  }

  vfloat getRoughness(vbool m) const override
  {
    return roughness;
  }
};

} // namespace prt
