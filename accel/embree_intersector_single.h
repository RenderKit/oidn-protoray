// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "core/intersector_single.h"
#include "embree_intersector.h"

namespace prt {

class EmbreeIntersectorSingle : public IntersectorSingle, EmbreeIntersector
{
public:
  EmbreeIntersectorSingle(const std::shared_ptr<Group>& scene, bool filter, const Props& props, Props& stats) : EmbreeIntersector(scene, filter, props, stats) {}

  void intersect(Ray& ray, Hit& hit, RayStats& stats, RayHint hint, IntersectorFilter* filter, void* payload) override
  {
    IntersectContext context(hint, filter, payload);

    stats.rayCount++;

    RTCRayHit eray;
    initRay(ray, eray.ray);
    initHit(eray.hit);
    rtcIntersect1(sceneHandle, &eray, &context.intersectArgs);

    if (eray.hit.geomID != RTC_INVALID_GEOMETRY_ID)
    {
      ray.far = eray.ray.tfar;
      hit.primId = eray.hit.primID;
      hit.geomId = eray.hit.geomID;
      hit.instId = eray.hit.instID[0];
      hit.u = eray.hit.u;
      hit.v = eray.hit.v;
    }
  }

  void occluded(Ray& ray, RayStats& stats, RayHint hint, IntersectorFilter* filter, void* payload) override
  {
    IntersectContext context(hint, filter, payload);

    stats.rayCount++;

    RTCRay eray;
    initRay(ray, eray);
    rtcOccluded1(sceneHandle, &eray, &context.occludedArgs);

    if (eray.tfar < 0.f)
      ray.far = 0.f;
  }
};

} // namespace prt
