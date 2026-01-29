// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "math/vec3.h"
#include "math/random.h"
#include "core/shading_context.h"
#include "pixel.h"
#include "color.h"

namespace prt {

enum class TextureAddress
{
  Repeat,
  Clamp,
};

class Texture
{
public:
  virtual ~Texture() {}

  virtual Vec3f get3f(const ShadingContext& ctx) const = 0;
  virtual Vec3vf get3f(vbool m, const ShadingContextSimd& ctx) const = 0;

  virtual Vec4f get4f(const ShadingContext& ctx) const
  {
    return get3f(ctx);
  }

  virtual Vec4vf get4f(vbool m, const ShadingContextSimd& ctx) const
  {
    return get3f(m, ctx);
  }

  virtual float get1f(const ShadingContext& ctx) const
  {
    return average(get3f(ctx));
  }

  virtual vfloat get1f(vbool m, const ShadingContextSimd& ctx) const
  {
    return average(get3f(m, ctx));
  }

  virtual bool hasAlpha() const { return false; }

  virtual void mutate(Random& rng) {}
};

} // namespace prt
