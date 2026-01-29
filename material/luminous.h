// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "material.h"

namespace prt {

class LuminousMaterial : public Material
{
private:
  Vec3f color;

#ifdef PRT_MUTATION_SUPPORT
  // Original values
  Vec3f colorOrg;
#endif

public:
  LuminousMaterial(const Props& props, MaterialFactory& factory)
    : Material(props, factory)
  {
    if (factory.isEmissiveEnabled())
      type |= MaterialType::Emissive;

    color = props.get("color", Vec3f(1.f));
    color *= props.get("intensity", 1.f);

#ifdef PRT_MUTATION_SUPPORT
    colorOrg = color;
#endif
  }

  Vec3f getEmissiveColor() const override
  {
    return color;
  }

  void mutate(Random& rng) override
  {
#ifdef PRT_MUTATION_SUPPORT
    float saturation = 0.f;
    float hue = 0.f;

    if (rng.get1f() < 0.8f)
    {
      // Colored light
      saturation = (rng.get1f() < 0.7f) ? rng.get1f() : rng.get1f(0.f, 0.5f);
      hue = rng.get1f();
    }

    float scale = rng.get1f(0.f, 2.f);

    color = rgbToHsv(colorOrg);
    color.x += hue;
    color.y = saturation;
    color = hsvToRgb(color);
    color *= scale;
#endif
  }
};

} // namespace prt
