// Copyright 2025 Intel Corporation
// Copyright 2011-2022 Blender Foundation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "math/math.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"

namespace prt {

/*
 * The direction vectors for the first four dimensions of the Sobol
 * sequence, stored with reversed-order bits.
 *
 * This is used in the Sobol-Burley sampler implementation.  We don't
 * need more than four dimensions because we achieve higher dimensions
 * with padding.  They're stored with reversed bits because we need
 * them reversed for the fast hash-based Owen scrambling anyway, and
 * this avoids doing that at run time.
 */
extern unsigned int sobolBurleyTable[4][32];

/* ***** Hash Prospector Hash Functions *****
 *
 * These are based on the high-quality 32-bit hash/mixing functions from
 * https://github.com/skeeto/hash-prospector
 */
template <typename UInt>
prt_inline UInt hashHP(UInt i)
{
  // The actual mixing function from Hash Prospector.
  i ^= i >> 16;
  i *= 0x21f0aaad;
  i ^= i >> 15;
  i *= 0xd35a2d97;
  i ^= i >> 15;

  // The xor is just to make input zero not map to output zero.
  // The number is randomly selected and isn't special.
  return i ^ 0xe6fe3beb;
}

/*
 * Performs base-2 Owen scrambling on a reversed-bit unsigned integer.
 *
 * This is equivalent to the Laine-Karras permutation, but much higher
 * quality.  See https://psychopath.io/post/2021_01_30_building_a_better_lk_hash
 */
template <typename UInt>
prt_inline UInt reversedBitOwen(UInt n, const UInt seed)
{
  n ^= n * 0x3d20adea;
  n += seed;
  n *= (seed >> 16) | 1;
  n ^= n * 0x05526c56;
  n ^= n * 0x53a22864;

  return n;
}

/*
 * Performs base-4 Owen scrambling on a reversed-bit unsigned integer.
 *
 * See https://psychopath.io/post/2022_08_14_a_fast_hash_for_base_4_owen_scrambling
 */
template <typename UInt>
prt_inline UInt reversedBitOwenBase4(UInt n, const UInt seed)
{
  n ^= n * 0x3d20adea;
  n ^= (n >> 1) & (n << 1) & 0x55555555;
  n += seed;
  n *= (seed >> 16) | 1;
  n ^= (n >> 1) & (n << 1) & 0x55555555;
  n ^= n * 0x05526c56;
  n ^= n * 0x53a22864;

  return n;
}

/*
 * Performs base-4 Owen scrambling on an unsigned integer.
 */
template <typename UInt>
prt_inline UInt owenBase4(const UInt i, const UInt seed)
{
  return reverseBits(reversedBitOwenBase4(reverseBits(i), seed));
}

/*
 * Computes a single dimension of a sample from an Owen-scrambled
 * Sobol sequence.  This is used in the main sampling functions,
 * sobolBurleySample#D(), below.
 *
 * - revBitIndex:  the sample index, with reversed order bits.
 * - dimension:    the sample dimension.
 * - scrambleSeed: the Owen scrambling seed.
 *
 * Note that the seed must be well randomized before being
 * passed to this function.
 */
prt_inline float sobolBurley(uint revBitIndex,
                              const uint dimension,
                              const uint scrambleSeed)
{
  uint result = 0;

  if (dimension == 0)
  {
    /* Fast-path for dimension 0, which is just Van der corput.
     * This makes a notable difference in performance since we reuse
     * dimensions for padding, and dimension 0 is reused the most. */
    result = reverseBits(revBitIndex);
  }
  else if (dimension == 1)
  {
    /* Ahmed, "An Implementation Algorithm of 2D Sobol Sequence Fast, Elegant, and
     * Compact", EGSR 2024, https://doi.org/10.2312/sr.20241147 */
    uint v = revBitIndex;
    v ^= v << 16;
    v ^= (v & 0x00FF00FF) << 8;
    v ^= (v & 0x0F0F0F0F) << 4;
    v ^= (v & 0x33333333) << 2;
    v ^= (v & 0x55555555) << 1;
    result = reverseBits(v);
  }
  else
  {
    uint i = 0;
    while (revBitIndex != 0)
    {
      const uint j = countLeadingZeros(revBitIndex);
      result ^= sobolBurleyTable[dimension][i + j];
      i += j + 1;

      /* We can't do "<<= j + 1" because that can overflow the shift
       * operator, which doesn't do what we need on at least x86. */
      revBitIndex <<= j;
      revBitIndex <<= 1;
    }
  }

  /* Apply Owen scrambling. */
  result = reverseBits(reversedBitOwen(result, scrambleSeed));
  return toFloatUnormExcl(result);
}

prt_inline vfloat sobolBurley(vuint revBitIndex,
                               const uint dimension,
                               const vuint scrambleSeed)
{
  vuint result = 0;

  if (dimension == 0)
  {
    /* Fast-path for dimension 0, which is just Van der corput.
     * This makes a notable difference in performance since we reuse
     * dimensions for padding, and dimension 0 is reused the most. */
    result = reverseBits(revBitIndex);
  }
  else if (dimension == 1)
  {
    /* Ahmed, "An Implementation Algorithm of 2D Sobol Sequence Fast, Elegant, and
     * Compact", EGSR 2024, https://doi.org/10.2312/sr.20241147 */
    vuint v = revBitIndex;
    v ^= v << 16;
    v ^= (v & 0x00FF00FF) << 8;
    v ^= (v & 0x0F0F0F0F) << 4;
    v ^= (v & 0x33333333) << 2;
    v ^= (v & 0x55555555) << 1;
    result = reverseBits(v);
  }
  else
  {
    uint i = 0;
    while (any(revBitIndex != 0))
    {
      const vbool mask = (revBitIndex & 0x80000000) != 0;
      set(mask, result, result ^ sobolBurleyTable[dimension][i]);
      revBitIndex <<= 1;
      i++;
    }
  }

  /* Apply Owen scrambling. */
  result = reverseBits(reversedBitOwen(result, scrambleSeed));
  return toFloatUnormExcl(result);
}

/*
 * NOTE: the functions below intentionally produce samples that are
 * uncorrelated between functions.  For example, a 1D sample and 2D
 * sample produced with the same index, dimension, and seed are
 * uncorrelated with each other.  This allows more care-free usage
 * of the functions together, without having to worry about
 * e.g. 1D and 2D samples being accidentally correlated with each
 * other.
 */

/*
 * Computes a 1D Owen-scrambled and shuffled Sobol sample.
 *
 * `index` is the index of the sample in the sequence.
 *
 * `dimension` is which dimensions of the sample you want to fetch.  Note
 * that different 1D dimensions are uncorrelated.  For samples with > 1D
 * stratification, use the multi-dimensional sampling methods below.
 *
 * `seed`: different seeds produce statistically independent,
 * uncorrelated sequences.
 *
 * `shuffledIndexMask` limits the sample sequence length, improving
 * performance. It must be a string of binary 1 bits followed by a
 * string of binary 0 bits (e.g. 0xffff0000) for the sampler to operate
 * correctly. In general, `reverseBits(shuffledIndexMask)`
 * should be >= the maximum number of samples expected to be taken. A safe
 * default (but least performant) is 0xffffffff, for maximum sequence
 * length.
 */
template <typename UInt>
prt_inline ToFloatT<UInt> sobolBurleySample1D(UInt index,
                                               const uint dimension,
                                               UInt seed,
                                               const UInt shuffledIndexMask)
{
  /* Include the dimension in the seed, so we get decorrelated
   * sequences for different dimensions via shuffling. */
  seed ^= hashHP(dimension);

  /* Shuffle and mask.  The masking is just for better
   * performance at low sample counts. */
  index = reversedBitOwen(reverseBits(index), seed ^ 0xbff95bfe);
  index &= shuffledIndexMask;

  return sobolBurley(index, 0, seed ^ 0x635c77bd);
}

/*
 * Computes a 2D Owen-scrambled and shuffled Sobol sample.
 *
 * `dimensionSet` is which two dimensions of the sample you want to
 * fetch.  For example, 0 is the first two, 1 is the second two, etc.
 * The dimensions within a single set are stratified, but different sets
 * are uncorrelated.
 *
 * See sobolBurleySample1D for further usage details.
 */
template <typename UInt>
prt_inline Vec2<ToFloatT<UInt>> sobolBurleySample2D(UInt index,
                                                     const uint dimensionSet,
                                                     UInt seed,
                                                     const UInt shuffledIndexMask)
{
  /* Include the dimension set in the seed, so we get decorrelated
   * sequences for different dimension sets via shuffling. */
  seed ^= hashHP(dimensionSet);

  /* Shuffle and mask.  The masking is just for better
   * performance at low sample counts. */
  index = reversedBitOwen(reverseBits(index), seed ^ 0xf8ade99a);
  index &= shuffledIndexMask;

  return {sobolBurley(index, 0, seed ^ 0xe0aaaf76),
          sobolBurley(index, 1, seed ^ 0x94964d4e)};
}

/*
 * Computes a 3D Owen-scrambled and shuffled Sobol sample.
 *
 * `dimensionSet` is which three dimensions of the sample you want to
 * fetch.  For example, 0 is the first three, 1 is the second three, etc.
 * The dimensions within a single set are stratified, but different sets
 * are uncorrelated.
 *
 * See sobolBurleySample1D for further usage details.
 */
template <typename UInt>
prt_inline Vec3<ToFloatT<UInt>> sobolBurleySample3D(UInt index,
                                                     const uint dimensionSet,
                                                     UInt seed,
                                                     const UInt shuffledIndexMask)
{
  /* Include the dimension set in the seed, so we get decorrelated
   * sequences for different dimension sets via shuffling. */
  seed ^= hashHP(dimensionSet);

  /* Shuffle and mask.  The masking is just for better
   * performance at low sample counts. */
  index = reversedBitOwen(reverseBits(index), seed ^ 0xcaa726ac);
  index &= shuffledIndexMask;

  return {sobolBurley(index, 0, seed ^ 0x9e78e391),
          sobolBurley(index, 1, seed ^ 0x67c33241),
          sobolBurley(index, 2, seed ^ 0x78c395c5)};
}

/*
 * Computes a 4D Owen-scrambled and shuffled Sobol sample.
 *
 * `dimensionSet` is which four dimensions of the sample you want to
 * fetch.  For example, 0 is the first four, 1 is the second four, etc.
 * The dimensions within a single set are stratified, but different sets
 * are uncorrelated.
 *
 * See sobolBurleySample1D for further usage details.
 */
template <typename UInt>
prt_inline Vec4<ToFloatT<UInt>> sobolBurleySample4D(UInt index,
                                                     const uint dimensionSet,
                                                     UInt seed,
                                                     const UInt shuffledIndexMask)
{
  /* Include the dimension set in the seed, so we get decorrelated
   * sequences for different dimension sets via shuffling. */
  seed ^= hashHP(dimensionSet);

  /* Shuffle and mask.  The masking is just for better
   * performance at low sample counts. */
  index = reversedBitOwen(reverseBits(index), seed ^ 0xc2c1a055);
  index &= shuffledIndexMask;

  return {sobolBurley(index, 0, seed ^ 0x39468210),
          sobolBurley(index, 1, seed ^ 0xe9d8a845),
          sobolBurley(index, 2, seed ^ 0x5f32b482),
          sobolBurley(index, 3, seed ^ 0x1524cc56)};
}

} // namespace prt