// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "distribution1d.h"

namespace prt {

Distribution1D::Distribution1D() : size(0) {}

Distribution1D::Distribution1D(const float* f, int size)
{
  init(f, size);
}

void Distribution1D::init(const float* f, int size)
{
  this->size = size;
  cdf.alloc(size+1);
  init(cdf.getData(), f, size);
}

void Distribution1D::init(float* cdf, const float* f, int size)
{
  // Initialize the cumulative distribution
  cdf[0] = 0.0f;
  for (int i = 1; i < size+1; ++i)
    cdf[i] = cdf[i-1] + f[i-1];

  // Normalize the cumulative distribution
  float normalization = (cdf[size] == 0.0f) ? 0.0f : rcp(cdf[size]);

  for (int i = 1; i < size+1; ++i)
    cdf[i] *= normalization;
  cdf[size] = 1.0f;
}

// Upper bound
int Distribution1D::lookup(const float* cdf, int size, float s)
{
  int first = 0;
  int len = size;

  while (len > 0)
  {
    int half = len >> 1;
    int mid = first + half;
    if (s < cdf[mid])
    {
      len = half;
    }
    else
    {
      first = mid + 1;
      len = len - half - 1;
    }
  };

  return max(first-1, 0);
}

vint Distribution1D::lookup(vbool m, const float* cdf, vint offset, int size, vfloat s)
{
  vint first = 0;
  vint len = size;

  while (any(m &= len > 0))
  {
    vint half = len >> 1;
    vint mid = first + half;

    vbool mLeft = m & (s < gather(m, cdf, offset + mid));
    set(mLeft, len, half);

    vbool mRight = andn(m, mLeft);
    set(mRight, first, mid + 1);
    set(mRight, len, len - half - 1);
  };

  return max(first-1, 0);
}

} // namespace prt
