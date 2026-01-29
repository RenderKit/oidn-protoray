// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/array.h"
#include "sys/tasking.h"
#include "sys/sysinfo.h"
#include "sys/timer.h"
#include "sys/logging.h"
#include "image/pixel.h"
#include "image/morton.h"
#include "renderer_stream.h"
#include "transparent_filter.h"
#include "math/sh.h"

namespace prt {

template <class Sampler, int streamSize, int tileSizeX, int tileSizeY>
class PathRendererStream : public Renderer, IntegratorBase
{
private:
  // Sample dimensions
  enum
  {
    sampleDimBsdfV     = 0,
    sampleDimBsdfC     = 2,
    sampleDimLightV    = 3,
    sampleDimLightC    = 5,
    sampleDimTerminate = 6,

    sampleDimBounceSize = 7
  };

  struct StateIo
  {
    RayStream<streamSize> ray;
    RayStreamChannel<int, streamSize> pixelId; // also contains extension and direct flags
    RayStreamChannel2<float, streamSize> pixelSample;
    RayStreamChannel3<float, streamSize> throughput;
    RayStreamChannel3<float, streamSize> L;
    RayStreamChannel<int, streamSize> mediumId;
    RayStreamChannel<float, streamSize> lastBsdfPdf;
    RayStreamChannel<int, streamSize> lastBsdfType;

    // SH
    RayStreamChannel3<float, streamSize> shL;      // irradiance
    RayStreamChannel<float, streamSize>  shScale;  // irradiance -> radiance scale
    RayStreamChannel3<float, streamSize> shWi;     // incoming direction at the first hit
    RayStreamChannel3<float, streamSize> shDirL;   // direct light radiance
    RayStreamChannel3<float, streamSize> shDirWi;  // incoming direct light direction at the first hit
  };

  struct State
  {
    RayStreamChannel<int, streamSize+simdSize> pathId; // padded for prefetching
    RayStreamMaterialIdSort matIdSort;
    RayStreamChannel<int, streamSize> matId;
    HitStream<streamSize> hit;
    RayStream<streamSize> shadowRay;
    RayStreamChannel3<float, streamSize> shadowT;
    RayStreamChannel3<float, streamSize> dirL;
    RayStreamChannel<float, streamSize> shDirScale; // SH direct light irradiance -> radiance scale
    StateIo io[2];
    typename Sampler::State sampler;
    RayStats rayStats;
  };

  static const int pixelIdMask     = 0x0fff0fff;
  static const int pixelCoordMask  = 0x00000fff;
  static const int sampleIdShift   = 12;
  static const int sampleIdMask    = 0x0000f000;
  static const int pathExtFlag     = 0x80000000; // extend path
  static const int pathDirFlag     = 0x40000000; // do direct sampling
  static const int pathAov1Flag    = 0x20000000; // first-hit AOVs accumulated
  static const int pathAov2Flag    = 0x10000000; // other AOVs accumulated

  static const int mediumIdMask   = 0x0000ffff;
  static const int pathDepthShift = 16;

  Sampler sampler;
  std::shared_ptr<IntersectorStream<streamSize>> intersector;
  std::shared_ptr<IntersectorFilterSimd> filter;
  const Camera* camera;
  const Camera* prevCamera;
  const Camera* nextCamera;
  FrameBuffer* frameBuffer;
  Array<std::shared_ptr<State>> states;
  int sampleIndex;
  int rrDepth;
  float minWeight;
  bool isStatic;

  // Irradiance mode
  bool isIrradianceMode;
  std::shared_ptr<Material> whiteMaterial;

public:
  PathRendererStream(const std::shared_ptr<const Scene>& scene, const std::shared_ptr<IntersectorStream<streamSize>>& intersector, const Props& props)
    : Renderer(scene, props),
      IntegratorBase(props)
  {
    this->intersector = intersector;

    if (scene->hasTransparentMaterials())
      filter = std::make_shared<TransparentShadowFilterSimd>(scene);

    //if (imageSize.x % tileSizeX != 0 || imageSize.y % tileSizeY != 0)
    if (imageSize.x % tileSizeX != 0 || imageSize.y % 8 != 0)
      throw std::logic_error("image size is not divisible by the tile size");
    if (imageSize.x > pixelCoordMask+1 || imageSize.y > pixelCoordMask+1)
      throw std::logic_error("image size is too large");

    maxDepth = props.get("maxDepth", 100);
    rrDepth = props.get("rrDepth", 8);
    minWeight = props.get("minWeight", 1e-4f);

    // Initialize the sampler
    initSampler(0);

    states.alloc(cpuCount);
    for (int i = 0; i < states.getSize(); ++i)
      states[i] = std::make_shared<State>();

    isStatic = props.exists("static");

    Log() << "Path state size: " << sizeof(State) << " bytes";
    Log() << "Tile size: " << tileSizeX << "x" << tileSizeY;

    // Initialize irradiance mode
    isIrradianceMode = props.exists("irradiance");
    if (isIrradianceMode)
    {
      MaterialFactory matFactory("", Props());
      Props whiteProps;
      whiteProps.set("type", "Principled");
      whiteProps.set("baseColor", Vec3f(1.f));
      whiteMaterial = matFactory.make(whiteProps);
    }
  }

  std::shared_ptr<Intersector> getIntersector() const override { return intersector; }

