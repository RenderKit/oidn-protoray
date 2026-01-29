// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "vec3.h"
#include "hash.h"
#include "interpolation.h"

namespace prt {

// Hash function used in the noise functions
// MurmurHash3
template <class T, int N, class Vec>
prt_inline T noiseHash(const Vec& v, T seed)
{
  T hash = seed;
  hash = murmurHash3Mix(hash, v.x);
  hash = murmurHash3Mix(hash, v.y);
  hash = murmurHash3Mix(hash, v.z);
  hash ^= 3*4;
  hash = murmurHash3Finalize(hash);
  return hash;
}

template <class T>
prt_inline T cellNoise1f(const Vec3<T>& x)
{
  typedef ToUIntT<T> U;
  Vec3<U> key = asUInt(floor(x));
  return toFloatUnorm(noiseHash<U,3>(key, 0x537e6612));
}

template <class T>
prt_inline ToUIntT<T> cellNoise1i(const Vec3<T>& x)
{
  typedef ToUIntT<T> U;
  Vec3<U> key = asUInt(floor(x));
  return noiseHash<U,3>(key, 0x537e6612);
}

template <class T>
prt_inline Vec3<T> cellNoise3f(const Vec3<T>& x)
{
  typedef ToUIntT<T> U;
  Vec3<U> key = asUInt(floor(x));
  return Vec3<T>(toFloatUnorm(noiseHash<U,3>(key, 0xf7acd0ce)),
                 toFloatUnorm(noiseHash<U,3>(key, 0x6e2bf582)),
                 toFloatUnorm(noiseHash<U,3>(key, 0xc6ae6d0d)));
}

template <class T>
prt_inline T smoothNoise1f(const Vec3<T>& x)
{
  Vec3<T> p = floor(x);
  Vec3<T> f = x - p;

  T vz[4];

  for (int z = -1; z <= 2; ++z)
  {
    T vy[4];

    for (int y = -1; y <= 2; ++y)
      vy[y+1] = catmullRom(cellNoise1f(p+Vec3<T>(-1.f, y, z)),
                           cellNoise1f(p+Vec3<T>(0.f, y, z)),
                           cellNoise1f(p+Vec3<T>(1.f, y, z)),
                           cellNoise1f(p+Vec3<T>(2.f, y, z)), f.x);

    vz[z+1] = catmullRom(vy[0], vy[1], vy[2], vy[3], f.y);
  }

  return catmullRom(vz[0], vz[1], vz[2], vz[3], f.z);
}

template <class T>
prt_inline Vec3<T> smoothNoise3f(const Vec3<T>& x)
{
  Vec3<T> p = floor(x);
  Vec3<T> f = x - p;

  Vec3<T> vz[4];

  for (int z = -1; z <= 2; ++z)
  {
    Vec3<T> vy[4];

    for (int y = -1; y <= 2; ++y)
      vy[y+1] = catmullRom(cellNoise3f(p+Vec3<T>(-1.f, y, z)),
                           cellNoise3f(p+Vec3<T>(0.f, y, z)),
                           cellNoise3f(p+Vec3<T>(1.f, y, z)),
                           cellNoise3f(p+Vec3<T>(2.f, y, z)), f.x);

    vz[z+1] = catmullRom(vy[0], vy[1], vy[2], vy[3], f.y);
  }

  return catmullRom(vz[0], vz[1], vz[2], vz[3], f.z);
}

template <class T>
prt_inline T fbm(const Vec3<T>& x, int octaves = 10, float lacunarity = 2.0f, float gain = 0.5f)
{
  float amp = gain;
  Vec3<T> p = x;
  T sum = zero;

  for (int i = 0; i < octaves; ++i)
  {
    sum += amp * smoothNoise1f(p);
    amp *= gain;
    p *= T(lacunarity);
  }

  return sum;
}

template <class T>
prt_inline Vec3<T> fbm3(const Vec3<T>& x, int octaves = 10, float lacunarity = 2.0f, float gain = 0.5f)
{
  float amp = gain;
  Vec3<T> p = x;
  Vec3<T> sum = zero;

  for (int i = 0; i < octaves; ++i)
  {
    sum += T(amp) * smoothNoise3f(p);
    amp *= gain;
    p *= T(lacunarity);
  }

  return sum;
}

} // namespace prt
