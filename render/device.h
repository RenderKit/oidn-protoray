// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/props.h"
#include "sys/array.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/mat4.h"
#include "math/box3.h"
#include "math/random.h"
#include "image/surface.h"
#include "core/ray.h"

namespace prt {

struct FrameAovs
{
  Vec3f* hdr = nullptr;
  Vec3f* hdrVar = nullptr;
  Vec3f* hdrHalf = nullptr;
  Vec3f* hdrUnclamp = nullptr;
  Vec3f* hdrUnclampVar = nullptr;
  Vec3f* hdrUnclampHalf = nullptr;
  Vec3f* ldr = nullptr;
  Vec3f* albedo = nullptr;
  Vec3f* albedoVar = nullptr;
  Vec3f* albedoHalf = nullptr;
  Vec3f* albedo1 = nullptr;
  Vec3f* diffuseAlbedo = nullptr;
  Vec3f* diffuseAlbedo1 = nullptr;
  Vec3f* specularAlbedo = nullptr;
  Vec3f* specularAlbedo1 = nullptr;
  Vec3f* normal = nullptr;
  Vec3f* normalVar = nullptr;
  Vec3f* normalHalf = nullptr;
  Vec3f* normal1 = nullptr;
  float* depth = nullptr;
  float* depthVar = nullptr;
  float* depthHalf = nullptr;
  // Vec3f* cameraNormal = nullptr;
  // Vec3f* cameraNormalVar = nullptr;
  // Vec3f* cameraNormalHalf = nullptr;
  float* hwDepth = nullptr;
  float* hwDepthVar = nullptr;
  float* hwDepthHalf = nullptr;
  float* roughness = nullptr;
  float* roughness1 = nullptr;
  float* alpha = nullptr;
  Vec3f* sh[4] = {nullptr, nullptr, nullptr, nullptr};
  Vec2f* motionBack = nullptr;
  Vec2f* motionFore = nullptr;
};

class Device
{
public:
  virtual ~Device() {}

  virtual std::string getInfo() = 0;

  // Scene
  virtual void initScene(const std::string& path, const Props& props) = 0;
  virtual Box3f getSceneBounds() = 0;
  virtual void initLights(const Props& props) = 0;
  virtual void mutateMaterials(Random& rng, bool mutateRegular, bool mutateEmissive) {}

  // Renderer (initialize the scene first)
  virtual void initRenderer(const Props& props, Props& stats) = 0;
  virtual void initSampler(unsigned int seed) = 0;
  virtual void render(Props& stats) = 0;
  virtual Props queryPixel(const Vec2f& point) = 0;
  virtual void queryPixel(const Vec2f& point, Array<Props>& hits) = 0;
  virtual Props queryPixel(int x, int y) = 0;
  virtual void queryPixel(int x, int y, Array<Props>& hits) = 0;
  virtual Props queryRay(const Ray& ray) = 0;
  virtual bool queryOcclusionRay(const Ray& ray) = 0;
  virtual void updateRenderer(const Props& props) {}

  std::string getSamplerType() const { return samplerType; }

  // Camera
  virtual void initCamera(const Props& props) = 0;
  virtual void initCamera(const Props& props, const Props& prevProps, const Props& nextProps) = 0;
  virtual Mat4f getWorldToViewD3D() = 0;
  virtual Mat4f getViewToClipD3D() = 0;
  virtual Mat4f getWorldToRaster() = 0;

  // Jitter
  virtual bool hasJitter() = 0;
  virtual Vec2f getJitter() = 0;
  virtual void setJitter(const Vec2f& jitter) = 0;
  virtual void disableJitter() = 0;

  // Framebuffer
  virtual void initFrame(const Vec2i& size, const Props& props) = 0;
  virtual void initFilter(const Props& props) = 0;
  virtual void initToneMapper(const Props& props) = 0;
  virtual void clearFrame() = 0;
  virtual void blitFrame(Surface& dest) = 0;
  virtual void blitFrame(FrameAovs& dest) = 0;
  virtual void blitFrameLdr(Surface& dest, const float* src, float offset = 0.f, float scale = 1.f) = 0;
  virtual void blitFrameLdr(Surface& dest, const Vec3f* src, float offset = 0.f, float scale = 1.f) = 0;
  virtual void blitFrameHdr(Surface& dest, const Vec3f* src) = 0;
  virtual float getFrameError() = 0;

  std::string getFilterType() const { return filterType; }
  virtual float getFilterWidth() const = 0;

protected:
  std::string samplerType;
  std::string filterType = "box";
};

} // namespace prt
