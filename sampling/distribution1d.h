// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/raw_array.h"
#include "math/simd.h"

namespace prt {

// Discrete 1D distribution
class Distribution1D
{
private:
  RawArray<float> cdf;
  int size;

public:
  Distribution1D();
  Distribution1D(const float* f, int size);

  void init(const float* f, int size);

  prt_inline int sample(float& pdf, float s) const
  {
    return sample(cdf.getData(), size, pdf, s);
  }

  prt_inline vint sample(vbool m, vfloat& pdf, vfloat s) const
  {
    return sample(m, cdf.getData(), zero, size, pdf, s);
  }

  prt_inline int sampleReuse(float& pdf, float& s) const
  {
    return sampleReuse(cdf.getData(), size, pdf, s);
  }

  prt_inline vint sampleReuse(vbool m, vfloat& pdf, vfloat& s) const
  {
    return sampleReuse(m, cdf.getData(), zero, size, pdf, s);
  }

  prt_inline float sampleContinuous(float& pdf, float s) const
  {
    return sampleContinuous(cdf.getData(), size, pdf, s);
  }

  prt_inline vfloat sampleContinuous(vbool m, vfloat& pdf, vfloat s) const
  {
    return sampleContinuous(m, cdf.getData(), zero, size, pdf, s);
  }

  prt_inline float pdf(int i) const
  {
    return pdf(cdf.getData(), size, i);
  }

  prt_inline vfloat pdf(vbool m, vint i) const
  {
    return pdf(m, cdf.getData(), zero, size, i);
  }

  prt_inline int getSize() const { return size; }

  // Static functions
  // ----------------

  // Initialize the cumulative distribution (size+1 values)
  static void init(float* cdf, const float* f, int size);

  static prt_inline int sample(const float* cdf, int size, float& pdf, float s)
  {
    int i = lookup(cdf, size, s);
    pdf = cdf[i+1] - cdf[i];
    return i;
  }

  static prt_inline vint sample(vbool m, const float* cdf, vint offset, int size, vfloat& pdf, vfloat s)
  {
    vint i = lookup(m, cdf, offset, size, s);
    pdf = gather(m, cdf, offset+i+1) - gather(m, cdf, offset+i);
    return i;
  }

  static prt_inline int sampleReuse(const float* cdf, int size, float& pdf, float& s)
  {
    int i = lookup(cdf, size, s);
    pdf = cdf[i+1] - cdf[i];
    s = (s - cdf[i]) * rcp(pdf);
    return i;
  }

  static prt_inline vint sampleReuse(vbool m, const float* cdf, vint offset, int size, vfloat& pdf, vfloat& s)
  {
    vint i = lookup(m, cdf, offset, size, s);
    vfloat cdf0 = gather(m, cdf, offset+i);
    vfloat cdf1 = gather(m, cdf, offset+i+1);
    pdf = cdf1 - cdf0;
    s = (s - cdf0) * rcp(pdf);
    return i;
  }

  static prt_inline float sampleContinuous(const float* cdf, int size, float& pdf, float s)
  {
    int i = sampleReuse(cdf, size, pdf, s);
    return (float(i) + s) * rcp(float(size));
  }

  static prt_inline vfloat sampleContinuous(vbool m, const float* cdf, vint offset, int size, vfloat& pdf, vfloat s)
  {
    vint i = sampleReuse(m, cdf, offset, size, pdf, s);
    return (toFloat(i) + s) * rcp(float(size));
  }

  static prt_inline float pdf(const float* cdf, int size, int i)
  {
    if (i < 0 || i >= size)
      return zero;

    return cdf[i+1] - cdf[i];
  }

  static prt_inline vfloat pdf(vbool m, const float* cdf, vint offset, int size, vint i)
  {
    vfloat pdf = zero;
    m &= i >= vint(0);
    m &= i < size;
    set(m, pdf, gather(m, cdf, offset+i+1) - gather(m, cdf, offset+i));
    return pdf;
  }

private:
  static int lookup(const float* cdf, int size, float s);
  static vint lookup(vbool m, const float* cdf, vint offset, int size, vfloat s);
};

} // namespace prt
