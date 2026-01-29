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

// Shoots one AO ray per pixel per stream
template <class ShadingContextT, class Sampler, bool accum, int streamSize, int tileSizeX, int tileSizeY>
class AoRendererStream : public Renderer, IntegratorBase
{
private:
  struct State
  {
    RayStream<streamSize> ray;
    HitStream<streamSize> hit;
    RayStreamChannel<int, streamSize> pixelId;
    RayStreamChannel<int, streamSize> pathId;
    RayStreamChannel3<float, streamSize> p;
    RayStreamChannel3<float, streamSize> U;
    RayStreamChannel3<float, streamSize> V;
    RayStreamChannel3<float, streamSize> N;
    RayStreamChannel<float, streamSize> eps;
    RayStreamChannel<float, streamSize> throughput;
    typename Sampler::State sampler;
    RayStats rayStats;
  };

  Sampler sampler;
  std::shared_ptr<IntersectorStream<streamSize>> intersector;
  const Camera* camera;
  FrameBuffer* frameBuffer;
  Array<std::shared_ptr<State>> states;
  int aoSpp;
  int sampleIndex;
  bool isStatic;

public:
  AoRendererStream(const std::shared_ptr<const Scene>& scene, const std::shared_ptr<IntersectorStream<streamSize>>& intersector, const Props& props)
    : Renderer(scene, props),
      IntegratorBase(props)
  {
    this->intersector = intersector;

    aoSpp = props.get("samples", 16);

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
      throw std::invalid_argument("AoRendererStream: framebuffer size mismatch");

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
    return sampleDimBaseSize + 2 * aoSpp;
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
      //Vec2vi pixel;
      //pixel.x = load(mortonTableX + i) + tileLow.x;
      //pixel.y = load(mortonTableY + i) + tileLow.y;
      Vec2vi pixel = getMorton8x8Step<tileSizeX>(i) + Vec2vi(tileLow);
      vint pixelId = pixel.x | (pixel.y << 16);
      state->pixelId.setA(i, pixelId);

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

      state->ray.setA(i, ray);
    }

    intersector->intersect(state->ray, state->hit, rayCount, state->rayStats, RayHint::Coherent);

    // Sort
    int missCount = rayStreamSort(state->ray, state->pathId.get(), rayCount);

    // No hits
    for (int i = 0; i < missCount; i += simdSize)
    {
      vbool m = (vint(step) + i) < missCount;
      vint pathId = state->pathId.getA(i);

      vint pixelId = state->pixelId.get(m, pathId);
      Vec2vi pixel(pixelId & 0xffff, shr(pixelId, 16));
      const vfloat color = zero;

      if (accum)
        frameBuffer->getColor().add(m, pixel, Vec3vf(color));
      else
        frameBuffer->getColor().set(m, pixel, Vec3vf(color));
    }

    // Hits
    for (int i = missCount, o = 0; i < rayCount; i += simdSize, o += simdSize)
    {
      vbool m = (vint(step) + i) < rayCount;
      vint pathId = state->pathId.get(i);

      RaySimd ray;
      HitSimd hit;
      state->ray.get(m, pathId, ray);
      state->hit.get(m, pathId, hit);

      ShadingContextT ctx;
      scene->postIntersect(m, ray, hit, ctx);
      Basis3vf frame = ctx.getFrame();

      state->p.setA(o, ctx.p);
      state->U.setA(o, frame.U);
      state->V.setA(o, frame.V);
      state->N.setA(o, frame.N);
      state->eps.setA(o, ctx.eps);
      state->throughput.setA(o, zero);
    }

    for (int k = 0; k < aoSpp; ++k)
    {
      for (int i = missCount, o = 0; i < rayCount; i += simdSize, o += simdSize)
      {
        vbool m = (vint(step) + i) < rayCount;
        vint pathId = state->pathId.get(i);

        RaySimd ray;
        Vec3vf p = state->p.getA(o);
        Basis3vf frame(state->U.getA(o), state->V.getA(o), state->N.getA(o));
        vfloat eps = state->eps.getA(o);

        vint pixelId = state->pixelId.get(m, pathId);
        sampler.resetSample(m, state->sampler, sampleIndex, pixelId);
        Vec2vf s = sampler.get2D(m, state->sampler, sampleDimBaseSize + 2 * k);
        ray.init(p, frame * cosineSampleHemisphere(s), eps);
        state->ray.setA(o, ray);
      }

      intersector->occluded(state->ray, rayCount - missCount, state->rayStats);

      for (int i = missCount, o = 0; i < rayCount; i += simdSize, o += simdSize)
      {
        vbool mHit = state->ray.far.getA(o) == 0.f;
        vfloat throughput = state->throughput.getA(o) + select(mHit, vfloat(0.f), vfloat(1.f));
        state->throughput.setA(o, throughput);
      }
    }

    // Final pass
    for (int i = missCount, o = 0; i < rayCount; i += simdSize, o += simdSize)
    {
      vbool m = (vint(step) + i) < rayCount;
      vint pathId = state->pathId.get(i);
      vint pixelId = state->pixelId.get(m, pathId);
      Vec2vi pixel(pixelId & 0xffff, shr(pixelId, 16));
      vfloat color = state->throughput.getA(o) * rcp(float(aoSpp));

      if (accum)
        frameBuffer->getColor().add(m, pixel, Vec3vf(color));
      else
        frameBuffer->getColor().set(m, pixel, Vec3vf(color));
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
