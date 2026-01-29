// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/array.h"
#include "sys/tasking.h"
#include "sys/sysinfo.h"
#include "sys/timer.h"
#include "image/pixel.h"
#include "image/morton.h"
#include "sampling/shape_sampler.h"
#include "renderer_stream.h"

namespace prt {

template <class ShadingContextT, class Sampler, bool accum, int streamSize, int tileSizeX, int tileSizeY>
class DiffuseRendererStream : public Renderer, IntegratorBase
{
private:
  struct StateIo
  {
    RayStream<streamSize> ray;
    RayStreamChannel<int, streamSize> pixelId;
  };

  struct State
  {
    RayStreamChannel<int, streamSize> pathId;
    HitStream<streamSize> hit;
    StateIo io[2];
    typename Sampler::State sampler;
    RayStats rayStats;
  };

  Sampler sampler;
  std::shared_ptr<IntersectorStream<streamSize>> intersector;
  const Camera* camera;
  FrameBuffer* frameBuffer;
  Array<std::shared_ptr<State>> states;
  int sampleIndex;
  bool isStatic;

public:
  DiffuseRendererStream(const std::shared_ptr<const Scene>& scene, const std::shared_ptr<IntersectorStream<streamSize>>& intersector, const Props& props)
    : Renderer(scene, props),
      IntegratorBase(props)
  {
    this->intersector = intersector;
    maxDepth = props.get("maxDepth", 6);

    // Initialize the sampler
    initSampler(0);

    states.alloc(cpuCount);
    for (int i = 0; i < states.getSize(); ++i)
      states[i] = std::make_shared<State>();

    isStatic = props.exists("static");
  }

  std::shared_ptr<Intersector> getIntersector() const override { return intersector; }

  void initSampler(unsigned int seed) override
  {
    sampler.init(getSampleSize(), sampleCount, seed);
    sampleIndex = 0;
  }

  void render(const Camera* camera, FrameBuffer* frameBuffer, Props& stats) override
  {
    if (frameBuffer->getSize() != imageSize)
      throw std::invalid_argument("DiffuseRendererStream: framebuffer size mismatch");

    this->camera = camera;
    this->frameBuffer = frameBuffer;

    Vec2i tileSize = Vec2i(tileSizeX, tileSizeY);
    Vec2i gridSize = (imageSize + tileSize - 1) / tileSize;
    Timer timer;
    Tasking::run(gridSize, [this](const Vec2i& tileId, int tid) { renderTile(tileId, tid); });
    double totalTime = timer.query();

    // Stats
    RayStats rayStats;
    for (int i = 0; i < states.getSize(); ++i)
    {
      rayStats += states[i]->rayStats;
      states[i]->rayStats.reset();
    }

    double mray = (double)rayStats.rayCount / 1000000.0 / totalTime;
    stats.set("mray", mray);
    stats.set("ray", rayStats.rayCount);

#ifdef PRT_PROFILE_MODE
    stats.set("nodes", (double)rayStats.nodeCount / rayStats.rayCount);
    stats.set("prims", (double)rayStats.primCount / rayStats.rayCount);
#endif
    if (!isStatic)
      ++sampleIndex;
  }

private:
  int getSampleSize() const
  {
    return sampleDimBaseSize + 2 * maxDepth;
  }

