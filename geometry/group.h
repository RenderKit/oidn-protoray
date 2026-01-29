// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/array.h"
#include "geometry.h"

namespace prt {

class Group : public Geometry
{
public:
  Array<std::shared_ptr<Geometry>> children;

  void postIntersect(const Ray& ray, const Hit& hit, ShadingContext& ctx, int level) const override
  {
    int geomId = hit.getInstGeomId(level);
    children[geomId]->postIntersect(ray, hit, ctx, level);
  }

  void postIntersect(vbool m, const RaySimd& ray, const HitSimd& hit, ShadingContextSimd& ctx, int level) const override
  {
    // For each unique geomId
    vint geomId = hit.getInstGeomId(level);
    while (any(m))
    {
      int pos = bitScan(toIntMask(m));
      int geomIdCur = geomId[pos];
      vbool mCur = m & (geomId == vint(geomIdCur));

      children[geomIdCur]->postIntersect(mCur, ray, hit, ctx, level);

      m = andn(m, mCur);
    };
  }

  void postIntersect(const Ray& ray, const Hit& hit, SimpleShadingContext& ctx, int level) const override
  {
    int geomId = hit.getInstGeomId(level);
    children[geomId]->postIntersect(ray, hit, ctx, level);
  }

  void postIntersect(vbool m, const RaySimd& ray, const HitSimd& hit, SimpleShadingContextSimd& ctx, int level) const override
  {
    // For each unique geomId
    vint geomId = hit.getInstGeomId(level);
    while (any(m))
    {
      int pos = bitScan(toIntMask(m));
      int geomIdCur = geomId[pos];
      vbool mCur = m & (geomId == vint(geomIdCur));

      children[geomIdCur]->postIntersect(mCur, ray, hit, ctx, level);

      m = andn(m, mCur);
    };
  }

  int getMaterialId(const Hit& hit, int level) const override
  {
    int geomId = hit.getInstGeomId(level);
    return children[geomId]->getMaterialId(hit, level);
  }

  vint getMaterialId(vbool m, const HitSimd& hit, int level) const override
  {
    vint geomId = hit.getInstGeomId(level);
    vint matId = zero;

    // For each unique geomId
    while (any(m))
    {
      int pos = bitScan(toIntMask(m));
      int geomIdCur = geomId[pos];
      vbool mCur = m & (geomId == vint(geomIdCur));

      set(mCur, matId, children[geomIdCur]->getMaterialId(mCur, hit, level));

      m = andn(m, mCur);
    };

    return matId;
  }

  Box3f getBounds() override
  {
    Box3f bounds = empty;
    for (const auto& geometry : children)
      bounds.grow(geometry->getBounds());
    return bounds;
  }
};

} // namespace prt
