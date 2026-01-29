// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/array.h"
#include "math/box3.h"
#include "math/affine3.h"
#include "core/shading_context.h"

namespace prt {

class Geometry
{
public:
  virtual ~Geometry() {}

  virtual void postIntersect(const Ray& ray, const Hit& hit, ShadingContext& ctx, int level) const = 0;
  virtual void postIntersect(vbool m, const RaySimd& ray, const HitSimd& hit, ShadingContextSimd& ctx, int level) const = 0;

  virtual void postIntersect(const Ray& ray, const Hit& hit, SimpleShadingContext& ctx, int level) const = 0;
  virtual void postIntersect(vbool m, const RaySimd& ray, const HitSimd& hit, SimpleShadingContextSimd& ctx, int level) const = 0;

  virtual int getMaterialId(const Hit& hit, int level) const = 0;
  virtual vint getMaterialId(vbool m, const HitSimd& hit, int level) const = 0;

  virtual Box3f getBounds() = 0;

  virtual std::shared_ptr<Geometry> clone(Affine3f transform = one) const
  {
    throw std::logic_error("Geometry::clone() not implemented for this geometry type");
  }
};

} // namespace prt
