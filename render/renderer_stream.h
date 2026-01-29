// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "core/intersector_stream.h"
#include "integrator.h"
#include "renderer.h"

namespace prt {

class RendererStream
{
public:
  template <int streamSize>
  static Props queryRay(const Scene* scene,
                        IntersectorStream<streamSize>* intersector,
                        const Ray& inputRay)
  {
    Props result;

    // Shoot the ray
    RaySimd ray;
    ray.org  = inputRay.org;
    ray.dir  = inputRay.dir;
    ray.near = inputRay.near;
    ray.far  = inputRay.far;
    RayStream<streamSize> rays;
    rays.setA(0, ray);
    HitStream<streamSize> hits;
    ShadingContextSimd ctx;
    RayStats stats;
    intersector->intersect(rays, hits, 1, stats);
    rays.getA(0, ray);
    HitSimd hit;
    hits.getA(0, hit);
    if (none(ray.isHit())) return result;
    scene->postIntersect(1, ray, hit, ctx);

    int primId = *hits.getPrimId();
    int matId = scene->getMaterialId(1, hit)[0];
    const Material* mat = scene->getMaterial(matId);
    int matType = mat->getType();
    Vec3vf T = (matType & MaterialType::Transparent) ? mat->getTransparency(1, ctx, -ray.dir) : zero;

    // Fill the query result
    result.set("mat", scene->getMaterialName(matId));
    result.set("matId", matId);
    result.set("matType", matType);
    result.set("p", toScalar(ray.getHitPoint()));
    result.set("Ng", toScalar(ctx.Ng));
    result.set("N", toScalar(ctx.f.N));
    result.set("U", toScalar(ctx.f.U));
    result.set("V", toScalar(ctx.f.V));
    result.set("primId", primId);
    result.set("uv", toScalar(ctx.uv[0]));
    result.set("T", toScalar(T));
    result.set("eps", toScalar(ctx.eps));
    result.set("dist", toScalar(ray.far));

    return result;
  }

  template <int streamSize>
  static bool queryOcclusionRay(const Scene* scene,
                  IntersectorStream<streamSize>* intersector,
                  IntersectorFilterSimd* filter,
                  const Ray& inputRay)
  {
    // Shoot the ray
    RaySimd ray;
    ray.org  = inputRay.org;
    ray.dir  = inputRay.dir;
    ray.near = inputRay.near;
    ray.far  = inputRay.far;
    RayStream<streamSize> rays;
    rays.setA(0, ray);
    RayStats stats;
    Vec3f T = 1.f;
    float* Tptr[] = {&T.x, &T.y, &T.z};
    intersector->occluded(rays, 1, stats, RayHint::None, filter, &Tptr);
    rays.getA(0, ray);
    return any(ray.isOccluded());
  }
};

template <int streamSize>
prt_inline int rayStreamSort(const RayStream<streamSize>& ray, int* pathId, int rayCount)
{
  int missCount = 0;
  for (int i = 0; i < rayCount; ++i)
    if (!ray.isHit(i))
      missCount++;

  int missIndex = 0;
  int hitIndex = missCount;
  for (int i = 0; i < rayCount; ++i)
  {
    if (ray.isHit(i))
      pathId[hitIndex++] = i;
    else
      pathId[missIndex++] = i;
  }

  return missCount;
}

struct RayStreamMaterialIdSort
{
  static const int maxMatCount = 2048;
  prt_align_cache int buckets[maxMatCount];

  template <int streamSize>
  prt_inline void operator ()(const Scene& scene, const RayStream<streamSize>& ray, const HitStream<streamSize>& hit, int* matId, int* pathId, int rayCount)
  {
    int matCount = scene.getMaterialCount();

    #pragma nounroll
    for (int i = 0; i < matCount; i += simdSize*2)
    {
      store(buckets + i, vint(zero));
      store(buckets + i + simdSize, vint(zero));
    }

    for (int i = 0; i < rayCount; ++i)
    {
      int id;
      if (ray.isHit(i))
      {
        Hit curHit;
        curHit.primId = hit.primId[i];
        curHit.geomId = hit.geomId[i];
        curHit.instId = hit.instId[i];
        id = scene.getMaterialId(curHit);
      }
      else
      {
        id = 0;
      }
      matId[i] = id;
      buckets[id]++;
    }

    /*
    // FIXME: crashes
    for (int i = 0; i < rayCount; i += simdSize)
    {
      vbool m = (vint(step) + i) < rayCount;
      vint id = 0;

      vbool mHit = m & ray.isHitA(i);
      if (any(mHit))
      {
        HitSimd curHit;
        curHit.primId = hit.primId.getA(i);
        curHit.geomId = hit.geomId.getA(i);
        curHit.instId = hit.instId.getA(i);
        set(mHit, id, scene.getMaterialId(mHit, curHit));
      }
      store(matId + i, id);

      vint count = gather(m, buckets, id);
      count += 1;
      scatter(m, buckets, id, count);
    }
    */

    int sum = 0;
    for (int i = 0; i < matCount; ++i)
    {
      int bucket = buckets[i];
      buckets[i] = sum;
      sum += bucket;
    }
    for (int i = 0; i < rayCount; ++i)
    {
      int id = matId[i];
      pathId[buckets[id]++] = i;
    }
  }
};

} // namespace prt
