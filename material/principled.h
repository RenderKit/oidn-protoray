// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "material.h"

namespace prt {

// DEPRECATED: use StandardMaterial instead
class PrincipledMaterial : public Material
{
private:
  MaterialParam3f baseColorPar;
  MaterialParam3f edgeColorPar;
  MaterialParam1f metallicPar;
  MaterialParam1f diffusePar;
  MaterialParam1f specularPar;
  float ior;
  MaterialParam1f transmissionPar;
  MaterialParam3f transmissionColor;
  float transmissionDepth;
  MaterialParam1f roughnessPar;
  MaterialParamNormal normalPar;
  MaterialParamNormal baseNormalPar;

  // Clear coat
  MaterialParam1f coatPar;
  float coatIor;
  MaterialParam3f coatColorPar;
  MaterialParam1f coatThicknessPar;
  MaterialParam1f coatRoughnessPar;
  MaterialParamNormal coatNormalPar;

  // Sheen
  MaterialParam1f sheenPar;
  MaterialParam3f sheenColorPar;
  MaterialParam1f sheenTintPar;
  MaterialParam1f sheenRoughnessPar;

  // Thin
  bool thin;
  MaterialParam1f backlightPar;
  MaterialParam1f thicknessPar;

  float outsideIor;
  MaterialParam1f opacityPar;

#ifdef PRT_MUTATION_SUPPORT
  // Original values
  float iorOrg;
  float coatIorOrg;
#endif

  static constexpr float eps = 1e-5f;

public:
  PrincipledMaterial(const Props& props, MaterialFactory& factory);

  Bsdf* getBsdf(ShadingContext& ctx, const Vec3f& wo) const override;
  BsdfSimd* getBsdf(vbool m, ShadingContextSimd& ctx, const Vec3vf& wo) const override;

  Vec3f getTransparency(ShadingContext& ctx, const Vec3f& wo) const override;
  Vec3vf getTransparency(vbool m, ShadingContextSimd& ctx, const Vec3vf& wo) const override;

  void mutate(Random& rng) override;

private:
  template<bool wantOpaque>
  Bsdf* getBsdfLobes(ShadingContext& ctx, const Vec3f& wo) const;

  template<bool wantOpaque>
  BsdfSimd* getBsdfLobes(vbool m, ShadingContextSimd& ctx, const Vec3vf& wo) const;
};

} // namespace prt
