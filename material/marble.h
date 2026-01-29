// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "material.h"

namespace prt {

class MarbleMaterial : public Material
{
private:
  MaterialParam3f color;
  MaterialParam1f roughness;
  MaterialParamNormal normal;
  MaterialParamNormal baseNormal;

  float coatEta;
  MaterialParam3f coatColor;
  float coatThickness;
  MaterialParam1f coatRoughness;
  MaterialParamNormal coatNormal;

  float extEta;
  MaterialParam1f opacity;

public:
  MarbleMaterial(const Props& props, MaterialFactory& factory);

  Bsdf* getBsdf(ShadingContext& ctx, const Vec3f& wo) const override;
  BsdfSimd* getBsdf(vbool m, ShadingContextSimd& ctx, const Vec3vf& wo) const override;
};

} // namespace prt