  void initSampler(unsigned int seed) override
  {
    sampler.init(getSampleSize(), sampleCount, seed);
    sampleIndex = 0;
  }

  void render(const Camera* camera, const Camera* prevCamera, const Camera* nextCamera,
        FrameBuffer* frameBuffer, Props& stats) override
  {
    if (frameBuffer->getSize() != imageSize)
      throw std::invalid_argument("RendererPacket: framebuffer size mismatch");

    this->camera = camera;
    this->prevCamera = prevCamera;
    this->nextCamera = nextCamera;
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
  int getSampleSize() const
  {
    return sampleDimBaseSize + sampleDimBounceSize * maxDepth;
  }

  void renderTile(const Vec2i& tileId, int tid)
  {
    Vec2i tileLow = tileId * Vec2i(tileSizeX, tileSizeY);
    int croppedTileSizeY = min(imageSize.y - tileLow.y, tileSizeY);

    int taskId = 0;
    int taskCount = tileSizeX * croppedTileSizeY;
    while (taskId < taskCount)
    {
      // Initialize state pointers
      State* state = states[tid].get();
      StateIo* stateI = &state->io[0];
      StateIo* stateO = &state->io[1];

      // Initialize paths
      int rayCount = min(taskCount - taskId, streamSize);
      for (int i = 0; i < rayCount; i += simdSize)
      {
        Vec2vi pixel;
        vint pixelId;

        // Get the pixel coords and initialize the sampler
        //pixel.x = load(mortonTableX + taskId) + tileLow.x;
        //pixel.y = load(mortonTableY + taskId) + tileLow.y;
        pixel = getMorton8x8Step<tileSizeX>(taskId) + Vec2vi(tileLow);
        pixelId = pixel.x | (pixel.y << 16);

        sampler.setSample(one, state->sampler, sampleIndex, pixelId);

        stateI->pixelId.setA(i, pixelId);

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

        stateI->pixelSample.setA(i, pixelSample);

        // Sample the camera
        CameraSampleSimd cameraSample;
        cameraSample.image = (toFloat(pixel) + pixelSample) * Vec2vf(frameBuffer->getInvSize());
        cameraSample.lens = sampler.get2D(one, state->sampler, sampleDimLens);

        RaySimd ray;
        camera->getRay(ray, cameraSample);

        // Store the rest of the path state
        stateI->ray.setA(i, ray);
        stateI->throughput.setA(i, Vec3vf(1.f));
        stateI->L.setA(i, Vec3vf(zero));
        stateI->mediumId.setA(i, 0);
        stateI->lastBsdfPdf.setA(i, 1.f);
        stateI->lastBsdfType.setA(i, BsdfType::SpecularReflection);

        // Store the SH path state
        if (frameBuffer->getSh(0))
        {
          stateI->shL.setA(i, Vec3vf(zero));
          stateI->shScale.setA(i, vfloat(zero));
          stateI->shWi.setA(i, Vec3vf(zero));
          stateI->shDirL.setA(i, Vec3vf(zero));
          stateI->shDirWi.setA(i, Vec3vf(zero));
        }

        taskId += simdSize;
      }

      int matCount = scene->getMaterialCount();

      // Trace the path stream
      int depth = 0;
      int sampleDim = sampleDimBaseSize;

      for (; ;)
      {
        // Intersect the rays
        RayHint rayHint = (depth == 0) ? RayHint::Coherent : RayHint::Incoherent;
        intersector->intersect(stateI->ray, state->hit, rayCount, state->rayStats, rayHint);

        // Sort the paths by material ID
        state->matIdSort(*scene, stateI->ray, state->hit, state->matId.get(), state->pathId.get(), rayCount);

        // No hits
        int missCount = state->matIdSort.buckets[0];
        for (int i = 0; i < missCount; i += simdSize)
        {
          // Get the active mask
          vbool m = (vint(step) + i) < missCount;

          // Get the path ID
          RayStreamId pathId = RayStreamId::getA(state->pathId, i);

          // Record stats
          prt_profile(state->rayStats.shadeSimdBatchCount++);
          prt_profile(state->rayStats.shadeSimdActiveLaneCount += bitCount(toIntMask(m)));

          // Get the ray direction
          Vec3vf rayDir = stateI->ray.dir.get(m, pathId);

          // Evaluate the environment light
          vfloat lightPdf = zero;
          Vec3vf lightValue = (!isIrradianceMode || depth > 0) ? scene->evalEnvLight(m, rayDir, lightPdf) : zero;

          // Get the path state for computing the radiance
          vfloat lastBsdfPdf = stateI->lastBsdfPdf.get(m, pathId);
          vint lastBsdfType = stateI->lastBsdfType.get(m, pathId);
          Vec3vf throughput = stateI->throughput.get(m, pathId);

          // Compute the radiance
          vfloat weight = select((lastBsdfType & BsdfType::Smooth) == 0, vfloat(1.f), misWeight(lastBsdfPdf, lightPdf));
          Vec3vf lightL = throughput * weight * lightValue;

          // Get the pixel state
          vint pixelId = stateI->pixelId.get(m, pathId);
          Vec2vi pixel(pixelId & pixelCoordMask, (pixelId >> 16) & pixelCoordMask);
          Vec2vf pixelSample = stateI->pixelSample.get(m, pathId);

          // Accumulate the color to the framebuffer
          Vec3vf L = stateI->L.get(m, pathId);
          L += lightL;
          addSample(m, frameBuffer->getColor(), pixel, clampL(L), pixelSample);

          // Accumulate the unclamped color
          if (frameBuffer->getColorUnclamp())
            addSample(m, frameBuffer->getColorUnclamp(), pixel, clampL(L, false), pixelSample);

          // Accumulate the SH radiance
          if (frameBuffer->getSh(0))
          {
            Vec3vf shL = stateI->shL.get(m, pathId);
            vfloat shScale = stateI->shScale.get(m, pathId);
            Vec3vf shWi = stateI->shWi.get(m, pathId);
            Vec3vf shDirL = stateI->shDirL.get(m, pathId);
            Vec3vf shDirWi = stateI->shDirWi.get(m, pathId);

            shL += lightL;

            SHL1<Vec3vf> sh = evalSHL1(shWi, clampL(shL * shScale));
            sh += evalSHL1(shDirWi, clampL(shDirL));

            addSampleSh(m, pixel, sh, pixelSample);
          }

          // Accumulate first-hit AOVs if not stored yet
          vbool mAov1 = m & ((pixelId & pathAov1Flag) == 0);
          if (UNLIKELY(any(mAov1)))
          {
            // Accumulate the first-hit albedo
            if (frameBuffer->getAlbedo1())
              addSample(mAov1, frameBuffer->getAlbedo1(), pixel, Vec3vf(zero), pixelSample);

            // Accumulate the first-hit diffuse albedo
            if (frameBuffer->getDiffuseAlbedo1())
              addSample(mAov1, frameBuffer->getDiffuseAlbedo1(), pixel, Vec3vf(zero), pixelSample);

            // Accumulate the first-hit specular albedo
            if (frameBuffer->getSpecularAlbedo1())
              addSample(mAov1, frameBuffer->getSpecularAlbedo1(), pixel, Vec3vf(zero), pixelSample);

            // Accumulate the first-hit world-space normal
            if (frameBuffer->getNormal1())
              addSample(mAov1, frameBuffer->getNormal1(), pixel, Vec3vf(zero), pixelSample);

            // Accumulate the first-hit roughness
            if (frameBuffer->getRoughness1())
              addSample(mAov1, frameBuffer->getRoughness1(), pixel, vfloat(zero), pixelSample);

            // Accumulate the depth
            if (frameBuffer->getDepth())
              addSample(mAov1, frameBuffer->getDepth(), pixel, zero, pixelSample);

            // Accumulate the HW depth
            if (frameBuffer->getHWDepth())
              addSample(mAov1, frameBuffer->getHWDepth(), pixel, zero, pixelSample);

            // Accumulate the alpha
            if (frameBuffer->getAlpha())
              addSample(mAov1, frameBuffer->getAlpha(), pixel, zero, pixelSample);

            // Accumulate the backward motion vector
            if (frameBuffer->getMotionBack())
              addSample(mAov1, frameBuffer->getMotionBack(), pixel, Vec2vf(zero), pixelSample);

            // Accumulate the forward motion vector
            if (frameBuffer->getMotionFore())
              addSample(mAov1, frameBuffer->getMotionFore(), pixel, Vec2vf(zero), pixelSample);
          }

          // Accumulate other AOVs if not stored yet
          vbool mAov2 = m & ((pixelId & pathAov2Flag) == 0);
          if (UNLIKELY(any(mAov2)))
          {
            // Accumulate the albedo
            if (frameBuffer->getAlbedo())
              addSample(mAov2, frameBuffer->getAlbedo(), pixel, Vec3vf(zero), pixelSample);

            // Accumulate the diffuse albedo
            if (frameBuffer->getDiffuseAlbedo())
              addSample(mAov2, frameBuffer->getDiffuseAlbedo(), pixel, Vec3vf(zero), pixelSample);

            // Accumulate the specular albedo
            if (frameBuffer->getSpecularAlbedo())
              addSample(mAov2, frameBuffer->getSpecularAlbedo(), pixel, Vec3vf(zero), pixelSample);

            // Accumulate the world-space normal
            if (frameBuffer->getNormal())
              addSample(mAov2, frameBuffer->getNormal(), pixel, Vec3vf(zero), pixelSample);

            // Accumulate the roughness
            if (frameBuffer->getRoughness())
              addSample(mAov2, frameBuffer->getRoughness(), pixel, vfloat(zero), pixelSample);
          }
        }

        //if (rayCount == missCount) break;

        // Area light hits
        int lightHitCount = state->matIdSort.buckets[scene->lightMaterialCount];
        for (int i = missCount; i < lightHitCount; i += simdSize)
        {
          // Get the active mask
          vbool m = (vint(step) + i) < lightHitCount;

          // Get the path ID
          RayStreamId pathId = RayStreamId::get(state->pathId, i);

          // Record stats
          prt_profile(state->rayStats.shadeSimdBatchCount++);
          prt_profile(state->rayStats.shadeSimdActiveLaneCount += bitCount(toIntMask(m)));

          // Get the ray and hit
          Vec3vf rayDir = stateI->ray.dir.get(m, pathId);
          vfloat rayFar = stateI->ray.far.get(m, pathId);
          HitSimd hit;
          state->hit.get(m, pathId, hit);

          // Evaluate the light
          vfloat lightPdf;
          Vec3vf lightValue = scene->evalAreaLight(m, rayDir, lightPdf, rayFar, hit);

          // Get the path state for computing the radiance
          vfloat lastBsdfPdf = stateI->lastBsdfPdf.get(m, pathId);
          vint lastBsdfType = stateI->lastBsdfType.get(m, pathId);
          Vec3vf throughput = stateI->throughput.get(m, pathId);

          // Compute and add the radiance
          vfloat weight = select((lastBsdfType & BsdfType::Smooth) == 0, vfloat(1.f), misWeight(lastBsdfPdf, lightPdf));
          Vec3vf lightL = throughput * weight * lightValue;

          // Get the pixel state
          vint pixelId = stateI->pixelId.get(m, pathId);
          Vec2vi pixel(pixelId & pixelCoordMask, (pixelId >> 16) & pixelCoordMask);
          Vec2vf pixelSample = stateI->pixelSample.get(m, pathId);

          // Accumulate the color to the framebuffer
          Vec3vf L = stateI->L.get(m, pathId);
          L += lightL;
          addSample(m, frameBuffer->getColor(), pixel, clampL(L), pixelSample);

          // Accumulate the unclamped color
          if (frameBuffer->getColorUnclamp())
            addSample(m, frameBuffer->getColorUnclamp(), pixel, clampL(L, false), pixelSample);

          // Accumulate the SH radiance
          if (frameBuffer->getSh(0))
          {
            Vec3vf shL = stateI->shL.get(m, pathId);
            vfloat shScale = stateI->shScale.get(m, pathId);
            Vec3vf shWi = stateI->shWi.get(m, pathId);
            Vec3vf shDirL = stateI->shDirL.get(m, pathId);
            Vec3vf shDirWi = stateI->shDirWi.get(m, pathId);

            shL += lightL;

            SHL1<Vec3vf> sh = evalSHL1(shWi, clampL(shL * shScale));
            sh += evalSHL1(shDirWi, clampL(shDirL));

            addSampleSh(m, pixel, sh, pixelSample);
          }

          // Accumulate first-hit AOVs if not stored yet
          vbool mAov1 = m & ((pixelId & pathAov1Flag) == 0);
          if (UNLIKELY(any(mAov1)))
          {
            // Accumulate the first-hit albedo
            if (frameBuffer->getAlbedo1())
              addSample(mAov1, frameBuffer->getAlbedo1(), pixel, Vec3vf(1.f), pixelSample);

            // Accumulate the first-hit diffuse albedo
            if (frameBuffer->getDiffuseAlbedo1())
              addSample(mAov1, frameBuffer->getDiffuseAlbedo1(), pixel, Vec3vf(1.f), pixelSample);

            // Accumulate the first-hit specular albedo
            if (frameBuffer->getSpecularAlbedo1())
              addSample(mAov1, frameBuffer->getSpecularAlbedo1(), pixel, Vec3vf(zero), pixelSample);

            // Accumulate the first-hit world-space normal
            if (frameBuffer->getNormal1())
            {
              Vec3vf N = normalize(-rayDir); // FIXME
              addSample(mAov1, frameBuffer->getNormal1(), pixel, N, pixelSample);
            }

            // Accumulate the first-hit roughness
            if (frameBuffer->getRoughness1())
              addSample(mAov1, frameBuffer->getRoughness1(), pixel, vfloat(1.f), pixelSample);

            // Get the hit point
            Vec3vf rayOrg = stateI->ray.org.get(m, pathId);
            Vec3vf p = rayOrg + rayDir * rayFar;

            // Project the hit point to raster space
            Vec4vf pRaster4D = xfmPoint(camera->worldToRaster, p);
            Vec3vf pRaster = projPoint(pRaster4D);

            // Accumulate the linear depth
            if (frameBuffer->getDepth())
            {
              vfloat depth = pRaster4D.w;
              addSample(mAov1, frameBuffer->getDepth(), pixel, depth, pixelSample);
            }

            // Accumulate the HW depth
            if (frameBuffer->getHWDepth())
            {
              vfloat depth = pRaster.z;
              addSample(mAov1, frameBuffer->getHWDepth(), pixel, depth, pixelSample);
            }

            // Accumulate the alpha
            if (frameBuffer->getAlpha())
              addSample(mAov1, frameBuffer->getAlpha(), pixel, vfloat(1.f), pixelSample);

            // Accumulate the backward motion vector
            if (frameBuffer->getMotionBack())
            {
              Vec3vf pRasterPrev = projPoint(prevCamera->worldToRaster, p);
              Vec2vf motion = pRasterPrev.xy() - pRaster.xy();
              addSample(mAov1, frameBuffer->getMotionBack(), pixel, motion, pixelSample);
            }

            // Accumulate the forward motion vector
            if (frameBuffer->getMotionFore())
            {
              Vec3vf pRasterNext = projPoint(nextCamera->worldToRaster, p);
              Vec2vf motion = pRasterNext.xy() - pRaster.xy();
              addSample(mAov1, frameBuffer->getMotionFore(), pixel, motion, pixelSample);
            }
          }

          // Accumulate other AOVs if not stored yet
          vbool mAov2 = m & ((pixelId & pathAov2Flag) == 0);
          if (UNLIKELY(any(mAov2)))
          {
            // Accumulate the albedo
            if (frameBuffer->getAlbedo())
              addSample(mAov2, frameBuffer->getAlbedo(), pixel, Vec3vf(1.f), pixelSample);

            // Accumulate the diffuse albedo
            if (frameBuffer->getDiffuseAlbedo())
              addSample(mAov2, frameBuffer->getDiffuseAlbedo(), pixel, Vec3vf(1.f), pixelSample);

            // Accumulate the specular albedo
            if (frameBuffer->getSpecularAlbedo())
              addSample(mAov2, frameBuffer->getSpecularAlbedo(), pixel, Vec3vf(zero), pixelSample);

            // Accumulate the world-space normal
            if (frameBuffer->getNormal())
            {
              Vec3vf N = normalize(-rayDir); // FIXME
              addSample(mAov2, frameBuffer->getNormal(), pixel, N, pixelSample);
            }

            // Accumulate the roughness
            if (frameBuffer->getRoughness())
              addSample(mAov2, frameBuffer->getRoughness(), pixel, vfloat(1.f), pixelSample);
          }
        }

        if (rayCount == lightHitCount) break;

        // Hits
        int i = lightHitCount;
        int o = 0;
        int oExt = 0;
        int oDir = 0;

        while (i < rayCount)
        {
          // Get the material
          int matId = state->matId[state->pathId[i]];
          int matEnd = state->matIdSort.buckets[matId];

          const Material* mat = (!isIrradianceMode || depth > 0) ? scene->getMaterial(matId) : whiteMaterial.get();
          int matType = mat->getType();

          // Iterate over the paths hitting the same material
          while (i < matEnd)
          {
            // Get the active mask
            vbool m = (vint(step) + i) < matEnd;

            // Get the path ID
            RayStreamId pathId = RayStreamId::get(state->pathId, i);

            // Get the pixel state
            vint pixelId = stateI->pixelId.get(m, pathId);
            Vec2vi pixel(pixelId & pixelCoordMask, (pixelId >> 16) & pixelCoordMask);
            Vec2vf pixelSample = stateI->pixelSample.get(m, pathId);

            if (depth < maxDepth)
            {
              // Get the medium state
              vint mediumId = stateI->mediumId.get(m, pathId);
              mediumId &= mediumIdMask;

              // Record stats
              prt_profile(state->rayStats.shadeSimdBatchCount++);
              prt_profile(state->rayStats.shadeSimdActiveLaneCount += bitCount(toIntMask(m)));

              // Get the ray and hit
              RaySimd ray;
              HitSimd hit;
              stateI->ray.get(m, pathId, ray);
              state->hit.get(m, pathId, hit);

              // Get the shading context for the hit
              ShadingContextSimd ctx;
              scene->postIntersect(m, ray, hit, ctx);

              // Get the BSDF
              Vec3vf wo = -ray.dir;
              const BsdfSimd* bsdf = mat->getBsdf(m, ctx, wo, mediumId);
              vint bsdfFlags = bsdf->getFlags();

              // Get the sampler state
              vint sampleId = (pixelId & sampleIdMask) >> sampleIdShift;
              sampler.resetSample(m, state->sampler, sampleIndex + sampleId, pixelId & pixelIdMask);

              // Get the path throughput
              Vec3vf throughput = stateI->throughput.get(m, pathId);

              // Get the AOV state
              vbool mAov1 = m & ((pixelId & pathAov1Flag) == 0);
              vbool mAov2 = m & ((pixelId & pathAov2Flag) == 0);
              vbool mRR = andn(m, mAov1 | mAov2); // disable roulette if AOVs have not been written yet

              // Apply volume transmission
              if (any(m & (mediumId > 0)))
              {
                Vec3vf mediumColor = scene->getMedia().getColor(m, mediumId);
                throughput *= pow(mediumColor, ray.far);
              }

              // Direct light
              vbool mDir = m & ((bsdfFlags & BsdfType::Smooth) != 0);
              if (LIKELY(any(mDir)))
              {
                // Sample a light
                Vec3vf dirWi;
                vfloat dirLightPdf;
                vfloat dirDist;
                LightSampleSimd lightSample(sampler.get3D(mDir, state->sampler, sampleDim+sampleDimLightV));
                Vec3vf dirLightWeight = scene->sampleLight(mDir, ctx, dirWi, dirLightPdf, dirDist, lightSample);

                // Continue only if the sampling was successful
                mDir &= reduceMax(dirLightWeight) > 0.f;
                mDir &= dirLightPdf > 0.f;

                // Evaluate the BSDF
                if (LIKELY(any(mDir)))
                {
                  // Evaluate the BSDF
                  vfloat dirBsdfPdf;
                  Vec3vf dirBsdfValue = bsdf->eval(mDir, ctx, wo, dirWi, dirBsdfPdf);
                  mDir &= reduceMax(dirBsdfValue) > 0.f;

                  if (LIKELY(any(mDir)))
                  {
                    // Initialize and store the shadow ray
                    RaySimd shadowRay;
                    shadowRay.init(ctx.p, dirWi, ctx.eps, dirDist-ctx.eps);
                    state->shadowRay.packSet(mDir, oDir, shadowRay);
                    state->shadowT.packSet(mDir, oDir, vfloat(1.f));

                    // Compute the direct light throughput
                    Vec3vf dirThroughput = throughput * dirBsdfValue;

                    // Russian roulette adjustment
                    if (depth >= rrDepth)
                    {
                      vfloat contProb = select(mRR, min(luminance(dirThroughput*rcp(dirBsdfPdf)), vfloat(0.95f)), 1.f);
                      dirBsdfPdf *= contProb;
                    }

                    // Compute and store direct light radiance (will be later added if shadow ray is unoccluded)
                    vfloat weight = misWeight(dirLightPdf, dirBsdfPdf);
                    Vec3vf dirL = dirThroughput * weight * dirLightWeight;
                    state->dirL.packSet(mDir, oDir, dirL);

                    // Compute and store SH direct light irradiance -> radiance scale
                    if (frameBuffer->getSh(0) && depth == 0)
                    {
                      vfloat shDirScale = vfloat(float(pi)) / max(dot(dirWi, bsdf->getNormal(mDir)), 0.f);
                      state->shDirScale.packSet(mDir, oDir, shDirScale);
                    }

                    // Set direct light flag
                    set(mDir, pixelId, pixelId | pathDirFlag);

                    oDir += bitCount(toIntMask(mDir));
                  }
                }
              }

              // Sample the BSDF
              vfloat bsdfPdf;
              Vec3vf bsdfWi;
              vint bsdfType;
              BsdfSampleSimd bsdfSample(sampler.get3D(m, state->sampler, sampleDim+sampleDimBsdfV));
              Vec3vf bsdfWeight = bsdf->sample(m, ctx, wo, bsdfWi, bsdfPdf, bsdfType, bsdfSample);

              // Extend the path only if sampling the BSDF was successful
              vbool mExt = m;
              mExt &= reduceMax(bsdfWeight) > 0.f;
              mExt &= bsdfPdf > 0.f;

              // Accumulate first-hit AOVs to the framebuffer
              const vint extType = select(mExt, bsdfType, 0);

              // Store first-hit AOVs if not a cutout transparency ray was sampled
              if (matType & MaterialType::Cutout)
                mAov1 &= (extType != BsdfType::Transparency);

              if (UNLIKELY(any(mAov1)))
              {
                // Accumulate the first-hit albedo
                if (frameBuffer->getAlbedo1())
                {
                  Vec3vf albedo = bsdf->getAlbedo(mAov1);
                  addSample(mAov1, frameBuffer->getAlbedo1(), pixel, albedo, pixelSample);
                }

                // Accumulate the first-hit diffuse albedo
                if (frameBuffer->getDiffuseAlbedo1())
                {
                  Vec3vf diffuseAlbedo = bsdf->getDiffuseAlbedo(mAov1);
                  addSample(mAov1, frameBuffer->getDiffuseAlbedo1(), pixel, diffuseAlbedo, pixelSample);
                }

                // Accumulate the first-hit specular albedo
                if (frameBuffer->getSpecularAlbedo1())
                {
                  Vec3vf specularAlbedo = bsdf->getSpecularAlbedo(mAov1, ctx, wo);
                  addSample(mAov1, frameBuffer->getSpecularAlbedo1(), pixel, specularAlbedo, pixelSample);
                }

                // Accumulate the first-hit world-space normal
                if (frameBuffer->getNormal1())
                {
                  //Vec3vf N = normalize(ctx.f.N);
                  Vec3vf N = bsdf->getNormal(mAov1);
                  addSample(mAov1, frameBuffer->getNormal1(), pixel, N, pixelSample);
                }

                // Accumulate the first-hit roughness
                if (frameBuffer->getRoughness1())
                {
                  vfloat roughness = bsdf->getRoughness(mAov1);
                  addSample(mAov1, frameBuffer->getRoughness1(), pixel, roughness, pixelSample);
                }

                // Project the hit point to raster space
                Vec4vf pRaster4D = xfmPoint(camera->worldToRaster, ctx.p);
                Vec3vf pRaster = projPoint(pRaster4D);

                // Accumulate the linear depth
                if (frameBuffer->getDepth())
                {
                  vfloat depth = pRaster4D.w;
                  addSample(mAov1, frameBuffer->getDepth(), pixel, depth, pixelSample);
                }

                // Accumulate the HW depth
                if (frameBuffer->getHWDepth())
                {
                  vfloat depth = pRaster.z;
                  addSample(mAov1, frameBuffer->getHWDepth(), pixel, depth, pixelSample);
                }

                // Accumulate the alpha
                if (frameBuffer->getAlpha())
                  addSample(mAov1, frameBuffer->getAlpha(), pixel, vfloat(1.f), pixelSample);

                // Accumulate the backward motion vector
                if (frameBuffer->getMotionBack())
                {
                  Vec3vf pRasterPrev = projPoint(prevCamera->worldToRaster, ctx.p);
                  Vec2vf motion = pRasterPrev.xy() - pRaster.xy();
                  addSample(mAov1, frameBuffer->getMotionBack(), pixel, motion, pixelSample);
                }

                // Accumulate the forward motion vector
                if (frameBuffer->getMotionFore())
                {
                  Vec3vf pRasterNext = projPoint(nextCamera->worldToRaster, ctx.p);
                  Vec2vf motion = pRasterNext.xy() - pRaster.xy();
                  addSample(mAov1, frameBuffer->getMotionFore(), pixel, motion, pixelSample);
                }

                // Make sure AOVs are written only once by setting the AOV flag
                set(mAov1, pixelId, pixelId | pathAov1Flag);
              }

              // Accumulate other AOVs to the framebuffer
              if ((matType & MaterialType::FirstHitAov) || isIrradianceMode)
              {
                // Store AOVs if not a cutout transparency ray was sampled
                if (matType & MaterialType::Cutout)
                  mAov2 &= (extType != BsdfType::Transparency);
              }
              else
              {
                // Store AOVs if not a delta BSDF was sampled
                mAov2 &= (extType & BsdfType::Delta) == 0;
              }

              if (UNLIKELY(any(mAov2)))
              {
                // Accumulate the albedo
                if (frameBuffer->getAlbedo())
                {
                  Vec3vf albedo = bsdf->getAlbedo(mAov2);
                  addSample(mAov2, frameBuffer->getAlbedo(), pixel, albedo, pixelSample);
                }

                // Accumulate the diffuse albedo
                if (frameBuffer->getDiffuseAlbedo())
                {
                  Vec3vf diffuseAlbedo = bsdf->getDiffuseAlbedo(mAov2);
                  addSample(mAov2, frameBuffer->getDiffuseAlbedo(), pixel, diffuseAlbedo, pixelSample);
                }

                // Accumulate the specular albedo
                if (frameBuffer->getSpecularAlbedo())
                {
                  Vec3vf specularAlbedo = bsdf->getSpecularAlbedo(mAov2, ctx, wo);
                  addSample(mAov2, frameBuffer->getSpecularAlbedo(), pixel, specularAlbedo, pixelSample);
                }

                // Accumulate the world-space normal
                if (frameBuffer->getNormal())
                {
                  //Vec3vf N = normalize(ctx.f.N);
                  Vec3vf N = bsdf->getNormal(mAov2);
                  addSample(mAov2, frameBuffer->getNormal(), pixel, N, pixelSample);
                }

                // Accumulate the roughness
                if (frameBuffer->getRoughness())
                {
                  vfloat roughness = bsdf->getRoughness(mAov2);
                  addSample(mAov2, frameBuffer->getRoughness(), pixel, roughness, pixelSample);
                }

                // Make sure AOVs are written only once by setting the AOV flag
                set(mAov2, pixelId, pixelId | pathAov2Flag);
              }

              // Adjust the path throughput and terminate the path if its below the threshold
              throughput *= bsdfWeight;
              mExt &= reduceMax(throughput) >= minWeight;

              // Russian roulette
              if (depth >= rrDepth)
              {
                vfloat contProb = select(mRR, min(luminance(throughput), vfloat(0.95f)), 1.f);
                mExt &= sampler.get1D(mExt, state->sampler, sampleDim+sampleDimTerminate) <= contProb;
                throughput *= rcp(contProb);
                bsdfPdf *= contProb;
              }

              // Check whether the path is marked for extension
              if (LIKELY(any(mExt)))
              {
                // Initialize the next (scattered) ray
                ray.init(ctx.p, bsdfWi, ctx.eps);
                stateO->ray.packSet(mExt, oExt, ray);

                // Get lastBsdfPdf and lastBsdfType
                vfloat lastBsdfPdf = stateI->lastBsdfPdf.get(m, pathId);
                vint lastBsdfType = stateI->lastBsdfType.get(m, pathId);

                // Update lastBsdfPdf and lastBsdfType if we did NOT sample a continuation ray
                vbool mUpdateBsdf = (bsdfType & BsdfType::Continuation) == 0;
                set(mUpdateBsdf, lastBsdfPdf, bsdfPdf);
                set(mUpdateBsdf, lastBsdfType, bsdfType);
                stateO->lastBsdfPdf.packSet(mExt, oExt, lastBsdfPdf);
                stateO->lastBsdfType.packSet(mExt, oExt, lastBsdfType);

                // Store the throughput
                stateO->throughput.packSet(mExt, oExt, throughput);

                // Update and store the medium
                mat->nextMedium(mExt, ctx, wo, bsdfWi, mediumId);
                stateO->mediumId.packSet(mExt, oExt, mediumId);

                // Update and store some of the SH state
                if (frameBuffer->getSh(0))
                {
                  vfloat shScale;
                  Vec3vf shWi;
                  if (depth == 0)
                  {
                    shScale = vfloat(float(pi)) / max(dot(bsdfWi, bsdf->getNormal(mExt)), 0.f);
                    shWi = bsdfWi;
                  }
                  else
                  {
                    shScale = stateI->shScale.get(m, pathId);
                    shWi = stateI->shWi.get(m, pathId);
                  }
                  stateO->shScale.packSet(mExt, oExt, shScale);
                  stateO->shWi.packSet(mExt, oExt, shWi);
                }

                // Set the path extension flag
                set(mExt, pixelId, pixelId | pathExtFlag);

                oExt += bitCount(toIntMask(mExt));
              }
            }

            // Store pixel ID and flags for all paths (non-extended too)
            stateO->pixelId.set(o, pixelId);

            i += simdSize;
            o += simdSize;
          }

          o -= i - matEnd;
          i = matEnd;
        }

        // Intersect shadow rays
        float* shadowTptr[] = {state->shadowT.x, state->shadowT.y, state->shadowT.z};
        intersector->occluded(state->shadowRay, oDir, state->rayStats, RayHint::Incoherent, filter.get(), shadowTptr);

        // Final pass
        for (i = lightHitCount, o = 0, oExt = 0, oDir = 0; i < rayCount; i += simdSize, o += simdSize)
        {
          // Get the active mask
          vbool m = (vint(step) + i) < rayCount;

          // Get the path ID
          RayStreamId pathId = RayStreamId::get(state->pathId, i);

          // Get the pixel state
          vint pixelId = stateO->pixelId.getA(o);
          Vec2vi pixel(pixelId & pixelCoordMask, (pixelId >> 16) & pixelCoordMask);
          Vec2vf pixelSample = stateI->pixelSample.get(m, pathId);

          // Get the direct light and path extension (termination) flags
          vbool mDir = m & ((pixelId & pathDirFlag) > 0);
          vbool mExt = m & (pixelId < 0); // pathExtFlag
          vbool mTerm = andn(m, mExt);

          // Reset the direct light and path extension (termination) flags
          pixelId &= pixelIdMask | sampleIdMask | pathAov1Flag | pathAov2Flag;

          // Get the radiance
          Vec3vf L = stateI->L.get(m, pathId);

          // Get the SH radiance
          Vec3vf shL;
          Vec3vf shDirL;
          Vec3vf shDirWi;
          if (frameBuffer->getSh(0))
          {
            shL = stateI->shL.get(m, pathId);
            shDirL = stateI->shDirL.get(m, pathId);
            shDirWi = stateI->shDirWi.get(m, pathId);
          }

          // Add direct light
          if (LIKELY(any(mDir)))
          {
            // Check whether the shadow rays are occluded
            vfloat shadowRayFar = state->shadowRay.far.getUnpack(mDir, oDir);
            vbool mDirMiss = mDir & (shadowRayFar > 0.f);

            // Add the previously computed radiance if there is no occlusion
            Vec3vf T = state->shadowT.getUnpack(mDir, oDir);
            Vec3vf dirL = state->dirL.getUnpack(mDir, oDir) * T;

            set(mDirMiss, L, L + dirL);

            // Add SH direct light
            if (frameBuffer->getSh(0))
            {
              if (depth == 0)
              {
                Vec3vf dirWi = state->shadowRay.dir.getUnpack(mDir, oDir);
                vfloat shDirScale = state->shDirScale.getUnpack(mDir, oDir);
                set(mDirMiss, shDirL, dirL * shDirScale);
                set(mDirMiss, shDirWi, dirWi);
              }
              else
              {
                set(mDirMiss, shL, shL + dirL);
              }
            }

            oDir += bitCount(toIntMask(mDir));
          }

          if (any(mTerm))
          {
            // Accumulate the color to the framebuffer
            addSample(mTerm, frameBuffer->getColor(), pixel, clampL(L), pixelSample);

            // Accumulate the unclamped color
            if (frameBuffer->getColorUnclamp())
              addSample(mTerm, frameBuffer->getColorUnclamp(), pixel, clampL(L, false), pixelSample);

            // Accumulate the SH radiance
            if (frameBuffer->getSh(0))
            {
              vfloat shScale = stateI->shScale.get(mTerm, pathId);
              Vec3vf shWi = stateI->shWi.get(mTerm, pathId);

              SHL1<Vec3vf> sh = evalSHL1(shWi, clampL(shL * shScale));
              sh += evalSHL1(shDirWi, clampL(shDirL));

              addSampleSh(mTerm, pixel, sh, pixelSample);
            }
          }

          // Store the state for extended paths
          if (LIKELY(any(mExt)))
          {
            // Store the pixel state
            stateO->pixelId.packSet(mExt, oExt, pixelId);
            stateO->pixelSample.packSet(mExt, oExt, pixelSample);

            // Store the radiance
            stateO->L.packSet(mExt, oExt, L);

            // Store the SH radiance
            if (frameBuffer->getSh(0))
            {
              stateO->shL.packSet(mExt, oExt, shL);
              stateO->shDirL.packSet(mExt, oExt, shDirL);
              stateO->shDirWi.packSet(mExt, oExt, shDirWi);
            }

            oExt += bitCount(toIntMask(mExt));
          }
        }

        rayCount = oExt;
        swap(stateI, stateO);
        depth++;
        sampleDim += sampleDimBounceSize;
      }
    }
  }

private:
  // MIS power heuristic
  static prt_inline vfloat misWeight(vfloat pdfA, vfloat pdfB)
  {
    return sqr(pdfA) * rcpSafe(sqr(pdfA) + sqr(pdfB));
  }

  template <class AccumBufferT, class T>
  prt_inline void addSample(vbool m, AccumBufferT& buffer, const Vec2vi& p, const T& c, const Vec2vf& s)
  {
    if (!frameBuffer->hasJitter() && frameBuffer->getFilter() && frameBuffer->getFilterMode() == PixelFilterMode::Splat)
      frameBuffer->getFilter()->splat(m, buffer, p, c, s);
    else
      buffer.addSerial(m, p, c);
  }

  template <class T>
  prt_inline void addSampleSh(vbool m, const Vec2vi& p, const SHL1<T>& sh, const Vec2vf& s)
  {
    for (int k = 0; k < 4; ++k)
      addSample(m, frameBuffer->getSh(k), p, sh[k], s);
  }

public:
  Props queryRay(const Ray& ray) override
  {
    return RendererStream::queryRay(scene.get(), intersector.get(), ray);
  }

  bool queryOcclusionRay(const Ray& ray) override
  {
    return RendererStream::queryOcclusionRay(scene.get(), intersector.get(), filter.get(), ray);
  }

  void update(const Props& props) override
  {
    updateIntegrator(props);
  }
};

} // namespace prt
