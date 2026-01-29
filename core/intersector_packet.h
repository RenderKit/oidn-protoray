// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "intersector.h"
#include "ray_simd.h"

namespace prt {

class IntersectorPacket : public Intersector
{
public:
  virtual void intersect(vbool mask, RaySimd& ray, HitSimd& hit, RayStats& stats,
                         RayHint hint = RayHint::None,
                         IntersectorFilterSimd* filter = nullptr,
                         void* payload = nullptr) = 0;

  virtual void occluded(vbool mask, RaySimd& ray, RayStats& stats,
                        RayHint hint = RayHint::None,
                        IntersectorFilterSimd* filter = nullptr,
                        void* payload = nullptr) = 0;
};

} // namespace prt
