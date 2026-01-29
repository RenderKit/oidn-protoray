// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "material.h"
#include "flakes.h"

namespace prt {

class CarPaintMaterial : public Material
{
private:
  // Base
  MaterialParam3f baseColorPar;
  MaterialParam1f roughnessPar;
  MaterialParamNormal normalPar;

  // Flakes
  std::shared_ptr<Flakes> flakes;
  MaterialParam1f flakeRoughnessPar;

  // Clear coat
  MaterialParam1f coatPar;
  float coatIor;
  MaterialParam3f coatColorPar;
  MaterialParam1f coatThicknessPar;
  MaterialParam1f coatRoughnessPar;
  MaterialParamNormal coatNormalPar;

  // Flip-flop
  MaterialParam3f flipflopColorPar;
  MaterialParam1f flipflopFalloffPar;

  float outsideIor;

#ifdef PRT_MUTATION_SUPPORT
  // Original values
  float coatIorOrg;
#endif

  static constexpr float eps = 1e-5f;

public:
  CarPaintMaterial(const Props& props, MaterialFactory& factory);

  Bsdf* getBsdf(ShadingContext& ctx, const Vec3f& wo) const override;
  BsdfSimd* getBsdf(vbool m, ShadingContextSimd& ctx, const Vec3vf& wo) const override;

  void mutate(Random& rng) override;
};

} // namespace prt
