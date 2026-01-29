// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/props.h"
#include "core/ray.h"
#include "bsdf/bsdf.h"
#include "medium.h"
#include "material_param.h"
#include "material_factory.h"

namespace prt {

namespace MaterialType
{
  enum : int
  {
    Transparent    = 1 << 0, // alpha transparency
    Cutout         = 1 << 1, // alpha transparency mask
    Transmissive   = 1 << 2,
    MediaInterface = 1 << 3,
    Emissive       = 1 << 4,
    FirstHitAov    = 1 << 30, // write all AOVs on the first non-transparent hit
  };
};

class Material
{
protected:
  int mediumIntId; // interior
  int mediumExtId; // exterior
  std::shared_ptr<Medium> mediumInt;
  std::shared_ptr<Medium> mediumExt;
  int type;

private:
  // Valid only during construction!
  const Props& props;
  MaterialFactory& factory;

public:
  Material(const Props& props, MaterialFactory& factory, int type = 0);
  virtual ~Material() {}

  //virtual Vec3f getColor(const ShadingContext& ctx) const { return zero; }
  virtual Vec3f getEmissiveColor() const { return zero; }

protected:
  virtual Bsdf* getBsdf(ShadingContext& ctx, const Vec3f& wo) const;
  virtual BsdfSimd* getBsdf(vbool m, ShadingContextSimd& ctx, const Vec3vf& wo) const;

public:
  prt_inline Bsdf* getBsdf(ShadingContext& ctx, const Vec3f& wo, int mediumId) const
  {
    // Fix orientation for media interfaces
    if (type & MaterialType::MediaInterface)
      ctx.backfacing = (mediumId == mediumIntId);
    else
      ctx.backfacing = false;

    ctx.begin();
    return getBsdf(ctx, wo);
  }

  prt_inline BsdfSimd* getBsdf(vbool m, ShadingContextSimd& ctx, const Vec3vf& wo, vint mediumId) const
  {
    // Fix orientation for media interfaces
    if (type & MaterialType::MediaInterface)
      ctx.backfacing = andn(ctx.backfacing, m) | (mediumId == mediumIntId);
    else
      ctx.backfacing = zero;

    ctx.begin();
    return getBsdf(m, ctx, wo);
  }

  prt_inline void nextMedium(const ShadingContext& ctx, const Vec3f& wo, const Vec3f& wi, int& mediumId) const
  {
    if ((type & MaterialType::MediaInterface) && (dot(wo, ctx.Ng) * dot(wi, ctx.Ng) < 0.0f))
      mediumId = (mediumId == mediumIntId) ? mediumExtId : mediumIntId;
  }

  prt_inline void nextMedium(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo, const Vec3vf& wi, vint& mediumId) const
  {
    if (type & MaterialType::MediaInterface)
    {
      vbool mUpdate = m & (dot(wo, ctx.Ng) * dot(wi, ctx.Ng) < 0.0f);
      if (any(mUpdate))
      {
        vbool mInt = mediumId == mediumIntId;
        set(mUpdate, mediumId, select(mInt, vint(mediumExtId), vint(mediumIntId)));
      }
    }
  }

  virtual Vec3f getTransparency(ShadingContext& ctx, const Vec3f& wo) const { return zero; }
  virtual Vec3vf getTransparency(vbool m, ShadingContextSimd& ctx, const Vec3vf& wo) const { return zero; }

  prt_inline int getType() const { return type; }

  virtual void mutate(Random& rng) {}

protected:
  // Gets material parameters (callable only in the constructor!)
  MaterialParam3f getParam3f(const std::string& name, const Vec3f& def, ColorSpace space = ColorSpace::Srgb); // e.g. color, albedo
  MaterialParam1f getParam1f(const std::string& name, float def, ColorSpace space = ColorSpace::Linear); // e.g. roughness, metallic
  MaterialParam1f getParamAlpha(const std::string& name, float def, bool hasScale = true);
  MaterialParamNormal getParamNormal(const std::string& normalName, const std::string& bumpName);

private:
  ImageTextureParams getImageTextureParams(const std::string& name, ColorSpace space);
};

} // namespace prt
