// Copyright 2025 Intel Corporation
// Copyright 2011-2022 Blender Foundation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "math/hash.h"
#include "sobol_burley.h"

namespace prt {

class SobolBurleySamplerSimd;

template <bool blueNoise = false>
class SobolBurleySampler
{
private:
  uint sampleCount = 0;
  uint seed = 0;
  uint shuffledIndexMask = 0xffffffff;

public:
  typedef SobolBurleySamplerSimd Simd;

  struct State
  {
    uint index;    // sample index
    uint scramble; // random number for scrambling the samples
  };

  void init(uint dimensionCount, uint sampleCount, uint seed)
  {
    if (blueNoise && sampleCount == 0)
      throw std::logic_error("blue noise sampling requires predetermined sample count");

    this->sampleCount = sampleCount;
    this->seed = seed;
  }

  prt_inline void setSample(State& state, uint sampleIndex, uint pixelId)
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
      const uint x = pixelId & 0xffff;
      const uint y = pixelId >> 16;

      const uint sampleOffset = owenBase4(morton2D(x, y), seed) * sampleCount;
      state.index = sampleIndex + sampleOffset;
      state.scramble = 0;
    }
    else
    {
      state.index = sampleIndex;

      uint hash = seed;
      hash = murmurHash3Mix(hash, pixelId);
      state.scramble = murmurHash3Finalize(hash);
    }
  }

  prt_inline void resetSample(State& state, uint sampleIndex, uint pixelId)
  {
    setSample(state, sampleIndex, pixelId);
  }

  prt_inline float get1D(State& state, uint dimension)
  {
    return sobolBurleySample1D(state.index, dimension, state.scramble, shuffledIndexMask);
  }

  prt_inline Vec2f get2D(State& state, uint dimension)
  {
    return sobolBurleySample2D(state.index, dimension, state.scramble, shuffledIndexMask);
  }

  prt_inline Vec3f get3D(State& state, uint dimension)
  {
    return sobolBurleySample3D(state.index, dimension, state.scramble, shuffledIndexMask);
  }
};

} // namespace prt
