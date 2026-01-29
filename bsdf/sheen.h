// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sampling/shape_sampler.h"
#include "optics.h"
#include "bsdf.h"
#include "sheen_distribution.h"
#include "albedo_tables.h"

namespace prt {

// Microfacet sheen BSDF for cloth-like materials.
// [Conty and Kulla, 2017, "Production Friendly Microfacet Sheen BRDF"]
// [Kulla and Conty, 2017, "Revisiting Physically Based Shading at Imageworks"]
class SheenBsdf : public Bsdf
{
private:
  const Bsdf* base; // substrate
  Vec3f color;
  SheenDistribution<float> microfacet;
  float roughness;

  float scale;

public:
  prt_inline SheenBsdf(const Basis3f* frame, const Bsdf* base, const Vec3f& color, float roughness, float scale)
    : Bsdf(frame, base->getFlags() | BsdfType::DiffuseReflection),
      base(base),
      color(color),
      microfacet(roughnessToAlpha(roughness)),
      roughness(roughness),
      scale(scale) {}

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

    // Evaluate the base
    // Ignore refraction
    float basePdf;
    Vec3f baseValue = base->eval(ctx, wo, wi, basePdf);

    // Energy conservation
    float Ro = MicrofacetSheenAlbedoTable::eval(cosThetaO, roughness) * scale;
    float Ri = MicrofacetSheenAlbedoTable::eval(abs(cosThetaI), roughness) * scale;
    float T = min(1.f - Ro, 1.f - Ri);
    baseValue *= T;

    float coatImportance = Ro * reduceMax(color);
    float baseImportance = (1.f - Ro) * base->getImportance();
    float coatPickProb = coatImportance * rcp(coatImportance + baseImportance);
    float basePickProb = 1.f - coatPickProb;

    if (cosThetaI > 0.f)
    {
      // Compute the microfacet normal
      Vec3f wh = normalize(wo + wi);
      float cosThetaH = dot(wh, getN());
      float cosThetaOH = dot(wo, wh);

      // Evaluate the coating reflection
      float D = microfacet.eval(cosThetaH);
      float G = microfacet.evalG2(cosThetaO, cosThetaI);

      float coatPdf = uniformSampleHemispherePdf();
      Vec3f coatValue = color * (D * G * rcp(4.f*cosThetaO) * scale);

      // Compute the total reflection
      pdf = coatPickProb * coatPdf + basePickProb * basePdf;
      Vec3f value = coatValue + baseValue;
      return value;
    }
    else
    {
      // Return the base transmission
      pdf = basePdf * basePickProb;
      return baseValue;
    }
  }

  Vec3f sample(const ShadingContext& ctx, const Vec3f& wo, Vec3f& wi, float& pdf, int& type, const BsdfSample& s) const override
  {
    float cosThetaO = dot(wo, getN());
    if (cosThetaO <= 0.f)
      return zero;

    // Energy conservation
    float Ro = MicrofacetSheenAlbedoTable::eval(cosThetaO, roughness) * scale;

    // Sample the coating or the base
    float basePdf;
    Vec3f baseValue;

    float coatImportance = Ro * reduceMax(color);
    float baseImportance = (1.f - Ro) * base->getImportance();
    float coatPickProb = coatImportance * rcp(coatImportance + baseImportance);
    float basePickProb = 1.f - coatPickProb;

    if (s.c <= coatPickProb)
    {
      // Sample the coating reflection
      type = BsdfType::DiffuseReflection;
      wi = getFrame().toGlobal(uniformSampleHemisphere(s.v));

      // Evaluate the base
      // Ignore refraction
      baseValue = base->eval(ctx, wo, wi, basePdf);
    }
    else
    {
      // Sample the base
      // Ignore refraction
      BsdfSample s1 = s;
      s1.c = (s1.c - coatPickProb) * rcp(basePickProb); // reallocate sample
      Vec3f baseWeight = base->sample(ctx, wo, wi, basePdf, type, s1);
      if (reduceMax(baseWeight) <= 0.f)
        return zero;
      baseValue = baseWeight * ((type & BsdfType::Delta) ? 1.f : basePdf); // correctly handle delta distributions
    }

    float cosThetaI = dot(wi, getN());
    if (cosThetaI * dot(wi, ctx.Ng) <= 0.f)
      return zero;

    // Energy conservation
    float Ri = MicrofacetSheenAlbedoTable::eval(abs(cosThetaI), roughness) * scale;
    float T = min(1.f - Ro, 1.f - Ri);
    baseValue *= T;

    if (type & BsdfType::Delta)
    {
      // If we sampled a delta distribution, we don't have to evaluate the coating reflection
      pdf = basePdf;
      return baseValue * rcp(basePickProb);
    }
    else if (cosThetaI <= 0.f)
    {
      // If we sampled transmission, we just have to return the base transmission
      pdf = basePickProb * basePdf;
      return baseValue * rcp(pdf);
    }
    else
    {
      // Compute the microfacet normal
      Vec3f wh = normalize(wo + wi);
      float cosThetaH = dot(wh, getN());
      float cosThetaOH = dot(wo, wh);

      // Evaluate the coating reflection
      float D = microfacet.eval(cosThetaH);
      float G = microfacet.evalG2(cosThetaO, cosThetaI);

      float coatPdf = uniformSampleHemispherePdf();
      Vec3f coatValue = color * (D * G * rcp(4.f*cosThetaO) * scale);

      // Compute the total reflection
      pdf = coatPickProb * coatPdf + basePickProb * basePdf;
      return (coatValue + baseValue) * rcp(pdf);
    }
  }

