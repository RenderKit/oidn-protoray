// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "optics.h"
#include "bsdf.h"

namespace prt {

// Simplified, energy conserving Weidlich-Wilkie smooth coating BSDF.
// Refraction is ignored, but absorption is computed from the refracted ray lengths.
// [Weidlich and Wilkie, 2007, "Arbitrarily Layered Micro-Facet Surfaces"]
// [Kulla and Conty, 2017, "Revisiting Physically Based Shading at Imageworks"]
// [Kelemen and Szirmay-Kalos, 2001, "A Microfacet Based Coupled Specular-Matte BRDF Model with Importance Sampling"]
class CoatingBsdf : public Bsdf
{
private:
  const Bsdf* base; // substrate
  float eta;
  Vec3f color;      // transmittance
  float thickness;

  // Energy conservation [Kulla and Conty, 2017]
  float Favg;

  float scale;

public:
  prt_inline CoatingBsdf(const Basis3f* frame, const Bsdf* base, float eta, const Vec3f& color, float thickness, float scale)
    : Bsdf(frame, base->getFlags() | BsdfType::SpecularReflection),
      base(base),
      eta(eta <= 1.f ? eta : rcp(eta)),
      color(color),
      thickness(thickness),
      Favg(fresnelDielectricAvg(eta) * scale),
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

    // Fresnel term
    float cosThetaO1; // positive
    float Fo = fresnelDielectricEx(cosThetaO, cosThetaO1, eta) * scale;

    float coatImportance = Fo;
    float baseImportance = (1.f - Fo) * base->getImportance();
    float coatPickProb = coatImportance * rcp(coatImportance + baseImportance);
    float basePickProb = 1.f - coatPickProb;

    // Evaluate the base
    // Ignore refraction
    float basePdf;
    Vec3f baseValue = base->eval(ctx, wo, wi, basePdf);

    float cosThetaI1; // positive
    float Fi = fresnelDielectricEx(abs(cosThetaI), cosThetaI1, eta) * scale;

    // Apply the coating medium absorption
    // Use refracted angles for computing the absorption path length
    float lengthO1 = rcp(cosThetaO1);
    float lengthI1 = rcp(cosThetaI1);
    float length = lengthO1 + lengthI1;
    if (cosThetaI <= 0.f) length *= 0.5f; // for transmission, use the average length
    baseValue = lerp(baseValue, baseValue * pow(color, thickness * length), scale);

    // Energy conservation
    float T;
    if (base->getFlags() & ~BsdfType::Diffuse)
      T = min(1.f - Fo, 1.f - Fi); // for generic (non-diffuse) bases [Kulla and Conty, 2017]
    else
      T = (1.f - Fo) * (1.f - Fi) * rcp(1.f - Favg); // for diffuse bases [Kelemen and Szirmay-Kalos, 2001]
    baseValue *= T;

    pdf = basePdf * basePickProb;
    return baseValue;
  }