  void renderTile(const Vec2i& tileId, int tid)
  {
    State* state = states[tid].get();
    StateIo* stateI = &state->io[0];
    StateIo* stateO = &state->io[1];

    vfloat throughput = 1.f;

    Vec2i tileLow = tileId * Vec2i(tileSizeX, tileSizeY);
    int croppedTileSizeY = min(imageSize.y - tileLow.y, tileSizeY);

    int rayCount = tileSizeX*croppedTileSizeY;
    for (int i = 0; i < rayCount; i += simdSize)
    {
      //Vec2vi pixel;
      //pixel.x = load(mortonTableX + i) + tileLow.x;
      //pixel.y = load(mortonTableY + i) + tileLow.y;
      Vec2vi pixel = getMorton8x8Step<tileSizeX>(i) + Vec2vi(tileLow);
      vint pixelId = pixel.x | (pixel.y << 16);
      stateI->pixelId.setA(i, pixelId);

      sampler.setSample(one, state->sampler, sampleIndex, pixelId);

      CameraSampleSimd cameraSample;
      Vec2vf pixelSample;
      if (frameBuffer->hasJitter())
        pixelSample = frameBuffer->getJitter();
      else
        pixelSample = sampler.get2D(one, state->sampler, sampleDimPixel);
      cameraSample.image = (toFloat(pixel) + pixelSample) * Vec2vf(frameBuffer->getInvSize());
      cameraSample.lens = sampler.get2D(one, state->sampler, sampleDimLens);

      RaySimd ray;
      camera->getRay(ray, cameraSample);

      stateI->ray.setA(i, ray);
    }

    int depth = 0;
    for (;;)
    {
      // Intersect rays
      //RayHint rayHint = (depth == 0) ? RayHint::Coherent : RayHint::Incoherent;
      RayHint rayHint = RayHint::Incoherent; // always use incoherent to avoid frequency drop on SKX
      intersector->intersect(stateI->ray, state->hit, rayCount, state->rayStats, rayHint);

      // Sort
      int missCount = rayStreamSort(stateI->ray, state->pathId.get(), rayCount);

      // No hits
      for (int i = 0; i < missCount; i += simdSize)
      {
        vbool m = (vint(step) + i) < missCount;
        vint pathId = state->pathId.getA(i);

        vint pixelId = stateI->pixelId.get(m, pathId);
        Vec2vi pixel(pixelId & 0xffff, shr(pixelId, 16));
        if (accum)
          frameBuffer->getColor().add(m, pixel, Vec3vf(throughput));
        else
          frameBuffer->getColor().set(m, pixel, Vec3vf(throughput));
      }

      if (missCount == rayCount) break;
      if (depth == maxDepth)
      {
        for (int i = missCount; i < rayCount; i += simdSize)
        {
          vbool m = (vint(step) + i) < rayCount;
          vint pathId = state->pathId.get(i);
          vint pixelId = stateI->pixelId.get(m, pathId);
          Vec2vi pixel(pixelId & 0xffff, shr(pixelId, 16));
          if (accum)
            frameBuffer->getColor().add(m, pixel, Vec3vf(zero));
          else
            frameBuffer->getColor().set(m, pixel, Vec3vf(zero));
        }
        break;
      }

      // Hits
      for (int i = missCount, o = 0; i < rayCount; i += simdSize, o += simdSize)
      {
        vbool m = (vint(step) + i) < rayCount;
        vint pathId = state->pathId.get(i);

        RaySimd ray;
        HitSimd hit;
        stateI->ray.get(m, pathId, ray);
        state->hit.get(m, pathId, hit);

        ShadingContextT ctx;
        scene->postIntersect(m, ray, hit, ctx);

        vint pixelId = stateI->pixelId.get(m, pathId);
        sampler.resetSample(m, state->sampler, sampleIndex, pixelId);
        Vec2vf s = sampler.get2D(m, state->sampler, sampleDimBaseSize + 2 * depth);
        ray.init(ctx.p, ctx.getFrame() * cosineSampleHemisphere(s), ctx.eps);
        stateO->ray.setA(o, ray);

        stateO->pixelId.setA(o, pixelId);
      }

      throughput *= 0.8f;

      rayCount -= missCount;
      swap(stateI, stateO);
      depth++;
    }
  }

public:
  Props queryRay(const Ray& ray) override
  {
    return RendererStream::queryRay(scene.get(), intersector.get(), ray);
  }

  bool queryOcclusionRay(const Ray& ray) override
  {
    return RendererStream::queryOcclusionRay(scene.get(), intersector.get(), nullptr, ray);
  }
};

} // namespace prt
