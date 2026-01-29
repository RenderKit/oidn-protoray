// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/array.h"
#include "sys/logging.h"
#include "sampling/distribution1d.h"
#include "math/random.h"
#include "accum_buffer.h"

namespace prt {

enum PixelFilterMode
{
  Sample, // importance sampling
  Splat   // splatting (slower and introduces pixel correlation!)
};

// Lookup table based pixel filter
class PixelFilter
{
private:
  Distribution1D distribution;

protected:
  float width;
  float radius;

public:
  PixelFilter(float width)
    : width(width), radius(width / 2.f)
  {}

  virtual ~PixelFilter() = default;

  // Must be called before using the filter!
  void init()
  {
    const int n = 2048;
    const int size = n*2 + 1;

    Array<float> lut(size);
    for (int i = 0; i < size; ++i)
      lut[i] = eval((float(i - n) / float(n)) * radius);

    distribution.init(lut.getData(), lut.getSize());

#if 0
    // Save the LUT to a file
    FILE* file = fopen("filter.csv", "wt");
    for (int i = 0; i < size; ++i)
      fprintf(file, "%f,%f\n", (float(i - n) / float(n)) * radius, lut[i]);
    fclose(file);
#endif
  }

  virtual float eval(float x) = 0;

  prt_inline Vec2vf sample(vbool m, const Vec2vf& s)
  {
    vfloat pdf;
    vfloat x = distribution.sampleContinuous(m, pdf, s.x);
    vfloat y = distribution.sampleContinuous(m, pdf, s.y);

    return (Vec2vf(x, y) - vfloat(0.5f)) * vfloat(width);
  }

  template <class AccumBufferT, class T>
  prt_inline void splat(vbool m, AccumBufferT& buffer, const Vec2vi& p, const T& c, const Vec2vf& s)
  {
    throw std::runtime_error("splatting not implemented");
  }

  prt_inline float getWidth()
  {
    return width;
  }
};

class TriangleFilter : public PixelFilter
{
public:
  TriangleFilter(float width)
    : PixelFilter(width) {}

  float eval(float x) override
  {
    return max(radius - abs(x), 0.f);
  }
};

class GaussianFilter : public PixelFilter
{
private:
  float alpha;
  float beta;

public:
  GaussianFilter(float width, float sigma, bool bias = true)
    : PixelFilter(width)
  {
    alpha = -1.f / (2.f*sqr(sigma));
    beta  = bias ? gauss(radius) : 0.f;
  }

  float eval(float x) override
  {
    return max(gauss(x) - beta, 0.f);
  }

private:
  prt_inline float gauss(float x)
  {
    return exp(alpha * sqr(x));
  }
};

class BlackmanHarrisFilter : public PixelFilter
{
public:
  BlackmanHarrisFilter(float width)
    : PixelFilter(width) {}

  float eval(float x) override
  {
    const float a0 = 0.35875f;
    const float a1 = 0.48829f;
    const float a2 = 0.14128f;
    const float a3 = 0.01168f;

    x /= radius;

    if (x < -1.f || x > 1.f)
      return 0.f;

    const float t = (x + 1.f) * float(pi);
    return a0 - a1 * cos(t) + a2 * cos(2.f * t) - a3 * cos(3.f * t);
  }
};

} // namespace prt
