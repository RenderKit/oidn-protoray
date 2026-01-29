// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/memory.h"
#include "sys/array.h"
#include "core/shading_context.h"
#include "material/material.h"

namespace prt {

struct LightSample
{
  Vec2f v; // direction/position
  float c; // component

  prt_inline LightSample() {}
  prt_inline LightSample(const Vec2f& v, float c) : v(v), c(c) {}
  prt_inline explicit LightSample(const Vec3f& s) : v(s.xy()), c(s.z) {}
};

struct LightSampleSimd
{
  Vec2vf v; // direction/position
  vfloat c; // component

  prt_inline LightSampleSimd() {}
  prt_inline LightSampleSimd(const Vec2vf& v, vfloat c) : v(v), c(c) {}
  prt_inline explicit LightSampleSimd(const Vec3vf& s) : v(s.xy()), c(s.z) {}
};

class Light
{
public:
  virtual ~Light() {}

  // Result is multiplied by 1/pdf
  // If the result is zero, the outputs are undefined
  virtual Vec3f sample(const ShadingContext& ctx, Vec3f& wi, float& pdf, float& dist, const LightSample& s) const { return zero; }
  virtual Vec3vf sample(vbool m, const ShadingContextSimd& ctx, Vec3vf& wi, vfloat& pdf, vfloat& dist, const LightSampleSimd& s) const { return zero; }

  virtual float getPower() const { return 1.f; }
};

class AreaLight : public Light
{
public:
  virtual Vec3f eval(const Vec3f& wi, float& pdf, float dist, const Hit& hit) const { pdf = zero; return zero; }
  virtual Vec3vf eval(vbool m, const Vec3vf& wi, vfloat& pdf, vfloat dist, const HitSimd& hit) const { pdf = zero; return zero; }

  virtual void update(const Array<std::shared_ptr<Material>>& materials) {}
};

class EnvironmentLight : public Light
{
public:
  virtual Vec3f eval(const Vec3f& wi, float& pdf) const { pdf = zero; return zero; }
  virtual Vec3vf eval(vbool m, const Vec3vf& wi, vfloat& pdf) const { pdf = zero; return zero; }
};

} // namespace prt
