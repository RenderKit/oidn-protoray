// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "tone_mapper.h"

namespace prt {

// [Hable, 2010, "Uncharted 2: HDR Lighting]
class FilmicToneMapper : public ToneMapper
{
private:
  float exposure;

public:
  FilmicToneMapper(const Props& props)
  {
    exposure = props.get("exposure", 1.0f);

    // Exposure adjustment to match 18% middle gray
    const float exposureBias = 1.758141f;
    exposure *= exposureBias;
  }

  float getExposure() const
  {
    return exposure;
  }

  Vec3f get(const Vec3f& c) const
  {
    return apply(c * exposure);
  }

  Vec3vf get(const Vec3vf& c) const
  {
    return apply(c * vfloat(exposure));
  }

private:
  template <class T>
  T eval(T x) const
  {
    const T A = 0.22f; // shoulder strength
    const T B = 0.30f; // linear strength
    const T C = 0.10f; // linear angle
    const T D = 0.20f; // toe strength
    const T E = 0.01f; // toe numerator
    const T F = 0.30f; // toe denominator

    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
  }

  template <class T>
  Vec3<T> apply(const Vec3<T>& c) const
  {
    const T W = 11.2f; // linear white point value

    const T whiteScale = rcp(eval(W));
    return Vec3<T>(eval(c.x), eval(c.y), eval(c.z)) * whiteScale;
  }
};

} // namespace prt
