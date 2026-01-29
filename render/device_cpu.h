// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/memory.h"
#include "math/random.h"
#include "camera/camera.h"
#include "image/frame_buffer.h"
#include "renderer.h"
#include "scene.h"
#include "device.h"

namespace prt {

class DeviceCpu : public Device
{
private:
  std::shared_ptr<Scene> scene;
  std::shared_ptr<Intersector> intersector;
  std::shared_ptr<Renderer> renderer;
  std::shared_ptr<Camera> camera, prevCamera, nextCamera;
  std::shared_ptr<FrameBuffer> frameBuffer;

public:
  DeviceCpu();

  std::string getInfo() override;

  // Scene
  void initScene(const std::string& path, const Props& props) override;
  Box3f getSceneBounds() override;
  void initLights(const Props& props) override;
  void mutateMaterials(Random& rng, bool mutateRegular, bool mutateEmissive) override;

  // Renderer
  void initRenderer(const Props& props, Props& stats) override;
  void initSampler(unsigned int seed) override;
  void render(Props& stats) override;
  Props queryRay(const Ray& ray) override;
  bool queryOcclusionRay(const Ray& ray) override;
  Props queryPixel(const Vec2f& point) override;
  void queryPixel(const Vec2f& point, Array<Props>& hits) override;
  Props queryPixel(int x, int y) override;
  void queryPixel(int x, int y, Array<Props>& hits) override;
  void updateRenderer(const Props& props) override;

  // Camera
  void initCamera(const Props& props) override;
  void initCamera(const Props& props, const Props& prevProps, const Props& nextProps) override;
  Mat4f getWorldToRaster() override;

  // Jitter
  bool hasJitter() override;
  Vec2f getJitter() override;
  void setJitter(const Vec2f& jitter) override;
  void disableJitter() override;

  // Film
  void initFrame(const Vec2i& size, const Props& props) override;
  void initFilter(const Props& props) override;
  void initToneMapper(const Props& props) override;
  void clearFrame() override;
  void blitFrame(Surface& dest) override;
  void blitFrame(FrameAovs& dest) override;
  void blitFrameLdr(Surface& dest, const float* src, float offset, float scale) override;
  void blitFrameLdr(Surface& dest, const Vec3f* src, float offset, float scale) override;
  void blitFrameHdr(Surface& dest, const Vec3f* src) override;
  float getFrameError() override;

  float getFilterWidth() const override
  {
    if (!frameBuffer || !frameBuffer->getFilter())
      return 0.f;
    return frameBuffer->getFilter()->getWidth();
  }

private:
  std::shared_ptr<Camera> makeCamera(const Props& props);
};

} // namespace prt
