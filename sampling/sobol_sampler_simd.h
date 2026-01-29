// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "math/vec2.h"
#include "math/hash.h"
#include "sobol.h"

namespace prt {

class SobolSampler;

class SobolSamplerSimd
{
private:
  unsigned int seed = 0;

public:
  typedef SobolSampler Scalar;

  struct State
  {
    vint index;     // sample index
    vuint scramble; // random number for scrambling the samples
    vuint lcg;      // LCG used when we run out of dimensions
  };

  void init(unsigned int dimensionCount, unsigned int sampleCount, unsigned int seed)
  {
    this->seed = seed;
  }

  prt_inline void setSample(vbool m, State& state, vuint sampleIndex, vuint pixelId)
  {
    const unsigned int skip = 64; // skip the first few samples to reduce correlation artifacts
    state.index = sampleIndex + skip;

    vuint hash = seed;
    hash = murmurHash3Mix(hash, pixelId);
    state.scramble = murmurHash3Finalize(hash);

    hash = murmurHash3Mix(hash, sampleIndex);
    state.lcg = murmurHash3Finalize(hash);
  }

  prt_inline void resetSample(vbool m, State& state, vuint sampleIndex, vuint pixelId)
  {
    setSample(m, state, sampleIndex, pixelId);
  }

  prt_inline vfloat get1D(vbool m, State& state, unsigned int dimension)
  {
    return get(state, dimension);
  }

  prt_inline Vec2vf get2D(vbool m, State& state, unsigned int dimension)
  {
    return Vec2vf(get(state, dimension), get(state, dimension+1));
  }

  prt_inline Vec3vf get3D(vbool m, State& state, unsigned int dimension)
  {
    return Vec3vf(get(state, dimension), get(state, dimension+1), get(state, dimension+2));
  }

private:
  prt_inline vfloat get(State& state, unsigned int dimension)
  {
    if (dimension >= sobol::Matrices::num_dimensions)
      return lcgGetFloat(state.lcg);

    // Compute one component of the Sobol sequence
    vuint result = zero;
    vint index = state.index;
    unsigned int i = dimension * sobol::Matrices::size;
    while (!all(index == zero))
    {
      vbool mask = (index & 1) == 1;
      set(mask, result, result ^ sobol::Matrices::matrices[i]);
      index = shr(index, 1);
      ++i;
    }

    // Get float in [0,1)
    vfloat s = toFloatUnorm(result);

    // Apply Cranley-Patterson rotation to reduce correlation artifacts
    vfloat shift = toFloatUnorm(hashToRandomSimple(vuint(dimension), state.scramble));
    return (s + shift) - floor(s + shift);
  }
};

} // namespace prt
