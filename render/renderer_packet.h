// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/logging.h"
#include "sys/array.h"
#include "sys/tasking.h"
#include "sys/sysinfo.h"
#include "sys/timer.h"
#include "image/pixel.h"
#include "image/morton.h"
#include "core/intersector_packet.h"
#include "renderer.h"
#include "integrator.h"
#include "transparent_filter.h"

namespace prt {

template <class Integrator, class Sampler, bool accum = true, int tileSizeX = 8, int tileSizeY = 8>
class RendererPacket : public Renderer
{
private:
  Integrator integrator;
  Sampler sampler;
  std::shared_ptr<IntersectorPacket> intersector;
  std::shared_ptr<IntersectorFilterSimd> filter;
  const Camera* camera;
  FrameBuffer* frameBuffer;
  Array<IntegratorState<Sampler>> states;
  int sampleIndex;
  bool isStatic;

public:
  RendererPacket(const std::shared_ptr<const Scene>& scene, const std::shared_ptr<IntersectorPacket>& intersector, const Props& props)
    : Renderer(scene, props),
      integrator(props),
      camera(nullptr),
      frameBuffer(nullptr),
      sampleIndex(0)
  {
    this->intersector = intersector;

    if (scene->hasTransparentMaterials())
      filter = std::make_shared<TransparentShadowFilterSimd>(scene);

    // Initialize the sampler
    initSampler(0);

    states.alloc(cpuCount);

    isStatic = props.exists("static");

    Log() << "Tile size: " << tileSizeX << "x" << tileSizeY;
  }

  std::shared_ptr<Intersector> getIntersector() const override { return intersector; }

  void initSampler(unsigned int seed) override
  {
    sampler.init(integrator.getSampleSize(), sampleCount, seed);
    sampleIndex = 0;
  }

  void render(const Camera* camera, FrameBuffer* frameBuffer, Props& stats) override
  {
    if (frameBuffer->getSize() != imageSize)
      throw std::invalid_argument("RendererPacket: framebuffer size mismatch");

    this->camera = camera;
    this->frameBuffer = frameBuffer;

    Vec2i gridSize = imageSize / Vec2i(tileSizeX, tileSizeY);
    Timer timer;
    Tasking::run(gridSize, [this](const Vec2i& tileId, int tid) { renderTile(tileId, tid); });
    double totalTime = timer.query();

    // Stats
    RayStats rayStats;
    for (auto& state : states)
    {
      rayStats += state.rayStats;
      state.rayStats.reset();
    }

    double mray = (double)rayStats.rayCount / 1000000.0 / totalTime;
    stats.set("mray", mray);
    stats.set("ray", rayStats.rayCount);
    stats.set("spp", 1);

#ifdef PRT_PROFILE_MODE
    stats.set("nodes", (double)rayStats.nodeCount / rayStats.rayCount);
    stats.set("prims", (double)rayStats.primCount / rayStats.rayCount);
    stats.set("shadeSimdUtil", (double)rayStats.shadeSimdActiveLaneCount / ((double)rayStats.shadeSimdBatchCount * simdSize) * 100.0);
#endif
    if (!isStatic)
      sampleIndex++;
  }

private:
  void renderTile(const Vec2i& tileId, int tid)
  {
    IntegratorState<Sampler>& state = states[tid];
    Vec2i tileLow = tileId * Vec2i(tileSizeX, tileSizeY);

    for (int i = 0; i < tileSizeX*tileSizeY; i += simdSize)
    {
      prefetchL1(camera); // workaround for strange slowdown on MIC

      Vec2vi pixel;
      pixel.x = load(mortonTableX + i) + tileLow.x;
      pixel.y = load(mortonTableY + i) + tileLow.y;
      vint pixelId = pixel.x | (pixel.y << 16);
      sampler.setSample(one, state.sampler, sampleIndex, pixelId);

      CameraSampleSimd cameraSample;
      Vec2vf pixelSample;
      if (frameBuffer->hasJitter())
        pixelSample = frameBuffer->getJitter();
      else
        pixelSample = sampler.get2D(one, state.sampler, Integrator::sampleDimPixel);
      cameraSample.image = (toFloat(pixel) + pixelSample) * Vec2vf(frameBuffer->getInvSize());
      cameraSample.lens = sampler.get2D(one, state.sampler, Integrator::sampleDimLens);

      RaySimd ray;
      camera->getRay(ray, cameraSample);

      Vec3vf color = integrator.getColor(ray, intersector.get(), filter.get(), scene.get(), sampler, state);
      if (accum)
        frameBuffer->getColor().add(pixel, color);
      else
        frameBuffer->getColor().set(pixel, color);
    }
  }

public:
  Props queryRay(const Ray& inputRay) override
  {
    Props result;

    // Shoot the ray
    RaySimd ray;
    ray.org  = inputRay.org;
    ray.dir  = inputRay.dir;
    ray.near = inputRay.near;
    ray.far  = inputRay.far;
    HitSimd hit;
    ShadingContextSimd ctx;
    RayStats stats;
    intersector->intersect(1, ray, hit, stats);
    if (none(ray.isHit())) return result;
    scene->postIntersect(1, ray, hit, ctx);

    int primId = *hit.getPrimId();
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

  bool queryOcclusionRay(const Ray& inputRay) override
  {
    // Shoot the ray
    RaySimd ray;
    ray.org  = inputRay.org;
    ray.dir  = inputRay.dir;
    ray.near = inputRay.near;
    ray.far  = inputRay.far;
    RayStats stats;
    Vec3f T = 1.f;
    float* Tptr[] = {&T.x, &T.y, &T.z};
    intersector->occluded(1, ray, stats, RayHint::None, filter.get(), &Tptr);
    return any(ray.isOccluded());
  }

  void update(const Props& props) override
  {
    integrator.updateIntegrator(props);
  }
};

} // namespace prt
