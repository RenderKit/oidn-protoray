// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "intersector.h"
#include "ray.h"

namespace prt {

class IntersectorSingle : public Intersector
{
public:
  virtual void intersect(Ray& ray, Hit& hit, RayStats& stats,
                         RayHint hint = RayHint::None,
                         IntersectorFilter* filter = nullptr,
                         void* payload = nullptr) = 0;

  virtual void occluded(Ray& ray, RayStats& stats,
                        RayHint hint = RayHint::None,
                        IntersectorFilter* filter = nullptr,
                        void* payload = nullptr) = 0;
};

} // namespace prt
