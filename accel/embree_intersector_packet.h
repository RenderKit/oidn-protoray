// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "core/intersector_packet.h"
#include "embree_intersector.h"

namespace prt {

class EmbreeIntersectorPacket : public IntersectorPacket, EmbreeIntersector
{
public:
  EmbreeIntersectorPacket(const std::shared_ptr<Group>& scene, bool filter, const Props& props, Props& stats) : EmbreeIntersector(scene, filter, props, stats) {}

  void intersect(vbool mask, RaySimd& ray, HitSimd& hit, RayStats& stats, RayHint hint, IntersectorFilterSimd* filter, void* payload) override
  {
    IntersectContext context(hint, filter, payload);

    stats.rayCount += bitCount(toIntMask(mask));

#if PRT_SIMD_SIZE == 16
    vint16 emask = select(mask, vint16(-1), zero);
    RTCRayHit16 eray;
    initRay(ray, eray.ray);
    initHit(eray.hit);
    rtcIntersect16((const int*)&emask, sceneHandle, &eray, &context.intersectArgs);
#else
    RTCRayHit8 eray;
    initRay(ray, eray.ray);
    initHit(eray.hit);
    rtcIntersect8((const int*)&mask, sceneHandle, &eray, &context.intersectArgs);
#endif

    vint geomId = load((int*)eray.hit.geomID);
    if (any(geomId != RTC_INVALID_GEOMETRY_ID))
    {
      ray.far = load(eray.ray.tfar);
      hit.primId = load((int*)eray.hit.primID);
      hit.geomId = geomId;
      hit.instId = load((int*)eray.hit.instID[0]);
      hit.u = load(eray.hit.u);
      hit.v = load(eray.hit.v);
    }
  }

  void occluded(vbool mask, RaySimd& ray, RayStats& stats, RayHint hint, IntersectorFilterSimd* filter, void* payload) override
  {
    IntersectContext context(hint, filter, payload);

    stats.rayCount += bitCount(toIntMask(mask));

#if PRT_SIMD_SIZE == 16
    vint16 emask = select(mask, vint16(-1), zero);
    RTCRay16 eray;
    initRay(ray, eray);
    rtcOccluded16((const int*)&emask, sceneHandle, &eray, &context.occludedArgs);
#else
    RTCRay8 eray;
    initRay(ray, eray);
    rtcOccluded8((const int*)&mask, sceneHandle, &eray, &context.occludedArgs);
#endif

    vbool hitMask = load(eray.tfar) < vfloat(zero);
    set(hitMask, ray.far, vfloat(zero));
  }
};

#if PRT_SIMD_SIZE == 16
class EmbreeIntersectorPacket8 : public IntersectorPacket, EmbreeIntersector
{
public:
  EmbreeIntersectorPacket8(const std::shared_ptr<Group>& scene, bool filter, const Props& props, Props& stats) : EmbreeIntersector(scene, filter, props, stats) {}

  void intersect(vbool mask, RaySimd& ray, HitSimd& hit, RayStats& stats, RayHint hint, IntersectorFilterSimd* filter, void* payload) override
  {
    IntersectContext context(hint, filter, payload);

    stats.rayCount += bitCount(toIntMask(mask));

    vint emask = select(mask, vint(-1), zero);
    for (size_t i = 0; i < simdSize; i += 8)
    {
      RTCRayHit8 eray;
      initRay(ray, i, eray.ray);
      initHit(eray.hit);
      rtcIntersect8((const int*)&emask[i], sceneHandle, &eray, &context.intersectArgs);

      vint8 geomId = load<8>((int*)eray.hit.geomID);
      if (any(geomId != RTC_INVALID_GEOMETRY_ID))
      {
        store(&ray.far[i],    load<8>(eray.ray.tfar));
        store(&hit.primId[i], load<8>((int*)eray.hit.primID));
        store(&hit.geomId[i], geomId);
        store(&hit.instId[i], load<8>((int*)eray.hit.instID[0]));
        store(&hit.u[i],      load<8>(eray.hit.u));
        store(&hit.v[i],      load<8>(eray.hit.v));
      }
    }
  }

  void occluded(vbool mask, RaySimd& ray, RayStats& stats, RayHint hint, IntersectorFilterSimd* filter, void* payload) override
  {
    IntersectContext context(hint, filter, payload);

    stats.rayCount += bitCount(toIntMask(mask));

    vint emask = select(mask, vint(-1), zero);
    for (size_t i = 0; i < simdSize; i += 8)
    {
      RTCRay8 eray;
      initRay(ray, i, eray);
      rtcOccluded8((const int*)&emask[i], sceneHandle, &eray, &context.occludedArgs);

      vbool8 hitMask = load<8>(eray.tfar) < vfloat8(zero);
      store(hitMask, &ray.far[i], vfloat8(zero));
    }
  }
};
#else
typedef EmbreeIntersectorPacket EmbreeIntersectorPacket8;
#endif

class EmbreeIntersectorSinglePacket : public IntersectorPacket, EmbreeIntersector
{
public:
  EmbreeIntersectorSinglePacket(const std::shared_ptr<Group>& scene, bool filter, const Props& props, Props& stats) : EmbreeIntersector(scene, filter, props, stats) {}

  void intersect(vbool mask, RaySimd& ray, HitSimd& hit, RayStats& stats, RayHint hint, IntersectorFilterSimd* filter, void* payload) override
  {
    IntersectContext context(hint, filter, payload);

    int intMask = toIntMask(mask);
    stats.rayCount += bitCount(intMask);

    int i = -1;
    while ((i = bitScan(intMask, i)) < simdSize)
    {
      RTCRayHit eray;
      initRay(ray, i, eray.ray);
      initHit(eray.hit);
      rtcIntersect1(sceneHandle, &eray, &context.intersectArgs);

      if (eray.hit.geomID != RTC_INVALID_GEOMETRY_ID)
      {
        ray.far[i] = eray.ray.tfar;
        hit.primId[i] = eray.hit.primID;
        hit.geomId[i] = eray.hit.geomID;
        hit.instId[i] = eray.hit.instID[0];
        hit.u[i] = eray.hit.u;
        hit.v[i] = eray.hit.v;
      }
    }
  }

  void occluded(vbool mask, RaySimd& ray, RayStats& stats, RayHint hint, IntersectorFilterSimd* filter, void* payload) override
  {
    IntersectContext context(hint, filter, payload);

    int intMask = toIntMask(mask);
    stats.rayCount += bitCount(intMask);

    int i = -1;
    while ((i = bitScan(intMask, i)) < simdSize)
    {
      RTCRay eray;
      initRay(ray, i, eray);
      rtcOccluded1(sceneHandle, &eray, &context.occludedArgs);

      if (eray.tfar < 0.f)
        ray.far[i] = 0.f;
    }
  }
};

} // namespace prt
