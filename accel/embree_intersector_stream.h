// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "core/intersector_stream.h"
#include "embree_intersector.h"

namespace prt {

template <int streamSize>
class EmbreeIntersectorSingleStream : public IntersectorStream<streamSize>, EmbreeIntersector
{
public:
  EmbreeIntersectorSingleStream(const std::shared_ptr<Group>& scene, bool filter, const Props& props, Props& stats) : EmbreeIntersector(scene, filter, props, stats) {}

  void intersect(RayStream<streamSize>& rays, HitStream<streamSize>& hits, int count, RayStats& stats, RayHint hint, IntersectorFilterSimd* filter, void* payload) override
  {
    IntersectContext context(hint, filter, payload);

    stats.rayCount += count;

    for (int i = 0; i < count; ++i)
    {
      RTCRayHit eray;
      initRay(rays, i, eray.ray, i);
      initHit(eray.hit);
      rtcIntersect1(sceneHandle, &eray, &context.intersectArgs);

      if (eray.hit.geomID != RTC_INVALID_GEOMETRY_ID)
      {
        rays.far[i] = eray.ray.tfar;
        hits.primId[i] = eray.hit.primID;
        hits.geomId[i] = eray.hit.geomID;
        hits.instId[i] = eray.hit.instID[0];
        hits.u[i] = eray.hit.u;
        hits.v[i] = eray.hit.v;
      }
    }
  }

  void occluded(RayStream<streamSize>& rays, int count, RayStats& stats, RayHint hint, IntersectorFilterSimd* filter, void* payload) override
  {
    IntersectContext context(hint, filter, payload);

    stats.rayCount += count;

    for (int i = 0; i < count; ++i)
    {
      RTCRay eray;
      initRay(rays, i, eray, i);
      rtcOccluded1(sceneHandle, &eray, &context.occludedArgs);

      if (eray.tfar < 0.f)
        rays.far[i] = 0.f;
    }
  }
};

template <int streamSize>
class EmbreeIntersectorPacketStream : public IntersectorStream<streamSize>, EmbreeIntersector
{
public:
  EmbreeIntersectorPacketStream(const std::shared_ptr<Group>& scene, bool filter, const Props& props, Props& stats) : EmbreeIntersector(scene, filter, props, stats) {}

  void intersect(RayStream<streamSize>& rays, HitStream<streamSize>& hits, int count, RayStats& stats, RayHint hint, IntersectorFilterSimd* filter, void* payload) override
  {
    IntersectContext context(hint, filter, payload);

    stats.rayCount += count;

    for (int i = 0; i < count; i += simdSize)
    {
      vbool mask = (vint(step) + i) < count;
      RaySimd ray;
      rays.getA(i, ray);

#if PRT_SIMD_SIZE == 16
      vint16 emask = select(mask, vint16(-1), zero);
      RTCRayHit16 eray;
      initRay(ray, eray.ray, i);
      initHit(eray.hit);
      rtcIntersect16((const int*)&emask, sceneHandle, &eray, &context.intersectArgs);
#else
      RTCRayHit8 eray;
      initRay(ray, eray.ray, i);
      initHit(eray.hit);
      rtcIntersect8((const int*)&mask, sceneHandle, &eray, &context.intersectArgs);
#endif

      vint geomId = load((int*)eray.hit.geomID);
      if (any(geomId != RTC_INVALID_GEOMETRY_ID))
      {
        rays.far.setA(i, load(eray.ray.tfar));
        hits.primId.setA(i, load((int*)eray.hit.primID));
        hits.geomId.setA(i, geomId);
        hits.instId.setA(i, load((int*)eray.hit.instID[0]));
        hits.u.setA(i, load(eray.hit.u));
        hits.v.setA(i, load(eray.hit.v));
      }
    }
  }

  void occluded(RayStream<streamSize>& rays, int count, RayStats& stats, RayHint hint, IntersectorFilterSimd* filter, void* payload) override
  {
    IntersectContext context(hint, filter, payload);

    stats.rayCount += count;

    for (int i = 0; i < count; i += simdSize)
    {
      vbool mask = (vint(step) + i) < count;
      RaySimd ray;
      rays.getA(i, ray);

#if PRT_SIMD_SIZE == 16
      vint16 emask = select(mask, vint16(-1), zero);
      RTCRay16 eray;
      initRay(ray, eray, i);
      rtcOccluded16((const int*)&emask, sceneHandle, &eray, &context.occludedArgs);
#else
      RTCRay8 eray;
      initRay(ray, eray, i);
      rtcOccluded8((const int*)&mask, sceneHandle, &eray, &context.occludedArgs);
#endif

      ray.far = load(eray.tfar);
      ray.far = select(ray.far < vfloat(zero), vfloat(zero), ray.far);
      rays.far.setA(i, ray.far);
    }
  }
};

} // namespace prt
