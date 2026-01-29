// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/common.h"

namespace prt {

struct RayStats
{
  int64_t rayCount;
  int64_t nodeCount;
  int64_t primCount;

  int64_t shadeSimdBatchCount;
  int64_t shadeSimdActiveLaneCount;

  prt_inline RayStats()
  {
    reset();
  }

  prt_inline void reset()
  {
    rayCount = 0;
    nodeCount = 0;
    primCount = 0;

    shadeSimdBatchCount = 0;
    shadeSimdActiveLaneCount = 0;
  }

  RayStats& operator +=(const RayStats& other)
  {
    rayCount += other.rayCount;
    nodeCount += other.nodeCount;
    primCount += other.primCount;

    shadeSimdBatchCount += other.shadeSimdBatchCount;
    shadeSimdActiveLaneCount += other.shadeSimdActiveLaneCount;

    return *this;
  }
};

} // namespace prt
