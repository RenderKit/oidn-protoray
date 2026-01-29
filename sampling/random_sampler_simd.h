// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "math/vec2.h"
#include "math/hash.h"

namespace prt {

class RandomSampler;

class RandomSamplerSimd
{
private:
  unsigned int seed = 0;

public:
  typedef RandomSampler Scalar;

  struct State
  {
    vuint lcg;
  };

  void init(unsigned int dimensionCount, unsigned int sampleCount, unsigned int seed)
  {
    this->seed = seed;
  }

  prt_inline void setSample(vbool m, State& state, vuint sampleIndex, vuint pixelId)
  {
    vuint hash = seed;
    hash = murmurHash3Mix(hash, pixelId);
    hash = murmurHash3Mix(hash, sampleIndex);
    hash = murmurHash3Finalize(hash);

    state.lcg = hash;
  }

  prt_inline void resetSample(vbool m, State& state, vuint sampleIndex, vuint pixelId)
  {
  }

  prt_inline vfloat get1D(vbool m, State& state, int dimension)
  {
    return lcgGetFloat(state.lcg);
  }

  prt_inline Vec2vf get2D(vbool m, State& state, int dimension)
  {
    vfloat x = lcgGetFloat(state.lcg);
    vfloat y = lcgGetFloat(state.lcg);

    return Vec2vf(x, y);
  }

  prt_inline Vec3vf get3D(vbool m, State& state, int dimension)
  {
    vfloat x = lcgGetFloat(state.lcg);
    vfloat y = lcgGetFloat(state.lcg);
    vfloat z = lcgGetFloat(state.lcg);

    return Vec3vf(x, y, z);
  }
};

} // namespace prt
