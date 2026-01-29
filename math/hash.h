// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "simd.h"
#include "vec2.h"
#include "vec3.h"

namespace prt {

template <class Uint>
prt_inline Uint murmurHash3Mix(Uint hash, Uint k)
{
  const unsigned int c1 = 0xcc9e2d51;
  const unsigned int c2 = 0x1b873593;
  const unsigned int r1 = 15;
  const unsigned int r2 = 13;
  const unsigned int m = 5;
  const unsigned int n = 0xe6546b64;

  k *= c1;
  k = shl(k, r1) | shr(k, 32 - r1);
  k *= c2;

  hash ^= k;
  hash = (shl(hash, r2) | shr(hash, 32 - r2)) * m + n;

  return hash;
}

template <class Uint>
prt_inline Uint murmurHash3Finalize(Uint hash)
{
  hash ^= shr(hash, 16);
  hash *= 0x85ebca6b;
  hash ^= shr(hash, 13);
  hash *= 0xc2b2ae35;
  hash ^= shr(hash, 16);

  return hash;
}

template <class Uint>
prt_inline Uint hashToRandomSimple(Uint value, Uint scramble)
{
  value = (value ^ 61) ^ scramble;
  value += shl(value, 3);
  value ^= shr(value, 4);
  value *= 0x27d4eb2d;
  return value;
}

// Thomas Wang's 64-bit mix function
inline uint64_t hashToRandom64(uint64_t key)
{
  key += ~(key << 32);
  key ^= (key >> 22);
  key += ~(key << 13);
  key ^= (key >> 8);
  key += (key << 3);
  key ^= (key >> 15);
  key += ~(key << 27);
  key ^= (key >> 31);
  return key;
}

template <class Uint>
prt_inline Uint lcgNext(Uint value)
{
  const unsigned int m = 1664525;
  const unsigned int n = 1013904223;

  return value * m + n;
}

template <class Uint>
prt_inline ToFloatT<Uint> lcgGetFloat(Uint& state)
{
  state = lcgNext(state);
  return toFloatUnorm(state);
}

template <class Uint>
prt_inline Vec2<ToFloatT<Uint>> lcgGetFloat2(Uint& state)
{
  return Vec2<ToFloatT<Uint>>(lcgGetFloat(state), lcgGetFloat(state));
}

template <class Uint>
prt_inline Vec3<ToFloatT<Uint>> lcgGetFloat3(Uint& state)
{
  return Vec3<ToFloatT<Uint>>(lcgGetFloat(state), lcgGetFloat(state), lcgGetFloat(state));
}

} // namespace prt
