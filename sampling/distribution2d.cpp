// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "distribution2d.h"

namespace prt {

Distribution2D::Distribution2D() : size(0) {}

Distribution2D::Distribution2D(const float* f, const Vec2i& size)
{
  init(f, size);
}

void Distribution2D::init(const float* f, const Vec2i& size)
{
  this->size = size;
  xDists.alloc(size.y * (size.x+1));
  RawArray<float> fy(size.y);

  // Compute the y distribution and initialize the x distributions
  for (int y = 0; y < size.y; ++y)
  {
    fy[y] = 0.0f;
    for (int x = 0; x < size.x; ++x)
      fy[y] += f[y*size.x + x];

    Distribution1D::init(xDists.getData() + y*(size.x+1), f + y*size.x, size.x);
  }

  // Initialize the y distribution
  yDist.alloc(size.y+1);
  Distribution1D::init(yDist.getData(), fy.getData(), size.y);
}

} // namespace prt
