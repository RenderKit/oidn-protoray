// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/props.h"
#include "image/texture.h"
#include "math/noise.h"
#include "bsdf/beckmann_distribution.h"

namespace prt {

// Procedural metallic flakes
class Flakes : public Texture
{
public:
  float scale;
  float density;
  float spread;
  float jitter;

#ifdef PRT_MUTATION_SUPPORT
  // Original values
  float scaleOrg;
  float densityOrg;
  float spreadOrg;
  float jitterOrg;
#endif

public:
  Flakes(float scale = 100.f, float density = 0.f, float spread = 0.3f, float jitter = 0.75f)
    : scale(scale), density(density), spread(spread), jitter(jitter)
  {
#ifdef PRT_MUTATION_SUPPORT
    scaleOrg = scale;
    densityOrg = density;
    spreadOrg = spread;
    jitterOrg = jitter;
#endif
  }

  template <class Float, class Int>
  Vec3<Float> eval(const Vec3<Float>& point, Int& mask) const
  {
    using UInt = ToUIntT<Int>;

    Vec3<Float> N(0.f, 0.f, 1.f);
    mask = 0;

    if (density <= 0.f)
      return N;

    if (density >= 1.f && spread <= 0.f)
    {
      mask = 1;
      return N;
    }

    const BeckmannDistribution<Float> flakeDistribution(roughnessToAlpha(spread));
    Vec3<Float> p = point * Float(scale);

    // Simple Worley cellular noise with jittered feature points
    // [Apodaca and Gritz, 1999, "Advanced RenderMan: Creating CGI for Motion Pictures", pp. 255-261]
    const Vec3<Float> thisCell = floor(p) + Float(0.5f);
    Float f1 = 1000.f;
    UInt cellRnd = 0;

    for (int i = -1; i <= 1; i++)
    {
      for (int j = -1; j <= 1; j++)
      {
        for (int k = -1; k <= 1; k++)
        {
          const Vec3<Float> testCell = thisCell + Vec3<Float>(i, j, k);

          // Generate a random ID for the test cell
          UInt rnd = cellNoise1i(testCell);

          // Generate the position of the feature point in the test cell
          const Vec3<Float> pos = testCell + Float(jitter) * (lcgGetFloat3(rnd) - Float(0.5f));

          // Distance test
          const Vec3<Float> offset = pos - p;
          const Float dist = dot(offset, offset); // actually dist^2
          const auto inside = dist < f1;
          if (any(inside))
          {
            set(inside, f1, dist);
            set(inside, cellRnd, rnd);
          }
        }
      }
    }

    // Test whether the flake exists
    const auto exists = lcgGetFloat(cellRnd) <= density;
    if (any(exists))
    {
      // Generate a random normal for the flake
      Float pdf;
      set(exists, N, flakeDistribution.sample(pdf, lcgGetFloat2(cellRnd)));
      set(exists, mask, Int(1));
    }

    return normalize(N);
  }

  Vec3f get3f(const ShadingContext& ctx) const override
  {
    int mask;
    return eval(ctx.p, mask);
  }

  Vec3vf get3f(vbool m, const ShadingContextSimd& ctx) const override
  {
    vint mask;
    return eval(ctx.p, mask);
  }

  void mutate(Random& rng) override
  {
#ifdef PRT_MUTATION_SUPPORT
    if (rng.get1f() < 0.3f)
    {
      scale = rng.get1f(scaleOrg * 0.5f, scaleOrg * 2.f);
      if (densityOrg > 0.f && densityOrg < 1.f)
        density = rng.get1f(densityOrg * 0.9f, 1.f);
      spread = rng.get1f(0.f, max(spreadOrg, 0.5f));
      jitter = rng.get1f(min(jitterOrg, 0.7f), 1.f);
    }
    else
    {
      // Disable flakes
      density = 1.f;
      spread = 0.f;
    }
#endif
  }
};

} // namespace prt
