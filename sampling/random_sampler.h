// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "math/vec2.h"
#include "math/hash.h"

namespace prt {

class RandomSamplerSimd;

class RandomSampler
{
private:
  unsigned int seed = 0;

public:
  typedef RandomSamplerSimd Simd;

  struct State
  {
    unsigned int lcg;
  };

  void init(unsigned int dimensionCount, unsigned int sampleCount, unsigned int seed)
  {
    this->seed = seed;
  }

  prt_inline void setSample(State& state, unsigned int sampleIndex, unsigned int pixelId)
  {
    unsigned int hash = seed;
    hash = murmurHash3Mix(hash, pixelId);
    hash = murmurHash3Mix(hash, sampleIndex);
    hash = murmurHash3Finalize(hash);

    state.lcg = hash;
  }

  prt_inline void resetSample(State& state, unsigned int sampleIndex, unsigned int pixelId)
  {
  }

  prt_inline float get1D(State& state, unsigned int dimension)
  {
    return lcgGetFloat(state.lcg);
  }

  prt_inline Vec2f get2D(State& state, unsigned int dimension)
  {
    float x = lcgGetFloat(state.lcg);
    float y = lcgGetFloat(state.lcg);

    return Vec2f(x, y);
  }

  prt_inline Vec3f get3D(State& state, unsigned int dimension)
  {
    float x = lcgGetFloat(state.lcg);
    float y = lcgGetFloat(state.lcg);
    float z = lcgGetFloat(state.lcg);

    return Vec3f(x, y, z);
  }
};

} // namespace prt
