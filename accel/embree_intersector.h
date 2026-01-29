// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <embree4/rtcore.h>
#include <embree4/rtcore_ray.h>
#include "sys/array.h"
#include "sys/props.h"
#include "sys/logging.h"
#include "geometry/group.h"
#include "core/intersector_single.h"
#include "core/intersector_packet.h"
#include "core/intersector_stream.h"
#include <unordered_map>

namespace prt {

class EmbreeIntersector
{
protected:
  std::shared_ptr<Group> scene;
  RTCDevice deviceHandle;
  RTCScene sceneHandle;
  std::unordered_map<Group*, RTCScene> instancedSceneHandles;
  std::unordered_map<Geometry*, RTCGeometry> geometryHandles;
  RTCSceneFlags sceneFlags;
  RTCBuildQuality buildQuality;
  long long primCount;

public:
  EmbreeIntersector(const std::shared_ptr<Group>& scene, bool filter, const Props& props, Props& stats);
  virtual ~EmbreeIntersector();

protected:
  class IntersectContext
  {
  public:
    RTCRayQueryContext context;
    RTCIntersectArguments intersectArgs;
    RTCOccludedArguments occludedArgs;
    void* filter;
    void* payload;

  private:
    prt_inline void init(RayHint hint)
    {
      rtcInitRayQueryContext(&context);

      rtcInitIntersectArguments(&intersectArgs);
      intersectArgs.context = &context;

      rtcInitOccludedArguments(&occludedArgs);
      occludedArgs.context = &context;

      if (hint == RayHint::Coherent)
      {
        intersectArgs.flags = RTC_RAY_QUERY_FLAG_COHERENT;
        occludedArgs.flags  = RTC_RAY_QUERY_FLAG_COHERENT;
      }
      else
      {
        intersectArgs.flags = RTC_RAY_QUERY_FLAG_INCOHERENT;
        occludedArgs.flags  = RTC_RAY_QUERY_FLAG_INCOHERENT;
      }

      filter = nullptr;
      payload = nullptr;
    }

  public:
    explicit prt_inline IntersectContext(RayHint hint)
    {
      init(hint);
    }

    prt_inline IntersectContext(RayHint hint, IntersectorFilter* filter, void* payload)
    {
      init(hint);

      if (filter)
      {
        this->intersectArgs.filter = filterFunc;
        this->occludedArgs.filter  = filterFunc;
        this->intersectArgs.flags = RTCRayQueryFlags(this->intersectArgs.flags | RTC_RAY_QUERY_FLAG_INVOKE_ARGUMENT_FILTER);
        this->occludedArgs.flags  = RTCRayQueryFlags(this->occludedArgs.flags  | RTC_RAY_QUERY_FLAG_INVOKE_ARGUMENT_FILTER);
        this->filter = filter;
        this->payload = payload;
      }
    }

    prt_inline IntersectContext(RayHint hint, IntersectorFilterSimd* filter, void* payload)
    {
      init(hint);

      if (filter)
      {
        this->intersectArgs.filter = filterSimdFunc;
        this->occludedArgs.filter  = filterSimdFunc;
        this->intersectArgs.flags  = RTCRayQueryFlags(this->intersectArgs.flags | RTC_RAY_QUERY_FLAG_INVOKE_ARGUMENT_FILTER);
        this->occludedArgs.flags   = RTCRayQueryFlags(this->occludedArgs.flags  | RTC_RAY_QUERY_FLAG_INVOKE_ARGUMENT_FILTER);
        this->filter = filter;
        this->payload = payload;
      }
    }
  };

  prt_inline void initRay(const Ray& ray, RTCRay& eray, int rayId = 0)
  {
    eray.org_x = ray.org.x;
    eray.org_y = ray.org.y;
    eray.org_z = ray.org.z;
    eray.tnear = ray.near;

    eray.dir_x = ray.dir.x;
    eray.dir_y = ray.dir.y;
    eray.dir_z = ray.dir.z;
    eray.time = 0.f;

    eray.tfar = ray.far;
    eray.mask = -1;
    eray.id = rayId;
  }

