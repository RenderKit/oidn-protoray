// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "optics.h"
#include "bsdf.h"
#include "ggx_distribution.h"
#include "albedo_tables.h"

namespace prt {

// Thin microfacet dielectric BSDF with the Smith microsurface model.
// It represents a transparent slab with unit thickness, and ignores refraction.
// [Walter et al., 2007, "Microfacet Models for Refraction through Rough Surfaces"]
// [Kulla and Conty, 2017, "Revisiting Physically Based Shading at Imageworks"]
// [Jakob et al., 2014, "A Comprehensive Framework for Rendering Layered Materials", Extended Technical Report]
// FIXME: add rough transmission
class ThinRoughDielectricBsdf : public Bsdf
{
private:
  float eta;
  GGXDistribution<float> microfacet;
  float roughness;
  Vec3f color;
  float thickness;

  // Energy compensation [Kulla and Conty, 2017]
  float Eavg;
  float fmsScale;

public:
  prt_inline ThinRoughDielectricBsdf(const Basis3f* frame, float etaExtInt, float roughness, float anisotropy, const Vec3f& color, float thickness)
    : Bsdf(frame, BsdfType::GlossyReflection | BsdfType::UnbentTransmission),
      eta(etaExtInt),
      microfacet(roughnessToAlpha(roughness, anisotropy)),
      roughness(roughness),
      color(color),
      thickness(thickness)
  {
    // Energy compensation
    Eavg = MicrofacetAlbedoTable::evalAvg(roughness);
    float Favg = fresnelDielectricAvg(eta);
    fmsScale = Favg * (1.f - Eavg) * rcp(1.f - Favg * Eavg);
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

    if (isRefl)
    {
      // Compute the microfacet normal
      Vec3f wh = normalize(wi + wo);
      float cosThetaH = dot(wh, getN());
      float cosThetaOH = dot(wo, wh);

      Vec3f wo0 = getFrame().toLocal(wo);
      Vec3f wi0 = getFrame().toLocal(wi);
      Vec3f wh0 = getFrame().toLocal(wh);

      float F = fresnelDielectric(cosThetaOH, eta);
      float whPdf;
      float D = microfacet.evalVisible(wh0, wo0, cosThetaOH, whPdf);
      float G = microfacet.evalG2(wo0, wi0);

      // Energy compensation
      float Eo = MicrofacetAlbedoTable::eval(cosThetaO, roughness);
      float Ei = MicrofacetAlbedoTable::eval(cosThetaI, roughness);
      float fms = fmsScale * ((1.f - Eo) * (1.f - Ei) * rcp(float(pi) * (1.f - Eavg)) * cosThetaI);

      // Energy conservation
      float R = MicrofacetDielectricReflectionAlbedoTable::eval(cosThetaO, eta, roughness)
                + fmsScale * (1.f - Eo); // add missing energy

      pdf = whPdf * rcp(4.f*abs(cosThetaOH)) * R;
      return (F * D * G * rcp(4.f*cosThetaO)) + fms;
    }
    else
    {
      pdf = zero;
      return zero;
    }
  }

