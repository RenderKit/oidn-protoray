// Copyright 2015 Intel Corporation
// Copyright 2014 Melissa O'Neill <oneill@pcg-random.org>
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <ctime>
#include "sys/common.h"
#include "sys/tick_counter.h"
#include "hash.h"
#include "vec2.h"
#include "vec3.h"

namespace prt {

// PCG random number generator (pcg-random.org)
class Random
{
private:
  uint64_t state; // RNG state, all values are possible
  uint64_t inc;   // controls which RNG sequence (stream) is selected, must *always* be odd

public:
  static constexpr uint64_t defaultSeed = 0xcafef00dd15ea5e5ULL;
  static constexpr uint64_t defaultSeq  = 1442695040888963407ULL >> 1;

  // Seeds the RNG. Specified in two parts, state initializer
  // and a sequence selection constant (a.k.a. stream id).
  prt_inline explicit Random(uint64_t seed = defaultSeed, uint64_t seq = defaultSeq)
  {
    reset(seed, seq);
  }

  prt_inline void reset(uint64_t seed = defaultSeed, uint64_t seq = defaultSeq)
  {
    inc = (seq << 1) | 1;
    state = seed + inc;
    next();
  }

  prt_inline uint64_t getSeq() const
  {
    return inc >> 1;
  }

  prt_inline void next()
  {
    state = state * 6364136223846793005ULL + inc;
  }

  prt_inline uint32_t get1ui()
  {
    const uint64_t oldState = state;
    next();
    const uint32_t xorShifted = ((oldState >> 18) ^ oldState) >> 27;
    const uint32_t rot = oldState >> 59;
    return (xorShifted >> rot) | (xorShifted << ((-rot) & 31));
  }

  prt_inline int get1i()
  {
    return (int)get1ui();
  }

  prt_inline float get1f()
  {
    return toFloatUnorm(get1ui());
  }

  prt_inline float get1f(float a, float b)
  {
    return a + get1f() * (b-a);
  }

  prt_inline Vec2f get2f()
  {
    return Vec2f(get1f(), get1f());
  }

  prt_inline Vec3f get3f()
  {
    return Vec3f(get1f(), get1f(), get1f());
  }

  prt_inline int get1i(int a, int b)
  {
    assert(a <= b);
    //return min(a + int(getFloat() * (b-a+1)), b);

    const uint32_t bound = b-a+1;

    // To avoid bias, we need to make the range of the RNG a multiple of
    // bound, which we do by dropping output less than a threshold.
    // A naive scheme to calculate the threshold would be to do
    //
    //     uint32_t threshold = 0x100000000ull % bound;
    //
    // but 64-bit div/mod is slower than 32-bit div/mod (especially on
    // 32-bit platforms).  In essence, we do
    //
    //     uint32_t threshold = (0x100000000ull-bound) % bound;
    //
    // because this version will calculate the same modulus, but the LHS
    // value is less than 2^32.
    const uint32_t threshold = -bound % bound;

    // Uniformity guarantees that this loop will terminate.  In practice, it
    // should usually terminate quickly; on average (assuming all bounds are
    // equally likely), 82.25% of the time, we can expect it to require just
    // one iteration.  In the worst case, someone passes a bound of 2^31 + 1
    // (i.e., 2147483649), which invalidates almost 50% of the range.  In
    // practice, bounds are typically small and only a tiny amount of the range
    // is eliminated.
    for (; ;)
    {
      const uint32_t r = get1ui();
      if (r >= threshold)
        return (r % bound) + a;
    }
  }

  prt_inline uint64_t get1ull()
  {
    return get1ui() | ((uint64_t)get1ui() << 32);
  }

  prt_inline float getNorm1f(float mu, float sigma)
  {
    // Box-Muller transform
    const float u1 = get1f();
    const float u2 = get1f();
    const float z0 = sqrt(-2.f * log(u1)) * cos(2.f * float(pi) * u2);
    //const float z1 = sqrt(-2.f * log(u1)) * sin(2.f * float(pi) * u2);
    return z0 * sigma + mu;
  }

  prt_inline float getTruncNorm1f(float mu, float sigma, float a, float b)
  {
    float x;
    do
    {
      x = getNorm1f(mu, sigma);
    } while (x < a || x > b);
    return x;
  }

  prt_inline float getTruncNorm1f(float mu, float sigma)
  {
    return getTruncNorm1f(mu, sigma, mu - sigma*3.f, mu + sigma*3.f);
  }

  prt_inline Vec2f getTruncNorm2f(float mu, float sigma, float a, float b)
  {
    return Vec2f(getTruncNorm1f(mu, sigma, a, b), getTruncNorm1f(mu, sigma, a, b));
  }

  prt_inline Vec2f getTruncNorm2f(float mu, float sigma)
  {
    return Vec2f(getTruncNorm1f(mu, sigma), getTruncNorm1f(mu, sigma));
  }
};

inline uint64_t getRandomSeed64()
{
  uint64_t seed;

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
  // No RDSEED equivalent on ARM; fall through to time-based seed
#else
  // First, try to use RDSEED
  for (int i = 0; i < 100; ++i)
  #if defined(__INTEL_COMPILER)
    if (_rdseed64_step(&seed))
  #else
    if (_rdseed64_step((unsigned long long*)&seed))
  #endif
      return seed;
#endif

  seed = time(0);
  seed = hashToRandom64(seed);
  seed ^= TickCounter::now();
  seed = hashToRandom64(seed);
  return seed;
}

} // namespace prt
