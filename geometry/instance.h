// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "math/affine3.h"
#include "geometry.h"
#include "group.h"

namespace prt {

class Instance : public Geometry
{
public:
  Affine3f transform;
  std::shared_ptr<Group> child;

  prt_inline void postIntersect(const Ray& ray, const Hit& hit, ShadingContext& ctx, int level) const override
  {
    child->postIntersect(ray, hit, ctx, level+1);

    ctx.f.U = normalize(xfmVector(transform, ctx.f.U));
    ctx.f.V = normalize(xfmVector(transform, ctx.f.V));
    ctx.f.N = normalize(xfmNormal(transform, ctx.f.N));
    ctx.Ng  = normalize(xfmNormal(transform, ctx.Ng));
  }

  prt_inline void postIntersect(vbool m, const RaySimd& ray, const HitSimd& hit, ShadingContextSimd& ctx, int level) const override
  {
    child->postIntersect(m, ray, hit, ctx, level+1);

    set(m, ctx.f.U, normalize(xfmVector(transform, ctx.f.U)));
    set(m, ctx.f.V, normalize(xfmVector(transform, ctx.f.V)));
    set(m, ctx.f.N, normalize(xfmNormal(transform, ctx.f.N)));
    set(m, ctx.Ng,  normalize(xfmNormal(transform, ctx.Ng)));
  }

  prt_inline void postIntersect(const Ray& ray, const Hit& hit, SimpleShadingContext& ctx, int level) const override
  {
    child->postIntersect(ray, hit, ctx, level+1);
    ctx.Ng = normalize(xfmNormal(transform, ctx.Ng));
  }

  prt_inline void postIntersect(vbool m, const RaySimd& ray, const HitSimd& hit, SimpleShadingContextSimd& ctx, int level) const override
  {
    child->postIntersect(m, ray, hit, ctx, level+1);
    set(m, ctx.Ng, normalize(xfmNormal(transform, ctx.Ng)));
  }

  prt_inline int getMaterialId(const Hit& hit, int level) const override
  {
    return child->getMaterialId(hit, level+1);
  }

  prt_inline vint getMaterialId(vbool m, const HitSimd& hit, int level) const override
  {
    return child->getMaterialId(m, hit, level+1);
  }

  prt_inline Box3f getBounds() override
  {
    return xfmBox(transform, child->getBounds());
  }
};

} // namespace prt
