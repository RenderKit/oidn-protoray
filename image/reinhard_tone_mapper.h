// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdexcept>
#include "color.h"
#include "tone_mapper.h"

namespace prt {

class ReinhardToneMapper : public ToneMapper
{
private:
  float exposure;
  float invWhite2;

public:
  ReinhardToneMapper(const Props& props)
  {
    exposure = props.get("exposure", 1.0f);
    float burn = props.get("burn", 0.0f);
    if (burn < 0.0f || burn > 1.0f)
      throw std::invalid_argument("burn must be between 0 and 1");

    //float white = 1.0f / burn;
    //invWhite2 = 1.0f / sqr(white);
    invWhite2 = sqr(burn);
  }

  float getExposure() const
  {
    return exposure;
  }

  /*
  Vec3f get(const Vec3f& c) const
  {
    Vec3f d = c * exposure;
    float y = luminance(d);

    float yMapped = y * (1.0f + y * invWhite2) * rcp(1.0f + y);
    return d * (yMapped * rcpSafe(y));
  }

  Vec3vf get(const Vec3vf& c) const
  {
    Vec3vf d = c * vfloat(exposure);
    vfloat y = luminance(d);

    vfloat yMapped = y * (1.0f + y * invWhite2) * rcp(1.0f + y);
    return d * (yMapped * rcpSafe(y));
  }
  */

  Vec3f get(const Vec3f& c) const
  {
    Vec3f rgb = c * exposure;

    Vec3f rgbMapped = rgb * (1.0f + rgb * invWhite2) * rcp(1.0f + rgb);
    return rgb * (rgbMapped * rcpSafe(rgb));
  }

  Vec3vf get(const Vec3vf& c) const
  {
    Vec3vf rgb = c * vfloat(exposure);

    Vec3vf rgbMapped = rgb * (vfloat(1.0f) + rgb * vfloat(invWhite2)) * rcp(vfloat(1.0f) + rgb);
    return rgb * (rgbMapped * rcpSafe(rgb));
  }
};

} // namespace prt
