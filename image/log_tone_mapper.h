// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "tone_mapper.h"

namespace prt {

class LogToneMapper : public ToneMapper
{
private:
  float exposure;

public:
  LogToneMapper(const Props& props)
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
  Vec3<T> apply(const Vec3<T>& c) const
  {
    const T scale = 1.f / 16.f;
    return Vec3<T>(log(c.x+1.f), log(c.y+1.f), log(c.z+1.f)) * scale;
  }
};

} // namespace prt
