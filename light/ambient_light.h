// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/props.h"
#include "sampling/shape_sampler.h"
#include "image/color.h"
#include "light.h"

namespace prt {

class AmbientLight : public EnvironmentLight
{
private:
  Vec3f color;
  float sceneRadius;

public:
  AmbientLight(const Props& props, float sceneRadius)
  {
    color = props.get("Ke", Vec3f(1.0f));
    this->sceneRadius = sceneRadius;
  }

  Vec3f eval(const Vec3f& wi, float& pdf) const
  {
    // We should sample the hemisphere but for the computation of the PDF we would need the DG too (which we want to avoid)
    pdf = uniformSampleSpherePdf();
    return color;
  }

  Vec3vf eval(vbool m, const Vec3vf& wi, vfloat& pdf) const
  {
    pdf = uniformSampleSpherePdf();
    return color;
  }

  Vec3f sample(const ShadingContext& ctx, Vec3f& wi, float& pdf, float& dist, const LightSample& s) const
  {
    wi = uniformSampleSphere(pdf, s.v);
    dist = posMax;
    return color * rcp(pdf);
  }

  Vec3vf sample(vbool m, const ShadingContextSimd& ctx, Vec3vf& wi, vfloat& pdf, vfloat& dist, const LightSampleSimd& s) const
  {
    wi = uniformSampleSphere(pdf, s.v);
    dist = posMax;
    return Vec3vf(color) * rcp(pdf);
  }

  float getPower() const
  {
    return 4.f * sqr(float(pi)) * sqr(sceneRadius) * luminance(color);
  }
};

} // namespace prt