  Vec3f sample(const ShadingContext& ctx, const Vec3f& wo, Vec3f& wi, float& pdf, int& type, const BsdfSample& s) const override
  {
    float cosThetaO = dot(wo, getN());
    if (cosThetaO <= 0.f)
       return zero;

    // Energy conservation
    float Eo = MicrofacetAlbedoTable::eval(cosThetaO, roughness);
    float R = MicrofacetDielectricReflectionAlbedoTable::eval(cosThetaO, eta, roughness)
              + fmsScale * (1.f - Eo); // add missing energy

    // Sample the reflection or the transmission
    if (s.c <= R)
    {
      // Reflection
      Vec3f wo0 = getFrame().toLocal(wo);

      // Sample the microfacet normal
      float whPdf;
      Vec3f wh = getFrame().toGlobal(microfacet.sampleVisible(wo0, whPdf, s.v));

      wi = reflect(wo, wh);
      float cosThetaI = dot(wi, getN());
      if ((cosThetaI <= 0.f) || (dot(wi, ctx.Ng) <= 0.f))
        return zero;
      float cosThetaOH = dot(wo, wh);
      Vec3f wi0 = getFrame().toLocal(wi);

      float F = fresnelDielectric(cosThetaOH, eta);
      float G = microfacet.evalG2(wo0, wi0);

      // Energy compensation
      float Ei = MicrofacetAlbedoTable::eval(cosThetaI, roughness);
      float fms = fmsScale * ((1.f - Eo) * (1.f - Ei) * rcp(float(pi) * (1.f - Eavg)) * cosThetaI);

      type = BsdfType::GlossyReflection;
      pdf = whPdf * rcp(4.f*abs(cosThetaOH)) * R;
      return (F * G * rcpSafe(microfacet.evalG1(wo0)) * rcp(R)) + (fms * rcp(pdf));
    }
    else
    {
      // Transmission (ignore refraction)
      wi = -wo;
      if (dot(wi, ctx.Ng) >= 0.f)
        return zero;
      type = BsdfType::UnbentTransmission;
      pdf = 1.f-R;
      return pow(color, rcp(refract(cosThetaO, eta)) * thickness); // ignore solid angle compression
    }
  }

  Vec3f getTransparency(const ShadingContext& ctx, const Vec3f& wo) const override
  {
    float cosThetaO = dot(wo, getN());
    if (cosThetaO <= 0.f)
       return zero;

    // Energy conservation
    float Eo = MicrofacetAlbedoTable::eval(cosThetaO, roughness);
    float R = MicrofacetDielectricReflectionAlbedoTable::eval(cosThetaO, eta, roughness)
              + fmsScale * (1.f - Eo); // add missing energy

    return (1.f-R) * pow(color, rcp(refract(cosThetaO, eta)) * thickness);
  }
};

class ThinRoughDielectricBsdfSimd : public BsdfSimd
{
private:
  vfloat eta;
  GGXDistribution<vfloat> microfacet;
  vfloat roughness;
  Vec3vf color;
  vfloat thickness;

  // Energy compensation [Kulla and Conty, 2017]
  vfloat Eavg;
  vfloat fmsScale;

public:
  prt_inline ThinRoughDielectricBsdfSimd(vbool m, const Basis3vf* frame, vfloat etaExtInt, vfloat roughness, vfloat anisotropy, const Vec3vf& color, vfloat thickness)
    : BsdfSimd(frame, BsdfType::GlossyReflection | BsdfType::UnbentTransmission),
      eta(etaExtInt),
      microfacet(roughnessToAlpha(roughness, anisotropy)),
      roughness(roughness),
      color(color),
      thickness(thickness)
  {
    // Energy compensation
    Eavg = MicrofacetAlbedoTable::evalAvg(m, roughness);
    vfloat Favg = fresnelDielectricAvg(m, eta);
    fmsScale = Favg * (1.f - Eavg) * rcp(1.f - Favg * Eavg);
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

    if (any(mRefl))
    {
      // Compute the microfacet normal
      Vec3vf wh = normalize(wi + wo);
      vfloat cosThetaH = dot(wh, getN());
      vfloat cosThetaOH = dot(wo, wh);

      Vec3vf wo0 = getFrame().toLocal(wo);
      Vec3vf wi0 = getFrame().toLocal(wi);
      Vec3vf wh0 = getFrame().toLocal(wh);

      vfloat F = fresnelDielectric(mRefl, cosThetaOH, eta);
      vfloat whPdf;
      vfloat D = microfacet.evalVisible(wh0, wo0, cosThetaOH, whPdf);
      vfloat G = microfacet.evalG2(wo0, wi0);

      // Energy compensation
      vfloat Eo = MicrofacetAlbedoTable::eval(mRefl, cosThetaO, roughness);
      vfloat Ei = MicrofacetAlbedoTable::eval(mRefl, cosThetaI, roughness);
      vfloat fms = fmsScale * ((1.f - Eo) * (1.f - Ei) * rcp(float(pi) * (1.f - Eavg)) * cosThetaI);

      // Energy conservation
      vfloat R = MicrofacetDielectricReflectionAlbedoTable::eval(mRefl, cosThetaO, eta, roughness)
                 + fmsScale * (1.f - Eo); // add missing energy

      set(mRefl, pdf, whPdf * rcp(4.f*abs(cosThetaOH)) * R);
      set(mRefl, value, (F * D * G * rcp(4.f*cosThetaO)) + fms);
    }

    return value;
  }

