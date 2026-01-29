// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/props.h"
#include "math/vec3.h"
#include "math/simd.h"

namespace prt {

class ToneMapper
{
public:
  virtual ~ToneMapper() {}

  virtual float getExposure() const = 0;

  virtual Vec3f get(const Vec3f& c) const = 0;
  virtual Vec3vf get(const Vec3vf& c) const = 0;
};

} // namespace prt
