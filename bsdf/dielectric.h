// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "optics.h"
#include "bsdf.h"

namespace prt {

// Smooth dielectric BSDF.
class DielectricBsdf : public Bsdf
{
private:
  float eta; // etaO / etaI

public:
  prt_inline DielectricBsdf(const Basis3f* frame, float etaExtInt)
    : Bsdf(frame, BsdfType::SpecularReflection | BsdfType::SpecularTransmission),
      eta(etaExtInt) {}

  Vec3f sample(const ShadingContext& ctx, const Vec3f& wo, Vec3f& wi, float& pdf, int& type, const BsdfSample& s) const
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
    }
    else
    {
      // Transmission
      wi = refract(wo, getN(), cosThetaO, cosThetaT, eta);
      if (dot(wi, ctx.Ng) >= 0.f)
        return zero;
      type = BsdfType::SpecularTransmission;
      pdf = 1.f-F;
    }

    return 1.f; // ignore solid angle compression
  }
};

class DielectricBsdfSimd : public BsdfSimd
{
private:
  vfloat eta;

public:
  prt_inline DielectricBsdfSimd(vbool m, const Basis3vf* frame, vfloat etaExtInt)
    : BsdfSimd(frame, BsdfType::SpecularReflection | BsdfType::SpecularTransmission),
      eta(etaExtInt) {}

  Vec3vf sample(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo, Vec3vf& wi, vfloat& pdf, vint& type, const BsdfSampleSimd& s) const
  {
    vfloat cosThetaO = dot(wo, getN());
    m &= cosThetaO > 0.f;
    if (none(m))
      return zero;

    // Fresnel term
    vfloat cosThetaT; // positive
    vfloat F = fresnelDielectricEx(m, cosThetaO, cosThetaT, eta);

    // Sample the reflection or the transmission
    vfloat weight = zero;
    wi = zero;
    pdf = zero;

    vbool mRefl = m & (s.c <= F);
    vbool mTrans = andn(m, mRefl);

    if (any(mRefl))
    {
      // Reflection
      set(mRefl, wi, reflect(wo, getN(), cosThetaO));
      mRefl &= dot(wi, ctx.Ng) > 0.f;
      set(mRefl, type, vint(BsdfType::SpecularReflection));
      set(mRefl, pdf, F);
      set(mRefl, weight, vfloat(1.f));
    }

    if (any(mTrans))
    {
      // Transmission
      set(mTrans, wi, refract(wo, getN(), cosThetaO, cosThetaT, eta));
      mTrans &= dot(wi, ctx.Ng) < 0.f;
      set(mTrans, type, vint(BsdfType::SpecularTransmission));
      set(mTrans, pdf, 1.f-F);
      set(mTrans, weight, vfloat(1.f)); // ignore solid angle compression
    }

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
