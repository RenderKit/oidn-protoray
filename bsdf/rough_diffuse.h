// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sampling/shape_sampler.h"
#include "ggx_distribution.h"
#include "bsdf.h"

namespace prt {

// Oren-Nayar BRDF
// [Fujii, 2012, "A tiny improvement of Oren-Nayar reflectance model"]
class RoughDiffuseBrdf : public Bsdf
{
private:
  Vec3f color;  // reflectance
  float roughness;

public:
  prt_inline RoughDiffuseBrdf(const Basis3f* frame, const Vec3f& color, float roughness)
    : Bsdf(frame, BsdfType::DiffuseReflection),
      color(color),
      roughness(roughness) {}

  prt_inline Vec3f eval(const ShadingContext& ctx, const Vec3f& wo, const Vec3f& wi, float& pdf) const override
  {
    float cosThetaO = max(dot(wo, getN()), 0.f);
    float cosThetaI = max(dot(wi, getN()), 0.f);
    pdf = cosineSampleHemispherePdf(cosThetaI);

    float s = dot(wo, wi) - cosThetaO * cosThetaI;
    float sInvt = (s > 0.f) ? (s * rcpSafe(max(cosThetaO, cosThetaI))) : s;
    return color * (cosThetaI * (1.f + roughness * sInvt) * rcp(float(pi) + (float(pi)/2.f - 2.f/3.f) * roughness));
  }

  Vec3f sample(const ShadingContext& ctx, const Vec3f& wo, Vec3f& wi, float& pdf, int& type, const BsdfSample& s) const override
  {
    wi = getFrame() * cosineSampleHemisphere(s.v);
    if (dot(wi, ctx.Ng) <= 0.f)
      return zero;
    type = BsdfType::DiffuseReflection;
    Vec3f value = eval(ctx, wo, wi, pdf);
    return value * rcp(pdf);
  }

  float getImportance() const override
  {
    return reduceMax(color);
  }
};

class RoughDiffuseBrdfSimd : public BsdfSimd
{
private:
  Vec3vf color;  // reflectance
  vfloat roughness;

public:
  prt_inline RoughDiffuseBrdfSimd(vbool m, const Basis3vf* frame, const Vec3vf& color, vfloat roughness)
    : BsdfSimd(frame, BsdfType::DiffuseReflection),
      color(color),
      roughness(roughness) {}

  prt_inline Vec3vf eval(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo, const Vec3vf& wi, vfloat& pdf) const override
  {
    vfloat cosThetaO = max(dot(wo, getN()), 0.f);
    vfloat cosThetaI = max(dot(wi, getN()), 0.f);
    pdf = cosineSampleHemispherePdf(cosThetaI);

    vfloat s = dot(wo, wi) - cosThetaO * cosThetaI;
    vfloat sInvt = select(s > 0.f, s * rcpSafe(max(cosThetaO, cosThetaI)), s);
    return color * (cosThetaI * (1.f + roughness * sInvt) * rcp(float(pi) + (float(pi)/2.f - 2.f/3.f) * roughness));
  }

  Vec3vf sample(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo, Vec3vf& wi, vfloat& pdf, vint& type, const BsdfSampleSimd& s) const override
  {
    wi = getFrame() * cosineSampleHemisphere(pdf, s.v);
    m &= dot(wi, ctx.Ng) > 0.f;
    if (none(m))
      return zero;
    type = BsdfType::DiffuseReflection;
    Vec3vf value = eval(m, ctx, wo, wi, pdf);
    return select(m, value * rcp(pdf), Vec3vf(zero));
  }

  vfloat getImportance(vbool m) const override
  {
    return reduceMax(color);
  }

  Vec3vf getAlbedo(vbool m) const override
  {
    return color;
  }

  Vec3vf getDiffuseAlbedo(vbool m) const override
  {
    return color;
  }

  vfloat getRoughness(vbool m) const override
  {
    return 1.f;
  }
};

} // namespace prt
