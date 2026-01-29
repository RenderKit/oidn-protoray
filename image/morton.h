// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "math/math.h"
#include "math/vec2.h"

namespace prt {

extern const int mortonTableX[];
extern const int mortonTableY[];

template <int tileSizeX>
prt_inline Vec2i getMorton8x8(int i)
{
  static const int logTileSizeX = bitScan(tileSizeX);
  int mortonId = i & 63;
  return Vec2i(mortonTableX[mortonId] + ((i >> 3) & (tileSizeX-8)),
               mortonTableY[mortonId] + ((i >> (logTileSizeX+3)) << 3));
}

template <int tileSizeX>
prt_inline Vec2vi getMorton8x8(vint i)
{
  static const int logTileSizeX = bitScan(tileSizeX);
  vint mortonId = i & 63;
  return Vec2vi(gather(mortonTableX, mortonId) + ((i >> 3) & (tileSizeX-8)),
                gather(mortonTableY, mortonId) + ((i >> (logTileSizeX+3)) << 3));
}

template <int tileSizeX>
prt_inline Vec2vi getMorton8x8Step(int i)
{
  static const int logTileSizeX = bitScan(tileSizeX);
  int mortonId = i & 63;
  return Vec2vi(load(mortonTableX + mortonId) + ((i >> 3) & (tileSizeX-8)),
                load(mortonTableY + mortonId) + ((i >> (logTileSizeX+3)) << 3));
}

} // namespace prt