  Vec3vf sample(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo, Vec3vf& wi, vfloat& pdf, vint& type, const BsdfSampleSimd& s) const override
  {
    vfloat cosThetaO = dot(wo, getN());
    m &= cosThetaO > 0.f;
    if (none(m))
      return zero;

    // Energy conservation
    vfloat Eo = MicrofacetAlbedoTable::eval(m, cosThetaO, roughness);
    vfloat R = MicrofacetDielectricReflectionAlbedoTable::eval(m, cosThetaO, eta, roughness)
               + fmsScale * (1.f - Eo); // add missing energy

    // Sample the reflection or the transmission
    Vec3vf weight = zero;
    pdf = zero;
    wi = zero;

    vbool mRefl = m & (s.c <= R);
    vbool mTrans = andn(m, mRefl);

    if (any(mRefl))
    {
      // Reflection
      Vec3vf wo0 = getFrame().toLocal(wo);

      // Sample the microfacet normal
      vfloat whPdf;
      Vec3vf wh = getFrame().toGlobal(microfacet.sampleVisible(wo0, whPdf, s.v));

      wi = reflect(wo, wh);
      vfloat cosThetaI = dot(wi, getN());
      mRefl &= (cosThetaI > 0.f) & (dot(wi, ctx.Ng) > 0.f);

      if (any(mRefl))
      {
        vfloat cosThetaOH = dot(wo, wh);
        Vec3vf wi0 = getFrame().toLocal(wi);

        vfloat F = fresnelDielectric(mRefl, cosThetaOH, eta);
        vfloat G = microfacet.evalG2(wo0, wi0);

        // Energy compensation
        vfloat Ei = MicrofacetAlbedoTable::eval(mRefl, cosThetaI, roughness);
        vfloat fms = fmsScale * ((1.f - Eo) * (1.f - Ei) * rcp(float(pi) * (1.f - Eavg)) * cosThetaI);

        set(mRefl, type, vint(BsdfType::GlossyReflection));
        set(mRefl, pdf, whPdf * rcp(4.f*abs(cosThetaOH)) * R);
        set(mRefl, weight, Vec3vf((F * G * rcpSafe(microfacet.evalG1(wo0)) * rcp(R)) + (fms * rcp(pdf))));
      }
    }

    if (any(mTrans))
    {
      // Transmission (ignore refraction)
      set(mTrans, wi, -wo);
      mTrans &= dot(wi, ctx.Ng) < 0.f;
      set(mTrans, type, vint(BsdfType::UnbentTransmission));
      set(mTrans, pdf, 1.f-R);
      set(mTrans, weight, pow(color, rcp(refract(cosThetaO, eta)) * thickness)); // ignore solid angle compression
    }

    return weight;
  }

  Vec3vf getTransparency(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo) const override
  {
    vfloat cosThetaO = dot(wo, getN());
    m &= cosThetaO > 0.f;
    if (none(m))
      return zero;

    // Energy conservation
    vfloat Eo = MicrofacetAlbedoTable::eval(m, cosThetaO, roughness);
    vfloat R = MicrofacetDielectricReflectionAlbedoTable::eval(m, cosThetaO, eta, roughness)
               + fmsScale * (1.f - Eo); // add missing energy

    Vec3vf value = (1.f-R) * pow(color, rcp(refract(cosThetaO, eta)) * thickness);
    return select(m, value, Vec3vf(zero));
  }

  Vec3vf getAlbedo(vbool m) const override
  {
    return pow(color, vfloat(thickness));
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
