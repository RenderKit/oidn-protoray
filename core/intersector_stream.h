// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "intersector.h"
#include "ray_stream.h"

namespace prt {

template <int streamSize>
class IntersectorStream : public Intersector
{
public:
  virtual void intersect(RayStream<streamSize>& rays, HitStream<streamSize>& hits, int count, RayStats& stats,
                         RayHint hint = RayHint::None,
                         IntersectorFilterSimd* filter = nullptr,
                         void* payload = nullptr) = 0;

  virtual void occluded(RayStream<streamSize>& rays, int count, RayStats& stats,
                        RayHint hint = RayHint::None,
                        IntersectorFilterSimd* filter = nullptr,
                        void* payload = nullptr) = 0;
};

} // namespace prt
