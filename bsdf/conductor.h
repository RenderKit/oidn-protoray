// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "optics.h"
#include "bsdf.h"

namespace prt {

// Smooth conductor BRDF

template <class Fresnel>
class ConductorBrdf : public Bsdf
{
private:
  Fresnel fresnel;

public:
  prt_inline ConductorBrdf(const Basis3f* frame, const Fresnel& fresnel)
    : Bsdf(frame, BsdfType::SpecularReflection),
      fresnel(fresnel) {}

  Vec3f sample(const ShadingContext& ctx, const Vec3f& wo, Vec3f& wi, float& pdf, int& type, const BsdfSample& s) const
  {
    float cosThetaO = dot(wo, getN());
    if (cosThetaO <= 0.f)
       return zero;

    wi = reflect(wo, getN(), cosThetaO);
    if (dot(wi, ctx.Ng) <= 0.f)
      return zero;
    type = BsdfType::SpecularReflection;
    pdf = 1.f;
    return fresnel.eval(cosThetaO);
  }
};

template <class Fresnel>
class ConductorBrdfSimd : public BsdfSimd
{
private:
  Fresnel fresnel;

public:
  prt_inline ConductorBrdfSimd(vbool m, const Basis3vf* frame, const Fresnel& fresnel)
    : BsdfSimd(frame, BsdfType::SpecularReflection),
      fresnel(fresnel) {}

  Vec3vf sample(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo, Vec3vf& wi, vfloat& pdf, vint& type, const BsdfSampleSimd& s) const override
  {
    vfloat cosThetaO = dot(wo, getN());
    m &= cosThetaO > 0.f;
    wi = reflect(wo, getN(), cosThetaO);
    m &= dot(wi, ctx.Ng) > 0.f;
    if (none(m))
      return zero;
    type = BsdfType::SpecularReflection;
    pdf = 1.f;
    return select(m, fresnel.eval(cosThetaO), Vec3vf(zero));
  }

  Vec3vf getAlbedo(vbool m) const override
  {
    return fresnel.getAlbedo();
  }

  Vec3vf getSpecularAlbedo(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo) const override
  {
    vfloat cosThetaO = dot(wo, getN());
    m &= cosThetaO > 0.f;
    if (none(m))
      return zero;

    return select(m, fresnel.eval(cosThetaO), Vec3vf(zero));
  }

  vfloat getRoughness(vbool m) const override
  {
    return 0.f;
  }
};

} // namespace prt
