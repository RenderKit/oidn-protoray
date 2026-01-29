// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/props.h"
#include "sys/logging.h"
#include "sampling/shape_sampler.h"
#include "image/color.h"
#include "light.h"

namespace prt {

class DistantLight : public EnvironmentLight
{
private:
  Vec3f color;
  Vec3f dir;
  float angle;
  Basis3f frame;
  float sceneRadius;

public:
  DistantLight(const Props& props, float sceneRadius)
  {
    color = props.get("Ke", Vec3f(1.0f));
    dir = normalize(props.get("direction", Vec3f(0.3f, 1.0f, 0.1f)));
    angle = props.get("angle", 9.35e-3f) * 0.5f; // convert from angular diameter
    frame = makeFrame(dir);
    this->sceneRadius = sceneRadius;

    //Log() << "Sun color: " << (color / reduceMax(color));
    //Log() << "Sun power: " << (reduceMax(color) / uniformSampleConePdf(1.f, angle));
  }

  Vec3f eval(const Vec3f& wi, float& pdf) const
  {
    if (dot(wi, dir) < cos(angle))
    {
      pdf = zero;
      return zero;
    }

    pdf = uniformSampleConePdf(dot(wi, dir), angle);
    return color;
  }

  Vec3vf eval(vbool m, const Vec3vf& wi, vfloat& pdf) const
  {
    Vec3vf value = zero;
    pdf = zero;
    m &= (dot(wi, Vec3vf(dir)) >= cos(vfloat(angle)));
    if (none(m)) return value;
    set(m, pdf, uniformSampleConePdf(dot(wi, Vec3vf(dir)), vfloat(angle)));
    set(m, value, Vec3vf(color));
    return value;
  }

  Vec3f sample(const ShadingContext& ctx, Vec3f& wi, float& pdf, float& dist, const LightSample& s) const
  {
    wi = frame * uniformSampleCone(pdf, s.v, angle);
    dist = posMax;
    return color * rcp(pdf);
  }

  Vec3vf sample(vbool m, const ShadingContextSimd& ctx, Vec3vf& wi, vfloat& pdf, vfloat& dist, const LightSampleSimd& s) const
  {
    wi = Basis3vf(frame) * uniformSampleCone(pdf, s.v, vfloat(angle));
    dist = posMax;
    return Vec3vf(color) * rcp(pdf);
  }

  float getPower() const
  {
    return 4.f * sqr(float(pi)) * sqr(sin(0.5f*angle)) * sqr(sceneRadius) * luminance(color);
  }
};

} // namespace prt
