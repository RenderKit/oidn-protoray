// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "tone_mapper.h"

namespace prt {

class AcesToneMapper : public ToneMapper
{
private:
  float exposure;

public:
  AcesToneMapper(const Props& props)
  {
    exposure = props.get("exposure", 1.0f);
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
  Vec3<T> apply(const Vec3<T>& v) const
  {
    static const Basis3<T> acesInputMat(
      Vec3<T>(0.5972782409f, 0.0760130499f, 0.0284085382f),
      Vec3<T>(0.3545713181f, 0.9083220973f, 0.1338243154f),
      Vec3<T>(0.0482176639f, 0.0156579968f, 0.8375684636f)
    );

    static const Basis3<T> acesOutputMat(
      Vec3<T>(1.6047539945f, -0.1020831870f, -0.0032670420f),
      Vec3<T>(-0.5310794927f, 1.1081322801f, -0.0727552477f),
      Vec3<T>(-0.0736720338f, -0.0060518756f, 1.0760219533f)
    );

    Vec3<T> x = acesInputMat * v;

    // Apply RRT and ODT
    const T a = 1.6773f;
    const T d = 0.9714f;
    const T b = 1.1174f;
    const T c = 0.2447f;
    x = pow(x, a) / ((pow(x, a*d)) * b + c);

    x = acesOutputMat * x;

    x = clamp(x, T(0.f), T(1.f));
    return x;
  }
};

} // namespace prt
