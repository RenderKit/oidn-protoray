// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "math/vec2.h"
#include "math/hash.h"
#include "sobol.h"

namespace prt {

class SobolSamplerSimd;

class SobolSampler
{
private:
  unsigned int seed = 0;

public:
  typedef SobolSamplerSimd Simd;

  struct State
  {
    unsigned int index;    // sample index
    unsigned int scramble; // random number for scrambling the samples
    unsigned int lcg;      // LCG used when we run out of dimensions
  };

  void init(unsigned int dimensionCount, unsigned int sampleCount, unsigned int seed)
  {
    this->seed = seed;
  }

  prt_inline void setSample(State& state, unsigned int sampleIndex, unsigned int pixelId)
  {
    const unsigned int skip = 64; // skip the first few samples to reduce correlation artifacts
    state.index = sampleIndex + skip;

    unsigned int hash = seed;
    hash = murmurHash3Mix(hash, pixelId);
    state.scramble = murmurHash3Finalize(hash);

    hash = murmurHash3Mix(hash, sampleIndex);
    state.lcg = murmurHash3Finalize(hash);
  }

  prt_inline void resetSample(State& state, unsigned int sampleIndex, unsigned int pixelId)
  {
    setSample(state, sampleIndex, pixelId);
  }

  prt_inline float get1D(State& state, unsigned int dimension)
  {
    return get(state, dimension);
  }

  prt_inline Vec2f get2D(State& state, unsigned int dimension)
  {
    return Vec2f(get(state, dimension), get(state, dimension+1));
  }

  prt_inline Vec3f get3D(State& state, unsigned int dimension)
  {
    return Vec3f(get(state, dimension), get(state, dimension+1), get(state, dimension+2));
  }

private:
  prt_inline float get(State& state, unsigned int dimension)
  {
    if (dimension >= sobol::Matrices::num_dimensions)
      return lcgGetFloat(state.lcg);

    // Compute one component of the Sobol sequence
    unsigned int result = 0;
    unsigned int index = state.index;
    unsigned int i = dimension * sobol::Matrices::size;
    while (index)
    {
      if (index & 1)
        result ^= sobol::Matrices::matrices[i];
      index >>= 1;
      ++i;
    }

    // Get float in [0,1)
    float s = toFloatUnorm(result);

    // Apply Cranley-Patterson rotation to reduce correlation artifacts
    float shift = toFloatUnorm(hashToRandomSimple(dimension, state.scramble));
    return (s + shift) - floor(s + shift);
  }
};

} // namespace prt
