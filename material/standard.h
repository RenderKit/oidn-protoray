// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "material.h"

namespace prt {

enum class StandardMaterialMode
{
  Default,
  Legacy // emulates Principled
};

class StandardMaterial : public Material
{
private:
  StandardMaterialMode mode;

  // Base
  MaterialParam1f baseWeightPar;
  MaterialParam3f baseColorPar;
  MaterialParam1f metalnessPar;
  MaterialParam1f diffuseRoughnessPar;
  MaterialParamNormal normalPar;

  // Specular
  MaterialParam1f specularWeightPar;
  MaterialParam3f specularColorPar;
  MaterialParam1f specularRoughnessPar;
  float specularIor;

  // Transmission
  MaterialParam1f transmissionWeightPar;
  MaterialParam3f transmissionColorPar;
  float transmissionDepth;

  // Subsurface
  MaterialParam1f subsurfaceWeightPar;
  MaterialParam3f subsurfaceColorPar;
  MaterialParam1f subsurfaceAnisotropyPar;

  // Coat
  MaterialParam1f coatWeightPar;
  MaterialParam3f coatColorPar;
  MaterialParam1f coatRoughnessPar;
  float coatIor;
  MaterialParamNormal coatNormalPar;

  // Sheen
  MaterialParam1f sheenWeightPar;
  MaterialParam3f sheenColorPar;
  MaterialParam1f sheenRoughnessPar;

  // Emission
  float emissionLuminance;
  Vec3f emissionColor;

  // Geometry
  bool thinWalled;
  MaterialParam1f opacityPar;

  // Ambient
  float ambientIor;

  // Legacy
  MaterialParam1f coatThicknessPar; // DEPRECATED
  MaterialParam1f sheenTintPar; // DEPRECATED
  MaterialParam1f thicknessPar; // DEPRECATED

#ifdef PRT_MUTATION_SUPPORT
  // Original values
  float specularIorOrg;
  float coatIorOrg;
  Vec3f emissionColorOrg;
#endif

  static constexpr float eps = 1e-5f;

public:
  StandardMaterial(const Props& props, MaterialFactory& factory,
                   StandardMaterialMode mode = StandardMaterialMode::Default);

  Bsdf* getBsdf(ShadingContext& ctx, const Vec3f& wo) const override;
  BsdfSimd* getBsdf(vbool m, ShadingContextSimd& ctx, const Vec3vf& wo) const override;

  Vec3f getTransparency(ShadingContext& ctx, const Vec3f& wo) const override;
  Vec3vf getTransparency(vbool m, ShadingContextSimd& ctx, const Vec3vf& wo) const override;

  Vec3f getEmissiveColor() const override;

  void mutate(Random& rng) override;

private:
  template<bool wantOpaque>
  Bsdf* getBsdfLobes(ShadingContext& ctx, const Vec3f& wo) const;

  template<bool wantOpaque>
  BsdfSimd* getBsdfLobes(vbool m, ShadingContextSimd& ctx, const Vec3vf& wo) const;
};

} // namespace prt