  Vec3f sample(const ShadingContext& ctx, const Vec3f& wo, Vec3f& wi, float& pdf, int& type, const BsdfSample& s) const override
  {
    float cosThetaO = dot(wo, getN());
    if (cosThetaO <= 0.f)
      return zero;

    // Fresnel term
    float cosThetaO1; // positive
    float Fo = fresnelDielectricEx(cosThetaO, cosThetaO1, eta) * scale;

    float coatImportance = Fo;
    float baseImportance = (1.f - Fo) * base->getImportance();
    float coatPickProb = coatImportance * rcp(coatImportance + baseImportance);
    float basePickProb = 1.f - coatPickProb;

    // Sample the coating or the base
    if (s.c <= coatPickProb)
    {
      // Sample the coating
      wi = reflect(wo, getN(), cosThetaO);
      if (dot(wi, ctx.Ng) <= 0.f)
        return zero;
      type = BsdfType::SpecularReflection;
      pdf = coatPickProb;
      return Fo * rcp(coatPickProb);
    }
    else
    {
      // Sample the base
      // Ignore refraction
      BsdfSample s1 = s;
      s1.c = (s1.c - coatPickProb) * rcp(basePickProb); // reallocate sample
      float basePdf;
      Vec3f baseWeight = base->sample(ctx, wo, wi, basePdf, type, s1);
      if (reduceMax(baseWeight) <= 0.f)
        return zero;

      float cosThetaI = dot(wi, getN());
      if (cosThetaI * dot(wi, ctx.Ng) <= 0.f)
        return zero;

      float cosThetaI1; // positive
      float Fi = fresnelDielectricEx(abs(cosThetaI), cosThetaI1, eta) * scale;

      // Apply the coating medium absorption
      // Use refracted angles for computing the absorption path length
      float lengthO1 = rcp(cosThetaO1);
      float lengthI1 = rcp(cosThetaI1);
      float length = lengthO1 + lengthI1;
      if (cosThetaI <= 0.f) length *= 0.5f; // for transmission, use the average length
      baseWeight = lerp(baseWeight, baseWeight * pow(color, thickness * length), scale);

      // Energy conservation
      float T;
      if (base->getFlags() & ~BsdfType::Diffuse)
        T = min(1.f - Fo, 1.f - Fi); // for generic (non-diffuse) bases [Kulla and Conty, 2017]
      else
        T = (1.f - Fo) * (1.f - Fi) * rcp(1.f - Favg); // for diffuse bases [Kelemen and Szirmay-Kalos, 2001]
      baseWeight *= (T * rcp(basePickProb));

      pdf = basePdf * basePickProb;
      return baseWeight;
    }
  }

  Vec3f getTransparency(const ShadingContext& ctx, const Vec3f& wo) const override
  {
    float cosThetaO = dot(wo, getN());
    if (cosThetaO <= 0.f)
      return zero;

    // Fresnel term
    float cosThetaO1; // positive
    float Fo = fresnelDielectricEx(cosThetaO, cosThetaO1, eta) * scale; // = Fi

    // Evaluate the base
    Vec3f baseValue = base->getTransparency(ctx, wo);

    // Apply the coating medium absorption
    // Use refracted angles for computing the absorption path length
    float lengthO1 = rcp(cosThetaO1); // = lengthI1
    float length = lengthO1; // for transmission, use the average length
    baseValue = lerp(baseValue, baseValue * pow(color, thickness * length), scale);

    // Energy conservation
    float T = 1.f - Fo; // for generic (non-diffuse) bases [Kulla and Conty, 2017]
    baseValue *= T;

    return baseValue;
  }

  float getImportance() const override
  {
    return max(1.f, base->getImportance());
  }
};

class CoatingBsdfSimd : public BsdfSimd
{
private:
  const BsdfSimd* base; // substrate
  vfloat eta;
  Vec3vf color;         // transmittance
  vfloat thickness;

  // Energy conservation [Kulla and Conty, 2017]
  vfloat Favg;

  vfloat scale;

public:
  prt_inline CoatingBsdfSimd(vbool m, const Basis3vf* frame, const BsdfSimd* base, vfloat eta, const Vec3vf& color, vfloat thickness, vfloat scale)
    : BsdfSimd(frame, base->getFlags() | BsdfType::SpecularReflection),
      base(base),
      eta(select(eta <= 1.f, eta, rcp(eta))),
      color(color),
      thickness(thickness),
      Favg(fresnelDielectricAvg(m, eta) * scale),
      scale(scale) {}

