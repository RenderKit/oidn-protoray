// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "tone_mapper.h"

namespace prt {

class LinearToneMapper : public ToneMapper
{
private:
  float exposure;

public:
  LinearToneMapper(const Props& props)
  {
    exposure = props.get("exposure", 1.0f);
  }

  float getExposure() const
  {
    return exposure;
  }

  Vec3f get(const Vec3f& c) const
  {
    return c * exposure;
  }

  Vec3vf get(const Vec3vf& c) const
  {
    return c * vfloat(exposure);
  }
};

} // namespace prt