  prt_inline void initHit(RTCHit& ehit)
  {
    ehit.geomID = RTC_INVALID_GEOMETRY_ID;
    ehit.instID[0] = RTC_INVALID_GEOMETRY_ID;
  }

  template <class RTCRayT>
  prt_inline void initRay(const RaySimd& ray, RTCRayT& eray, int rayId = 0)
  {
    store(eray.org_x, ray.org.x);
    store(eray.org_y, ray.org.y);
    store(eray.org_z, ray.org.z);
    store(eray.tnear, ray.near);

    store(eray.dir_x, ray.dir.x);
    store(eray.dir_y, ray.dir.y);
    store(eray.dir_z, ray.dir.z);
    store(eray.time, vfloat(zero));

    store(eray.tfar, ray.far);
    store((int*)eray.mask, vint(-1));
    store((int*)eray.id, vint(step) + rayId);
  }

  template <class RTCHitT>
  prt_inline void initHit(RTCHitT& ehit)
  {
    store((int*)ehit.geomID, vint(RTC_INVALID_GEOMETRY_ID));
    store((int*)ehit.instID[0], vint(RTC_INVALID_GEOMETRY_ID));
  }

  prt_inline void initRay(const RaySimd& ray, int i, RTCRay8& eray, int rayId = 0)
  {
    store(eray.org_x, load<8>(&ray.org.x[i]));
    store(eray.org_y, load<8>(&ray.org.y[i]));
    store(eray.org_z, load<8>(&ray.org.z[i]));
    store(eray.tnear, load<8>(&ray.near[i]));

    store(eray.dir_x, load<8>(&ray.dir.x[i]));
    store(eray.dir_y, load<8>(&ray.dir.y[i]));
    store(eray.dir_z, load<8>(&ray.dir.z[i]));
    store(eray.time, vfloat8(zero));

    store(eray.tfar, load<8>(&ray.far[i]));
    store((int*)eray.mask, vint8(-1));
    store((int*)eray.id, vint8(step) + rayId);
  }

  prt_inline void initHit(RTCHit8& ehit)
  {
    store((int*)ehit.geomID, vint8(RTC_INVALID_GEOMETRY_ID));
    store((int*)ehit.instID[0], vint8(RTC_INVALID_GEOMETRY_ID));
  }

  prt_inline void initRay(const RaySimd& ray, int i, RTCRay& eray, int rayId = 0)
  {
    eray.org_x = ray.org.x[i];
    eray.org_y = ray.org.y[i];
    eray.org_z = ray.org.z[i];
    eray.tnear = ray.near[i];

    eray.dir_x = ray.dir.x[i];
    eray.dir_y = ray.dir.y[i];
    eray.dir_z = ray.dir.z[i];
    eray.time = 0.f;

    eray.tfar = ray.far[i];
    eray.mask = -1;
    eray.id = rayId;
  }

  template <int streamSize>
  prt_inline void initRay(const RayStream<streamSize>& rays, int i, RTCRay& eray, int rayId = 0)
  {
    eray.org_x = rays.org.x[i];
    eray.org_y = rays.org.y[i];
    eray.org_z = rays.org.z[i];
    eray.tnear = rays.near[i];

    eray.dir_x = rays.dir.x[i];
    eray.dir_y = rays.dir.y[i];
    eray.dir_z = rays.dir.z[i];
    eray.time = 0.f;

    eray.tfar = rays.far[i];
    eray.mask = -1;
    eray.id = rayId;
  }

private:
  static void filterFunc(const RTCFilterFunctionNArguments* args);
  static void filterSimdFunc(const RTCFilterFunctionNArguments* args);

  RTCGeometry createGeometry(Geometry* geometry);
};

} // namespace prt