  Vec3vf eval(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo, const Vec3vf& wi, vfloat& pdf) const override
  {
    Vec3vf value = zero;
    pdf = zero;

    vfloat cosThetaO = dot(wo, getN());
    m &= cosThetaO > 0.f;
    if (none(m))
      return value;

    vfloat cosThetaI = dot(wi, getN());
    m &= cosThetaI * dot(wi, ctx.Ng) > 0.f;
    if (none(m))
      return value;

    // Fresnel term
    vfloat cosThetaO1; // positive
    vfloat Fo = fresnelDielectricEx(m, cosThetaO, cosThetaO1, eta) * scale;

    vfloat coatImportance = Fo;
    vfloat baseImportance = (1.f - Fo) * base->getImportance(m);
    vfloat coatPickProb = coatImportance * rcp(coatImportance + baseImportance);
    vfloat basePickProb = 1.f - coatPickProb;

    // Evaluate the base
    // Ignore refraction
    vfloat basePdf;
    Vec3vf baseValue = base->eval(m, ctx, wo, wi, basePdf);

    vfloat cosThetaI1; // positive
    vfloat Fi = fresnelDielectricEx(m, abs(cosThetaI), cosThetaI1, eta) * scale;

    // Apply the coating medium absorption
    // Use refracted angles for computing the absorption path length
    vfloat lengthO1 = rcp(cosThetaO1);
    vfloat lengthI1 = rcp(cosThetaI1);
    vfloat length = lengthO1 + lengthI1;
    vbool mTrans = m & (cosThetaI <= 0.f);
    if (any(mTrans)) set(mTrans, length, length * 0.5f); // for transmission, use the average length
    baseValue = lerp(baseValue, baseValue * pow(color, thickness * length), scale);

    // Energy conservation
    vfloat T = min(1.f - Fo, 1.f - Fi); // for generic (non-diffuse) bases [Kulla and Conty, 2017]
    vbool mDiff = m & ((base->getFlags() & ~BsdfType::Diffuse) == 0);
    if (any(mDiff))
      set(mDiff, T, (1.f - Fo) * (1.f - Fi) * rcp(1.f - Favg)); // for diffuse bases [Kelemen and Szirmay-Kalos, 2001]
    baseValue *= T;

    set(m, pdf, basePdf * basePickProb);
    set(m, value, baseValue);
    return value;
  }

  Vec3vf sample(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo, Vec3vf& wi, vfloat& pdf, vint& type, const BsdfSampleSimd& s) const override
  {
    vfloat cosThetaO = dot(wo, getN());
    m &= cosThetaO > 0.f;
    if (none(m))
      return zero;

    // Fresnel term
    vfloat cosThetaO1; // positive
    vfloat Fo = fresnelDielectricEx(m, cosThetaO, cosThetaO1, eta) * scale;

    vfloat coatImportance = Fo;
    vfloat baseImportance = (1.f - Fo) * base->getImportance(m);
    vfloat coatPickProb = coatImportance * rcp(coatImportance + baseImportance);
    vfloat basePickProb = 1.f - coatPickProb;

    // Sample the coating or the base
    Vec3vf weight = zero;
    wi = zero;
    pdf = zero;
    type = zero;

    vbool mCoat = m & (s.c <= coatPickProb);
    vbool mBase = andn(m, mCoat);

    if (any(mCoat))
    {
      // Sample the coating
      set(mCoat, wi, reflect(wo, getN(), cosThetaO));
      mCoat &= dot(wi, ctx.Ng) > 0.f;
      set(mCoat, type, vint(BsdfType::SpecularReflection));
      set(mCoat, pdf, coatPickProb);
      set(mCoat, weight, Vec3vf(Fo * rcp(coatPickProb)));
    }

    if (any(mBase))
    {
      // Sample the base
      // Ignore refraction
      BsdfSampleSimd s1 = s;
      s1.c = (s1.c - coatPickProb) * rcp(basePickProb); // reallocate sample
      Vec3vf baseWi;
      vfloat basePdf;
      vint baseType;
      Vec3vf baseWeight = base->sample(mBase, ctx, wo, baseWi, basePdf, baseType, s1);
      set(mBase, wi, baseWi);
      set(mBase, type, baseType);
      mBase &= reduceMax(baseWeight) > 0.f;

      vfloat cosThetaI = dot(wi, getN());
      mBase &= cosThetaI * dot(wi, ctx.Ng) > 0.f;

      if (any(mBase))
      {
        vfloat cosThetaI1; // positive
        vfloat Fi = fresnelDielectricEx(mBase, abs(cosThetaI), cosThetaI1, eta) * scale;

        // Apply the coating medium absorption
        // Use refracted angles for computing the absorption path length
        vfloat lengthO1 = rcp(cosThetaO1);
        vfloat lengthI1 = rcp(cosThetaI1);
        vfloat length = lengthO1 + lengthI1;
        vbool mTrans = mBase & (cosThetaI <= 0.f);
        if (any(mTrans)) set(mTrans, length, length * 0.5f); // for transmission, use the average length
        baseWeight = lerp(baseWeight, baseWeight * pow(color, thickness * length), scale);

        // Energy conservation
        vfloat T = min(1.f - Fo, 1.f - Fi); // for generic (non-diffuse) bases [Kulla and Conty, 2017]
        vbool mDiff = mBase & ((base->getFlags() & ~BsdfType::Diffuse) == 0);
        if (any(mDiff))
          set(mDiff, T, (1.f - Fo) * (1.f - Fi) * rcp(1.f - Favg)); // for diffuse bases [Kelemen and Szirmay-Kalos, 2001]
        baseWeight *= (T * rcp(basePickProb));

        set(mBase, pdf, basePdf * basePickProb);
        set(mBase, weight, baseWeight);
      }
    }

    return weight;
  }

