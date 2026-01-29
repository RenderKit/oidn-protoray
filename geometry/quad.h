// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "math/box3.h"
#include "vertex.h"

namespace prt {

struct Quad
{
  Vec3f v[4];

  Quad() {}

  Quad(const Vec3f& v0, const Vec3f& v1, const Vec3f& v2, const Vec3f& v3)
  {
    v[0] = v0;
    v[1] = v1;
    v[2] = v2;
    v[3] = v3;
  }
};

} // namespace prt