  Vec3f getTransparency(const ShadingContext& ctx, const Vec3f& wo) const override
  {
    float cosThetaO = dot(wo, getN());
    if (cosThetaO <= 0.f)
      return zero;

    // Evaluate the base
    Vec3f baseValue = base->getTransparency(ctx, wo);

    // Energy conservation
    float Ro = MicrofacetSheenAlbedoTable::eval(cosThetaO, roughness) * scale; // = Ri
    float T = 1.f - Ro;
    baseValue *= T;

    return baseValue;
  }

  float getImportance() const override
  {
    return max(reduceMax(color), base->getImportance());
  }
};

class SheenBsdfSimd : public BsdfSimd
{
private:
  const BsdfSimd* base; // substrate
  Vec3vf color;
  SheenDistribution<vfloat> microfacet;
  vfloat roughness;

  vfloat scale;

public:
  prt_inline SheenBsdfSimd(vbool m, const Basis3vf* frame, const BsdfSimd* base, const Vec3vf& color, vfloat roughness, vfloat scale)
    : BsdfSimd(frame, base->getFlags() | BsdfType::DiffuseReflection),
      base(base),
      color(color),
      microfacet(roughnessToAlpha(roughness)),
      roughness(roughness),
      scale(scale) {}

  Vec3vf eval(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo, const Vec3vf& wi, vfloat& pdf) const override
  {
    Vec3vf value = zero;
    pdf = zero;

    vfloat cosThetaO = dot(wo, getN());
    m &= cosThetaO > 0.f;
    if (none(m))
      return zero;

    vfloat cosThetaI = dot(wi, getN());
    m &= cosThetaI * dot(wi, ctx.Ng) > 0.f;
    if (none(m))
      return value;

    // Evaluate the base
    // Ignore refraction
    vfloat basePdf;
    Vec3vf baseValue = base->eval(m, ctx, wo, wi, basePdf);

    // Energy conservation
    vfloat Ro = MicrofacetSheenAlbedoTable::eval(m, cosThetaO, roughness) * scale;
    vfloat Ri = MicrofacetSheenAlbedoTable::eval(m, abs(cosThetaI), roughness) * scale;
    vfloat T = min(1.f - Ro, 1.f - Ri);
    baseValue *= T;

    vfloat coatImportance = Ro * reduceMax(color);
    vfloat baseImportance = (1.f - Ro) * base->getImportance(m);
    vfloat coatPickProb = coatImportance * rcp(coatImportance + baseImportance);
    vfloat basePickProb = 1.f - coatPickProb;

    vbool mRefl = m & (cosThetaI > 0.f);
    vbool mTrans = andn(m, mRefl);

    if (any(mRefl))
    {
      // Compute the microfacet normal
      Vec3vf wh = normalize(wo + wi);
      vfloat cosThetaH = dot(wh, getN());
      vfloat cosThetaOH = dot(wo, wh);

      // Evaluate the coating reflection
      vfloat D = microfacet.eval(cosThetaH);
      vfloat G = microfacet.evalG2(cosThetaO, cosThetaI);

      vfloat coatPdf = uniformSampleHemispherePdf();
      Vec3vf coatValue = color * (D * G * rcp(4.f*cosThetaO) * scale);

      // Compute the total reflection
      set(mRefl, pdf, coatPickProb * coatPdf + basePickProb * basePdf);
      set(mRefl, value, coatValue + baseValue);
    }

    if (any(mTrans))
    {
      // Return the base transmission
      set(mTrans, pdf, basePdf * basePickProb);
      set(mTrans, value, baseValue);
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
    vfloat Ro = MicrofacetSheenAlbedoTable::eval(m, cosThetaO, roughness) * scale;

    // Sample the coating or the base
    Vec3vf weight = zero;
    wi = zero;
    pdf = zero;
    type = zero;

    vfloat basePdf = zero;
    Vec3vf baseValue = zero;

    vfloat coatImportance = Ro * reduceMax(color);
    vfloat baseImportance = (1.f - Ro) * base->getImportance(m);
    vfloat coatPickProb = coatImportance * rcp(coatImportance + baseImportance);
    vfloat basePickProb = 1.f - coatPickProb;

    vbool mCoat = m & (s.c <= coatPickProb);
    vbool mBase = andn(m, mCoat);

    if (any(mCoat))
    {
      // Sample the coating reflection
      set(mCoat, type, vint(BsdfType::DiffuseReflection));
      set(mCoat, wi, getFrame().toGlobal(uniformSampleHemisphere(s.v)));

      // Evaluate the base
      // Ignore refraction
      set(mCoat, baseValue, base->eval(mCoat, ctx, wo, wi, basePdf));
    }

    if (any(mBase))
    {
      // Sample the base
      // Ignore refraction
      BsdfSampleSimd s1 = s;
      s1.c = (s1.c - coatPickProb) * rcp(basePickProb); // reallocate sample
      Vec3vf baseWi;
      vfloat basePdf2;
      vint baseType;
      Vec3vf baseWeight = base->sample(mBase, ctx, wo, baseWi, basePdf2, baseType, s1);
      mBase &= reduceMax(baseWeight) > 0.0f;
      if (any(mBase))
      {
        set(mBase, wi, baseWi);
        set(mBase, basePdf, basePdf2);
        set(mBase, type, baseType);
        set(mBase, baseValue, baseWeight * select((type & BsdfType::Delta) != 0, vfloat(1.f), basePdf)); // correctly handle delta distributions
      }
    }

    m = mCoat | mBase;
    if (none(m))
      return weight;

    vfloat cosThetaI = dot(wi, getN());
    m &= cosThetaI * dot(wi, ctx.Ng) > 0.f;
    if (none(m))
      return zero;

    // Energy conservation
    vfloat Ri = MicrofacetSheenAlbedoTable::eval(m, abs(cosThetaI), roughness) * scale;
    vfloat T = min(1.f - Ro, 1.f - Ri);
    baseValue *= T;

    vbool mCond = m & ((type & BsdfType::Delta) != 0);
    if (any(mCond))
    {
      // If we sampled a delta distribution, we don't have to evaluate the coating reflection
      set(mCond, pdf, basePdf);
      set(mCond, weight, baseValue * rcp(basePickProb));
    }

    m = andn(m, mCond); // else
    mCond = m & (cosThetaI <= 0.f);
    if (any(mCond))
    {
      // If we sampled transmission, we just have to return the base transmission
      set(mCond, pdf, basePickProb * basePdf);
      set(mCond, weight, baseValue * rcp(pdf));
    }

    m = andn(m, mCond); // else
    mCond = m;
    if (any(mCond))
    {
      // Compute the microfacet normal
      Vec3vf wh = normalize(wo + wi);
      vfloat cosThetaH = dot(wh, getN());
      vfloat cosThetaOH = dot(wo, wh);

      // Evaluate the coating reflection
      vfloat D = microfacet.eval(cosThetaH);
      vfloat G = microfacet.evalG2(cosThetaO, cosThetaI);

      vfloat coatPdf = uniformSampleHemispherePdf();
      Vec3vf coatValue = color * (D * G * rcp(4.f*cosThetaO) * scale);

      // Compute the total reflection
      set(mCond, pdf, coatPickProb * coatPdf + basePickProb * basePdf);
      set(mCond, weight, (coatValue + baseValue) * rcp(pdf));
    }

    return weight;
  }

  Vec3vf getTransparency(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo) const override
  {
    vfloat cosThetaO = dot(wo, getN());
    m &= cosThetaO > 0.f;
    if (none(m))
      return zero;

    // Evaluate the base
    Vec3vf baseValue = base->getTransparency(m, ctx, wo);

    // Energy conservation
    vfloat Ro = MicrofacetSheenAlbedoTable::eval(m, cosThetaO, roughness) * scale; // = Ri
    vfloat T = 1.f - Ro;
    baseValue *= T;

    return select(m, baseValue, Vec3vf(zero));
  }

  vfloat getImportance(vbool m) const override
  {
    return max(reduceMax(color), base->getImportance(m));
  }

  Vec3vf getAlbedo(vbool m) const override
  {
    return base->getAlbedo(m);
  }

  Vec3vf getDiffuseAlbedo(vbool m) const override
  {
    return base->getDiffuseAlbedo(m);
  }

  Vec3vf getSpecularAlbedo(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo) const override
  {
    vfloat cosThetaO = dot(wo, getN());
    m &= cosThetaO > 0.f;
    if (none(m))
      return zero;

    Vec3vf baseValue = base->getSpecularAlbedo(m, ctx, wo);

    vfloat Ro = MicrofacetSheenAlbedoTable::eval(m, cosThetaO, roughness) * scale;
    return select(m, (1.f - Ro) * baseValue + Ro * color, Vec3vf(zero));
  }

  Vec3vf getNormal(vbool m) const override
  {
    return base->getNormal(m);
  }

  vfloat getRoughness(vbool m) const override
  {
    return base->getRoughness(m);
  }
};

} // namespace prt
