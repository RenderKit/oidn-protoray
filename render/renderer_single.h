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
#include "core/intersector_single.h"
#include "renderer.h"
#include "integrator.h"
#include "transparent_filter.h"

namespace prt {

template <class Integrator, class Sampler, bool accum = true, int tileSizeX = 8, int tileSizeY = 8>
class RendererSingle : public Renderer
{
private:
  Integrator integrator;
  Sampler sampler;
  std::shared_ptr<IntersectorSingle> intersector;
  std::shared_ptr<IntersectorFilter> filter;
  const Camera* camera;
  FrameBuffer* frameBuffer;
  Array<IntegratorState<Sampler>> states;
  int sampleIndex;
  bool isStatic;

public:
  RendererSingle(const std::shared_ptr<const Scene>& scene, const std::shared_ptr<IntersectorSingle>& intersector, const Props& props)
    : Renderer(scene, props),
      integrator(props),
      camera(nullptr),
      frameBuffer(nullptr),
      sampleIndex(0)
  {
    this->intersector = intersector;

    if (scene->hasTransparentMaterials())
      filter = std::make_shared<TransparentShadowFilter>(scene);

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
      throw std::invalid_argument("RendererSingle: framebuffer size mismatch");

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
#endif
    if (!isStatic)
      sampleIndex++;
  }

private:
  void renderTile(const Vec2i& tileId, int tid)
  {
    IntegratorState<Sampler>& state = states[tid];
    Vec2i tileLow = tileId * Vec2i(tileSizeX, tileSizeY);

    for (int i = 0; i < tileSizeX*tileSizeY; ++i)
    {
      prefetchL1(camera); // workaround for strange slowdown on MIC

      Vec2i pixel = Vec2i(mortonTableX[i], mortonTableY[i]) + tileLow;
      int pixelId = pixel.x | (pixel.y << 16);
      sampler.setSample(state.sampler, sampleIndex, pixelId);

      CameraSample cameraSample;
      Vec2f pixelSample;
      if (frameBuffer->hasJitter())
        pixelSample = frameBuffer->getJitter();
      else
        pixelSample = sampler.get2D(state.sampler, Integrator::sampleDimPixel);
      cameraSample.image = (toFloat(pixel) + pixelSample) * frameBuffer->getInvSize();
      cameraSample.lens = sampler.get2D(state.sampler, Integrator::sampleDimLens);

      Ray ray;
      camera->getRay(ray, cameraSample);

      Vec3f color = integrator.getColor(ray, intersector.get(), filter.get(), scene.get(), sampler, state);
      //if (!all(isfinite(color))) LogWarn() << "Infinite radiance: " << color;

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
    Ray ray = inputRay;
    Hit hit;
    ShadingContext ctx;
    RayStats stats;
    intersector->intersect(ray, hit, stats);
    if (!ray.isHit()) return result;
    scene->postIntersect(ray, hit, ctx);

    int matId = scene->getMaterialId(hit);
    const Material* mat = scene->getMaterial(matId);
    int matType = mat->getType();
    Vec3f T = (matType & MaterialType::Transparent) ? mat->getTransparency(ctx, -ray.dir) : zero;

    // Fill the query result
    result.set("mat", scene->getMaterialName(matId));
    result.set("matId", matId);
    result.set("matType", matType);
    result.set("p", ray.getHitPoint());
    result.set("Ng", ctx.Ng);
    result.set("N", ctx.f.N);
    result.set("U", ctx.f.U);
    result.set("V", ctx.f.V);
    result.set("primId", hit.primId);
    result.set("uv", ctx.uv[0]);
    result.set("T", T);
    result.set("eps", ctx.eps);
    result.set("dist", ray.far);

    return result;
  }

  bool queryOcclusionRay(const Ray& inputRay) override
  {
    // Shoot the ray
    Ray ray = inputRay;
    RayStats stats;
    Vec3f T = one;
    intersector->occluded(ray, stats, RayHint::None, filter.get(), &T);
    return ray.isOccluded();
  }

  void update(const Props& props) override
  {
    integrator.updateIntegrator(props);
  }
};

} // namespace prt
