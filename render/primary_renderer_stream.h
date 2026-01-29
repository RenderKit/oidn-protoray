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
class PrimaryRendererStream : public Renderer, IntegratorBase
{
private:
  struct State
  {
    RayStream<streamSize> ray;
    RayStreamChannel<int, streamSize> pixelId;
    HitStream<streamSize> hit;
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
  PrimaryRendererStream(const std::shared_ptr<const Scene>& scene, const std::shared_ptr<IntersectorStream<streamSize>>& intersector, const Props& props)
    : Renderer(scene, props),
      IntegratorBase(props)
  {
    this->intersector = intersector;

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
      throw std::invalid_argument("PrimaryRendererStream: framebuffer size mismatch");

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
    return sampleDimBaseSize;
  }

  void renderTile(const Vec2i& tileId, int tid)
  {
    State* state = states[tid].get();

    Vec2i tileLow = tileId * Vec2i(tileSizeX, tileSizeY);
    //int rayCount = tileSizeX*tileSizeY;
    int croppedTileSizeY = min(imageSize.y - tileLow.y, tileSizeY);
    int rayCount = tileSizeX*croppedTileSizeY;

    for (int i = 0; i < rayCount; i += simdSize)
    {
      /*
      Vec2vi pixel;
      pixel.x = load(mortonTableX + i) + tileLow.x;
      pixel.y = load(mortonTableY + i) + tileLow.y;
      */
      Vec2vi pixel = getMorton8x8Step<tileSizeX>(i) + Vec2vi(tileLow);
      vint pixelId = pixel.x | (pixel.y << 16);
      state->pixelId.setA(i, pixelId);

      sampler.setSample(one, state->sampler, sampleIndex, pixelId);

      // Sample the pixel
      Vec2vf pixelSample;
      if (frameBuffer->hasJitter())
      {
        pixelSample = frameBuffer->getJitter();
      }
      else
      {
        pixelSample = sampler.get2D(one, state->sampler, sampleDimPixel);
        if (frameBuffer->getFilter() && frameBuffer->getFilterMode() == PixelFilterMode::Sample)
          pixelSample = frameBuffer->getFilter()->sample(one, pixelSample) + vfloat(0.5f);
      }

      // Sample the camera
      CameraSampleSimd cameraSample;
      cameraSample.image = (toFloat(pixel) + pixelSample) * Vec2vf(frameBuffer->getInvSize());
      cameraSample.lens = sampler.get2D(one, state->sampler, sampleDimLens);

      RaySimd ray;
      camera->getRay(ray, cameraSample);

      state->ray.setA(i, ray);
    }

    intersector->intersect(state->ray, state->hit, rayCount, state->rayStats, RayHint::Coherent);

    for (int i = 0; i < rayCount; i += simdSize)
    {
      RaySimd ray;
      HitSimd hit;
      state->ray.getA(i, ray);
      state->hit.getA(i, hit);

      vbool m = ray.isHit();

      ShadingContextT ctx;
      scene->postIntersect(m, ray, hit, ctx);

      Vec3vf color = (ctx.getN() + vfloat(1.0f)) * vfloat(0.5f);
      color = select(m, color, Vec3vf(0.05f));

      vint pixelId = state->pixelId.getA(i);
      Vec2vi pixel(pixelId & 0xffff, shr(pixelId, 16));
      if (accum)
        frameBuffer->getColor().add(pixel, color);
      else
        frameBuffer->getColor().set(pixel, color);
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
