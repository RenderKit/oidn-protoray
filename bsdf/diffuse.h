// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sampling/shape_sampler.h"
#include "bsdf.h"

namespace prt {

// Lambertian reflection

class DiffuseBrdf : public Bsdf
{
private:
  Vec3f color; // reflectance

public:
  prt_inline DiffuseBrdf(const Basis3f* frame, const Vec3f& color)
    : Bsdf(frame, BsdfType::DiffuseReflection),
      color(color) {}

  Vec3f eval(const ShadingContext& ctx, const Vec3f& wo, const Vec3f& wi, float& pdf) const override
  {
    float cosThetaI = max(dot(wi, getN()), 0.f);
    pdf = cosineSampleHemispherePdf(cosThetaI);
    return color * (1.f/float(pi)) * cosThetaI;
  }

  Vec3f sample(const ShadingContext& ctx, const Vec3f& wo, Vec3f& wi, float& pdf, int& type, const BsdfSample& s) const override
  {
    wi = getFrame() * cosineSampleHemisphere(pdf, s.v);
    if (dot(wi, ctx.Ng) <= 0.f)
      return zero;
    type = BsdfType::DiffuseReflection;
    return color;
  }

  float getImportance() const override
  {
    return reduceMax(color);
  }
};

class DiffuseBrdfSimd : public BsdfSimd
{
private:
  Vec3vf color; // reflectance

public:
  prt_inline DiffuseBrdfSimd(vbool m, const Basis3vf* frame, const Vec3vf& color)
    : BsdfSimd(frame, BsdfType::DiffuseReflection),
      color(color) {}

  Vec3vf eval(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo, const Vec3vf& wi, vfloat& pdf) const override
  {
    vfloat cosThetaI = max(dot(wi, getN()), 0.f);
    pdf = cosineSampleHemispherePdf(cosThetaI);
    return color * vfloat(1.f/float(pi)) * cosThetaI;
  }

  Vec3vf sample(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo, Vec3vf& wi, vfloat& pdf, vint& type, const BsdfSampleSimd& s) const override
  {
    wi = getFrame() * cosineSampleHemisphere(pdf, s.v);
    m &= dot(wi, ctx.Ng) > 0.f;
    type = BsdfType::DiffuseReflection;
    return select(m, color, Vec3vf(zero));
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


// Lambertian transmission

class DiffuseBtdf : public Bsdf
{
private:
  Vec3f color; // transmission

public:
  prt_inline DiffuseBtdf(const Basis3f* frame, const Vec3f& color)
    : Bsdf(frame, BsdfType::DiffuseTransmission),
      color(color) {}

  Vec3f eval(const ShadingContext& ctx, const Vec3f& wo, const Vec3f& wi, float& pdf) const override
  {
    float cosThetaI = max(-dot(wi, getN()), 0.f);
    pdf = cosineSampleHemispherePdf(cosThetaI);
    return color * (1.f/float(pi)) * cosThetaI;
  }

  Vec3f sample(const ShadingContext& ctx, const Vec3f& wo, Vec3f& wi, float& pdf, int& type, const BsdfSample& s) const override
  {
    wi = getFrame() * -cosineSampleHemisphere(pdf, s.v);
    if (dot(wi, ctx.Ng) > 0.f)
      return zero;
    type = BsdfType::DiffuseTransmission;
    return color;
  }

  float getImportance() const override
  {
    return reduceMax(color);
  }
};

class DiffuseBtdfSimd : public BsdfSimd
{
private:
  Vec3vf color; // transmission

public:
  prt_inline DiffuseBtdfSimd(vbool m, const Basis3vf* frame, const Vec3vf& color)
    : BsdfSimd(frame, BsdfType::DiffuseTransmission),
      color(color) {}

  Vec3vf eval(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo, const Vec3vf& wi, vfloat& pdf) const override
  {
    vfloat cosThetaI = max(-dot(wi, getN()), 0.f);
    pdf = cosineSampleHemispherePdf(cosThetaI);
    return color * vfloat(1.f/float(pi)) * cosThetaI;
  }

  Vec3vf sample(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo, Vec3vf& wi, vfloat& pdf, vint& type, const BsdfSampleSimd& s) const override
  {
    wi = getFrame() * -cosineSampleHemisphere(pdf, s.v);
    m &= dot(wi, ctx.Ng) <= 0.f;
    type = BsdfType::DiffuseTransmission;
    return select(m, color, Vec3vf(zero));
  }

  vfloat getImportance(vbool m) const override
  {
    return reduceMax(color);
  }

  Vec3vf getAlbedo(vbool m) const override
  {
    return select(m, color, Vec3vf(zero));
  }

  Vec3vf getDiffuseAlbedo(vbool m) const override
  {
    return select(m, color, Vec3vf(zero));
  }

  vfloat getRoughness(vbool m) const override
  {
    return 1.f;
  }
};

} // namespace prt