  Vec3vf getTransparency(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo) const override
  {
    vfloat cosThetaO = dot(wo, getN());
    m &= cosThetaO > 0.f;
    if (none(m))
      return zero;

    // Fresnel term
    vfloat cosThetaO1; // positive
    vfloat Fo = fresnelDielectricEx(m, cosThetaO, cosThetaO1, eta) * scale; // = Fi

    // Evaluate the base
    Vec3vf baseValue = base->getTransparency(m, ctx, wo);

    // Apply the coating medium absorption
    // Use refracted angles for computing the absorption path length
    vfloat lengthO1 = rcp(cosThetaO1); // = lengthI1
    vfloat length = lengthO1; // for transmission, use the average length
    baseValue = lerp(baseValue, baseValue * pow(color, thickness * length), scale);

    // Energy conservation
    vfloat T = 1.f - Fo; // for generic (non-diffuse) bases [Kulla and Conty, 2017]
    baseValue *= T;

    return select(m, baseValue, Vec3vf(zero));
  }

  vfloat getImportance(vbool m) const override
  {
    return max(1.f, base->getImportance(m));
  }

  Vec3vf getAlbedo(vbool m) const override
  {
    Vec3vf baseValue = base->getAlbedo(m);
    return lerp(baseValue, baseValue * pow(color, vfloat(thickness)), scale);
  }

  Vec3vf getDiffuseAlbedo(vbool m) const override
  {
    Vec3vf baseValue = base->getDiffuseAlbedo(m);
    return lerp(baseValue, baseValue * pow(color, vfloat(thickness)), scale);
  }

  Vec3vf getSpecularAlbedo(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo) const override
  {
    vfloat cosThetaO = dot(wo, getN());
    m &= cosThetaO > 0.f;
    if (none(m))
      return zero;

    Vec3vf baseValue = base->getSpecularAlbedo(m, ctx, wo);
    baseValue = lerp(baseValue, baseValue * pow(color, vfloat(thickness)), scale);

    vfloat cosThetaO1; // positive
    vfloat Fo = fresnelDielectricEx(m, cosThetaO, cosThetaO1, eta) * scale;

    return select(m, (1.f - Fo) * baseValue + Fo, Vec3vf(zero));
  }

  Vec3vf getNormal(vbool m) const override
  {
    return base->getNormal(m);
  }

  vfloat getRoughness(vbool m) const override
  {
    return 0.f;
  }
};

} // namespace prt
