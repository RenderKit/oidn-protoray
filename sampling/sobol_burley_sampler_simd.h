// Copyright 2025 Intel Corporation
// Copyright 2011-2022 Blender Foundation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "math/hash.h"
#include "sobol_burley.h"

namespace prt {

template <bool blueNoise = false>
class SobolBurleySamplerSimd
{
private:
  uint sampleCount = 0;
  uint seed = 0;
  uint shuffledIndexMask = 0xffffffff;

public:
  struct State
  {
    vuint index;    // sample index
    vuint scramble; // random number for scrambling the samples
  };

  void init(uint dimensionCount, uint sampleCount, uint seed)
  {
    if (blueNoise && sampleCount == 0)
      throw std::logic_error("blue noise sampling requires predetermined sample count");

    this->sampleCount = sampleCount;
    this->seed = seed;
  }

  prt_inline void setSample(vbool m, State& state, vuint sampleIndex, vuint pixelId)
  {
    if (blueNoise)
    {
      /* The blue-noise samplers use a single sequence for all pixels, but offset the index within
       * the sequence for each pixel. We use a hierarchically shuffled 2D morton curve to determine
       * each pixel's offset along the sequence.
       *
       * Based on:
       * https://psychopath.io/post/2022_07_24_owen_scrambling_based_dithered_blue_noise_sampling.
       */
      const vuint x = pixelId & 0xffff;
      const vuint y = pixelId >> 16;

      const vuint sampleOffset = owenBase4(morton2D(x, y), vuint(seed)) * sampleCount;
      state.index = sampleIndex + sampleOffset;
      state.scramble = 0;
    }
    else
    {
      state.index = sampleIndex;

      vuint hash = seed;
      hash = murmurHash3Mix(hash, pixelId);
      state.scramble = murmurHash3Finalize(hash);
    }
  }

  prt_inline void resetSample(vbool m, State& state, vuint sampleIndex, vuint pixelId)
  {
    setSample(m, state, sampleIndex, pixelId);
  }

  prt_inline vfloat get1D(vbool m, State& state, uint dimension)
  {
    return sobolBurleySample1D(state.index, dimension, state.scramble,
                               select(m, shuffledIndexMask, vuint(zero)));
  }

  prt_inline Vec2vf get2D(vbool m, State& state, uint dimension)
  {
    return sobolBurleySample2D(state.index, dimension, state.scramble,
                               select(m, shuffledIndexMask, vuint(zero)));
  }

  prt_inline Vec3vf get3D(vbool m, State& state, uint dimension)
  {
    return sobolBurleySample3D(state.index, dimension, state.scramble,
                               select(m, shuffledIndexMask, vuint(zero)));
  }
};

} // namespace prt
