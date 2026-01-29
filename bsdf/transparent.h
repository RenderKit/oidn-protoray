// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "optics.h"
#include "bsdf.h"

namespace prt {

class TransparentBtdf : public Bsdf
{
public:
  prt_inline TransparentBtdf()
    : Bsdf(nullptr, BsdfType::Transparency) {}

  Vec3f sample(const ShadingContext& ctx, const Vec3f& wo, Vec3f& wi, float& pdf, int& type, const BsdfSample& s) const override
  {
    // Transmission
    wi = -wo;
    type = BsdfType::Transparency;
    pdf = 1.f;
    return Vec3f(1.f);
  }

  Vec3f getTransparency(const ShadingContext& ctx, const Vec3f& wo) const override
  {
    return one;
  }
};

class TransparentBtdfSimd : public BsdfSimd
{
public:
  explicit prt_inline TransparentBtdfSimd()
    : BsdfSimd(nullptr, BsdfType::Transparency) {}

  Vec3vf sample(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo, Vec3vf& wi, vfloat& pdf, vint& type, const BsdfSampleSimd& s) const override
  {
    // Transmission
    wi = -wo;
    type = BsdfType::Transparency;
    pdf = 1.f;
    return Vec3vf(1.f);
  }

  Vec3vf getTransparency(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo) const override
  {
    return one;
  }

  Vec3vf getAlbedo(vbool m) const override
  {
    return vfloat(-1.f); // return negative value for MultiBsdf
  }

  Vec3vf getDiffuseAlbedo(vbool m) const override
  {
    return vfloat(-1.f); // return negative value for MultiBsdf
  }

  Vec3vf getSpecularAlbedo(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo) const override
  {
    return vfloat(-1.f); // return negative value for MultiBsdf
  }

  vfloat getRoughness(vbool m) const override
  {
    return -1.f; // return negative value for MultiBsdf
  }
};

} // namespace prt
