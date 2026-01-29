// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "sys/timer.h"
#include "geometry/instance.h"
#include "geometry/triangle_mesh.h"
#include "embree_intersector.h"

namespace prt {

EmbreeIntersector::EmbreeIntersector(const std::shared_ptr<Group>& scene, bool filter, const Props& props, Props& stats)
  : scene(scene)
{
  bool isHighQuality = !props.exists("no-sbvh");
  bool isBenchmark = props.exists("benchmark");
  int buildCount = props.get("buildCount", isBenchmark ? 12 : 1);
  int buildWarmup = props.get("buildWarmup", buildCount / 6);

  std::string deviceCfg = props.get("embree", "");
  deviceCfg = props.get("rtcore", deviceCfg);

  if (buildCount > 1)
  {
    std::string deviceCfg2 = "tri_accel=bvh8.triangle4,tri_builder=";
    if (isHighQuality)
      deviceCfg2 += "sah_fast_spatial";
    else
      deviceCfg2 += "sah";

    if (deviceCfg.empty())
      deviceCfg = deviceCfg2;
    else
      deviceCfg = deviceCfg2 + "," + deviceCfg;
  }

  // Create device
  Log() << "Creating Embree device: " << deviceCfg;
  deviceHandle = rtcNewDevice(deviceCfg.c_str());

  // Create scene
  sceneFlags = (buildCount == 1 ? RTC_SCENE_FLAG_NONE : RTC_SCENE_FLAG_DYNAMIC);
  if (filter)
    sceneFlags = sceneFlags | RTC_SCENE_FLAG_FILTER_FUNCTION_IN_ARGUMENTS;
  buildQuality = isHighQuality ? RTC_BUILD_QUALITY_HIGH : RTC_BUILD_QUALITY_MEDIUM;

  sceneHandle = rtcNewScene(deviceHandle);
  rtcSetSceneFlags(sceneHandle, sceneFlags);
  rtcSetSceneBuildQuality(sceneHandle, buildQuality);

  // Build
  if (buildCount == 1)
    Log() << "Building acceleration structure";
  else
    Log() << "Building acceleration structure (" << buildCount << "x)";

  primCount = 0;
  for (int i = 0; i < scene->children.getSize(); ++i)
  {
    Geometry* child = scene->children[i].get();
    RTCGeometry childHandle = createGeometry(child);
    rtcAttachGeometryByID(sceneHandle, childHandle, i);
  }

  Timer timer;
  double buildTimeSum = 0.0;

  for (int buildIndex = 0; buildIndex < buildCount; ++buildIndex)
  {
    timer.reset();
    for (auto item : geometryHandles)
      rtcCommitGeometry(item.second);
    for (auto item : instancedSceneHandles)
      rtcCommitScene(item.second);
    rtcCommitScene(sceneHandle);
    double buildTime = timer.query();
    if (buildCount == 1 || buildIndex >= buildWarmup)
      buildTimeSum += buildTime;
  }

  // Stats
  double buildTimeAvg = (buildCount == 1) ? buildTimeSum : (buildTimeSum / double(max(buildCount-buildWarmup, 0)));
  double buildMsAvg = buildTimeAvg * 1000.0;
  double buildMprimAvg = double(primCount) / 1000000.0 / buildTimeAvg;

  stats.set("buildMs", buildMsAvg);
  stats.set("buildMprim", buildMprimAvg);

  Log() << "Build time: " << buildMsAvg << " ms";
  Log() << "Build speed: " << buildMprimAvg << " Mprim/s";
}

RTCGeometry EmbreeIntersector::createGeometry(Geometry* geometry)
{
  auto geometryHandleItem = geometryHandles.find(geometry);
  if (geometryHandleItem != geometryHandles.end())
    return geometryHandleItem->second;

  RTCGeometry geometryHandle;

  if (Instance* instance = dynamic_cast<Instance*>(geometry))
  {
    geometryHandle = rtcNewGeometry(deviceHandle, RTC_GEOMETRY_TYPE_INSTANCE);
    rtcSetGeometryTransform(geometryHandle, 0, RTC_FORMAT_FLOAT3X4_COLUMN_MAJOR, &instance->transform);

    RTCScene instancedSceneHandle;
    Group* group = instance->child.get();
    auto instancedSceneHandleItem = instancedSceneHandles.find(group);
    if (instancedSceneHandleItem == instancedSceneHandles.end())
    {
      instancedSceneHandle = rtcNewScene(deviceHandle);
      rtcSetSceneFlags(instancedSceneHandle, sceneFlags);
      rtcSetSceneBuildQuality(instancedSceneHandle, buildQuality);

      for (int i = 0; i < group->children.getSize(); ++i)
      {
        Geometry* child = group->children[i].get();
        RTCGeometry childHandle = createGeometry(child);
        rtcAttachGeometryByID(instancedSceneHandle, childHandle, i);
      }

      instancedSceneHandles[group] = instancedSceneHandle;
    }
    else
    {
      instancedSceneHandle = instancedSceneHandleItem->second;
    }

    rtcSetGeometryInstancedScene(geometryHandle, instancedSceneHandle);
  }
  else if (TriangleMesh* mesh = dynamic_cast<TriangleMesh*>(geometry))
  {
    geometryHandle = rtcNewGeometry(deviceHandle, RTC_GEOMETRY_TYPE_TRIANGLE);
    rtcSetSharedGeometryBuffer(geometryHandle, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, mesh->vertexAttribs.getData(), 0, mesh->vertexStride*sizeof(int), mesh->vertexCount);
    rtcSetSharedGeometryBuffer(geometryHandle, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, mesh->indices.getData(), 0, sizeof(Vec3i), mesh->indices.getSize());
    primCount += mesh->getPrimCount();
  }
  else
    throw std::runtime_error("EmbreeIntersector: unsupported input geometry");

  geometryHandles[geometry] = geometryHandle;
  return geometryHandle;
}

EmbreeIntersector::~EmbreeIntersector()
{
  for (auto item : geometryHandles)
    rtcReleaseGeometry(item.second);
  for (auto item : instancedSceneHandles)
    rtcReleaseScene(item.second);
  rtcReleaseScene(sceneHandle);
  rtcReleaseDevice(deviceHandle);
}

void EmbreeIntersector::filterFunc(const RTCFilterFunctionNArguments* args)
{
  assert(args->N == 1);

  Ray ray;
  ray.org.x = RTCRayN_org_x(args->ray, 1, 0);
  ray.org.y = RTCRayN_org_y(args->ray, 1, 0);
  ray.org.z = RTCRayN_org_z(args->ray, 1, 0);
  ray.dir.x = RTCRayN_dir_x(args->ray, 1, 0);
  ray.dir.y = RTCRayN_dir_y(args->ray, 1, 0);
  ray.dir.z = RTCRayN_dir_z(args->ray, 1, 0);
  ray.far = RTCRayN_tfar(args->ray, 1, 0);

  Hit hit;
  hit.primId = RTCHitN_primID(args->hit, 1, 0);
  hit.geomId = RTCHitN_geomID(args->hit, 1, 0);
  hit.instId = RTCHitN_instID(args->hit, 1, 0, 0);
  hit.u = RTCHitN_u(args->hit, 1, 0);
  hit.v = RTCHitN_v(args->hit, 1, 0);

  IntersectContext* context = (IntersectContext*)args->context;
  IntersectorFilter* filter = (IntersectorFilter*)context->filter;
  bool valid = filter->filter(ray, hit, context->payload);
  args->valid[0] = valid ? -1 : 0;
}

void EmbreeIntersector::filterSimdFunc(const RTCFilterFunctionNArguments* args)
{
  const int N = args->N;
  IntersectContext* context = (IntersectContext*)args->context;
  IntersectorFilterSimd* filter = (IntersectorFilterSimd*)context->filter;

  for (int i = 0; i < N; i += simdSize)
  {
    vbool m = (vint(step) + i) < N;
    m &= loadu(m, args->valid + i) == -1;
    if (none(m))
      continue;

    RaySimd ray;
    ray.org.x = loadu(m, &RTCRayN_org_x(args->ray, N, i));
    ray.org.y = loadu(m, &RTCRayN_org_y(args->ray, N, i));
    ray.org.z = loadu(m, &RTCRayN_org_z(args->ray, N, i));
    ray.dir.x = loadu(m, &RTCRayN_dir_x(args->ray, N, i));
    ray.dir.y = loadu(m, &RTCRayN_dir_y(args->ray, N, i));
    ray.dir.z = loadu(m, &RTCRayN_dir_z(args->ray, N, i));
    ray.far = loadu(m, &RTCRayN_tfar(args->ray, N, i));
    vint rayId = loadu(m, (int*)&RTCRayN_id(args->ray, N, i));

    HitSimd hit;
    hit.primId = loadu(m, (int*)&RTCHitN_primID(args->hit, N, i));
    hit.geomId = loadu(m, (int*)&RTCHitN_geomID(args->hit, N, i));
    hit.instId = loadu(m, (int*)&RTCHitN_instID(args->hit, N, i, 0));
    hit.u = loadu(m, &RTCHitN_u(args->hit, N, i));
    hit.v = loadu(m, &RTCHitN_v(args->hit, N, i));

    vbool valid = filter->filter(m, rayId, ray, hit, context->payload);
    vint validi = select(valid, vint(-1), vint(0)); // FIXME
    storeu(m, args->valid + i, validi);
  }
}

} // namespace prt
