// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/raw_array.h"
#include "math/vec2.h"
#include "distribution1d.h"

namespace prt {

// Discrete 2D distribution
class Distribution2D
{
private:
  RawArray<float> yDist;
  RawArray<float> xDists;
  Vec2i size;

public:
  Distribution2D();
  Distribution2D(const float* f, const Vec2i& size);

  void init(const float* f, const Vec2i& size);

  prt_inline Vec2i sample(float& pdf, const Vec2f& s) const
  {
    // Sample a row
    float yPdf;
    int y = Distribution1D::sample(yDist.getData(), size.y, yPdf, s.y);

    // Use y to sample a column
    float xPdf;
    int x = Distribution1D::sample(xDists.getData() + y*(size.x+1), size.x, xPdf, s.x);

    pdf = xPdf * yPdf;
    return Vec2i(x, y);
  }

  prt_inline Vec2vi sample(vbool m, vfloat& pdf, const Vec2vf& s) const
  {
    // Sample a row
    vfloat yPdf;
    vint y = Distribution1D::sample(m, yDist.getData(), zero, size.y, yPdf, s.y);

    // Use y to sample a column
    vfloat xPdf;
    vint x = Distribution1D::sample(m, xDists.getData(), y*(size.x+1), size.x, xPdf, s.x);

    pdf = xPdf * yPdf;
    return Vec2vi(x, y);
  }

  prt_inline Vec2i sampleReuse(float& pdf, Vec2f& s) const
  {
    // Sample a row
    float yPdf;
    int y = Distribution1D::sampleReuse(yDist.getData(), size.y, yPdf, s.y);

    // Use y to sample a column
    float xPdf;
    int x = Distribution1D::sampleReuse(xDists.getData() + y*(size.x+1), size.x, xPdf, s.x);

    pdf = xPdf * yPdf;
    return Vec2i(x, y);
  }

  prt_inline Vec2vi sampleReuse(vbool m, vfloat& pdf, Vec2vf& s) const
  {
    // Sample a row
    vfloat yPdf;
    vint y = Distribution1D::sampleReuse(m, yDist.getData(), zero, size.y, yPdf, s.y);

    // Use y to sample a column
    vfloat xPdf;
    vint x = Distribution1D::sampleReuse(m, xDists.getData(), y*(size.x+1), size.x, xPdf, s.x);

    pdf = xPdf * yPdf;
    return Vec2vi(x, y);
  }

  prt_inline float pdf(const Vec2i& i) const
  {
    if (i.y < 0 || i.y >= size.y)
      return zero;

    float xPdf = Distribution1D::pdf(xDists.getData() + i.y*(size.x+1), size.x, i.x);
    float yPdf = Distribution1D::pdf(yDist.getData(), size.y, i.y);

    return xPdf * yPdf;
  }

  prt_inline vfloat pdf(vbool m, const Vec2vi& i) const
  {
    vfloat pdf = zero;
    m &= i.y >= 0;
    m &= i.y < size.y;

    vfloat xPdf = Distribution1D::pdf(m, xDists.getData(), i.y*(size.x+1), size.x, i.x);
    vfloat yPdf = Distribution1D::pdf(m, yDist.getData(), zero, size.y, i.y);

    set(m, pdf, xPdf * yPdf);
    return pdf;
  }

  prt_inline Vec2i getSize() const { return size; }
};

} // namespace prt
