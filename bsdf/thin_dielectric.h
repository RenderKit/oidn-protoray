// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "optics.h"
#include "bsdf.h"

namespace prt {

// Smooth thin dielectric BSDF.
// It represents a transparent slab with unit thickness, and ignores refraction.
class ThinDielectricBsdf : public Bsdf
{
private:
  float eta;
  Vec3f color;
  float thickness;

public:
  prt_inline ThinDielectricBsdf(const Basis3f* frame, float etaExtInt, const Vec3f& color, float thickness)
    : Bsdf(frame, BsdfType::SpecularReflection | BsdfType::UnbentTransmission),
      eta(etaExtInt),
      color(color),
      thickness(thickness) {}

  Vec3f sample(const ShadingContext& ctx, const Vec3f& wo, Vec3f& wi, float& pdf, int& type, const BsdfSample& s) const override
  {
    float cosThetaO = dot(wo, getN());
    if (cosThetaO <= 0.f)
       return zero;

    // Fresnel term
    float cosThetaT; // positive
    float F = fresnelDielectricEx(cosThetaO, cosThetaT, eta);

    // Sample the reflection or the transmission
    if (s.c <= F)
    {
      // Reflection
      wi = reflect(wo, getN(), cosThetaO);
      if (dot(wi, ctx.Ng) <= 0.f)
        return zero;
      type = BsdfType::SpecularReflection;
      pdf = F;
      return 1.f;
    }
    else
    {
      // Transmission (ignore refraction)
      wi = -wo;
      if (dot(wi, ctx.Ng) >= 0.f)
        return zero;
      type = BsdfType::UnbentTransmission;
      pdf = 1.f-F;
      return pow(color, rcp(cosThetaT) * thickness);
    }
  }

  Vec3f getTransparency(const ShadingContext& ctx, const Vec3f& wo) const override
  {
    float cosThetaO = dot(wo, getN());
    if (cosThetaO <= 0.f)
       return zero;

    // Fresnel term
    float cosThetaT; // positive
    float F = fresnelDielectricEx(cosThetaO, cosThetaT, eta);

    return (1.f-F) * pow(color, rcp(cosThetaT) * thickness);
  }
};

class ThinDielectricBsdfSimd : public BsdfSimd
{
private:
  vfloat eta;
  Vec3vf color;
  vfloat thickness;

public:
  prt_inline ThinDielectricBsdfSimd(vbool m, const Basis3vf* frame, vfloat etaExtInt, const Vec3vf& color, vfloat thickness)
    : BsdfSimd(frame, BsdfType::SpecularReflection | BsdfType::UnbentTransmission),
      eta(etaExtInt),
      color(color),
      thickness(thickness) {}

  Vec3vf sample(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo, Vec3vf& wi, vfloat& pdf, vint& type, const BsdfSampleSimd& s) const override
  {
    vfloat cosThetaO = dot(wo, getN());
    m &= cosThetaO > 0.f;
    if (none(m))
      return zero;

    // Fresnel term
    vfloat cosThetaT; // positive
    vfloat F = fresnelDielectricEx(m, cosThetaO, cosThetaT, eta);

    // Sample the reflection or the transmission
    Vec3vf weight = zero;
    pdf = zero;
    wi = zero;

    vbool mRefl = m & (s.c <= F);
    vbool mTrans = andn(m, mRefl);

    if (any(mRefl))
    {
      // Reflection
      set(mRefl, wi, reflect(wo, getN(), cosThetaO));
      mRefl &= dot(wi, ctx.Ng) > 0.f;
      set(mRefl, type, vint(BsdfType::SpecularReflection));
      set(mRefl, pdf, F);
      set(mRefl, weight, Vec3vf(1.f));
    }

    if (any(mTrans))
    {
      // Transmission (ignore refraction)
      set(mTrans, wi, -wo);
      mTrans &= dot(wi, ctx.Ng) < 0.f;
      set(mTrans, type, vint(BsdfType::UnbentTransmission));
      set(mTrans, pdf, 1.f-F);
      set(mTrans, weight, pow(color, rcp(cosThetaT) * thickness)); // ignore solid angle compression
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
    vfloat cosThetaT; // positive
    vfloat F = fresnelDielectricEx(m, cosThetaO, cosThetaT, eta);

    Vec3vf value = (1.f-F) * pow(color, rcp(cosThetaT) * thickness);
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

    vfloat cosThetaT; // positive
    vfloat F = fresnelDielectricEx(m, cosThetaO, cosThetaT, eta);
    return select(m, F, zero);
  }

  vfloat getRoughness(vbool m) const override
  {
    return 0.f;
  }
};

} // namespace prt
