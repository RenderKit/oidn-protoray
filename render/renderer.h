// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <ctime>
#include "sys/props.h"
#include "sys/logging.h"
#include "math/random.h"
#include "math/hash.h"
#include "camera/camera.h"
#include "image/frame_buffer.h"
#include "core/intersector.h"
#include "scene.h"

namespace prt {

class Renderer
{
protected:
  std::shared_ptr<const Scene> scene;
  Vec2i imageSize;
  int sampleCount;

  virtual void render(const Camera* camera, FrameBuffer* frameBuffer, Props& stats)
  {
    throw std::logic_error("not implemented");
  }

public:
  Renderer(const std::shared_ptr<const Scene>& scene, const Props& props)
  {
    this->scene = scene;
    imageSize = props.get<Vec2i>("imageSize");
    sampleCount = props.get("spp", 0);
  }

  virtual ~Renderer() {}

  virtual void initSampler(unsigned int seed) = 0;
  virtual void render(const Camera* camera, const Camera* prevCamera, const Camera* nextCamera,
            FrameBuffer* frameBuffer, Props& stats)
  {
    render(camera, frameBuffer, stats);
  }

  virtual Props queryRay(const Ray& ray) { return Props(); }
  virtual bool queryOcclusionRay(const Ray& ray) { return false; }

  Props queryPixel(const Camera* camera, const Vec2f& point)
  {
    CameraSample cameraSample;
    cameraSample.lens = zero;

    // Generate a ray through the center of the image plane
    // We need this to compute the depth
    Ray centerRay;
    cameraSample.image = 0.5f;
    camera->getRay(centerRay, cameraSample);

    // Generate a ray through the pixel
    Ray ray;
    cameraSample.image = clamp(point, 0.f, 1.f);
    camera->getRay(ray, cameraSample);

    // Shoot the ray
    Props hit = queryRay(ray);
    if (hit.isEmpty())
      return hit;

    // Fill the query result
    float dist = hit.get<float>("dist");
    hit.set("depth", dist * dot(ray.dir, centerRay.dir));
    return hit;
  }

  Props queryPixel(const Camera* camera, int x, int y)
  {
    const Vec2f point = (Vec2f(x, y) + 0.5f) / toFloat(imageSize);
    return queryPixel(camera, point);
  }

  void queryPixel(const Camera* camera, const Vec2f& point, Array<Props>& hits)
  {
    const int maxHits = 32;

    CameraSample cameraSample;
    cameraSample.lens = zero;

    // Generate a ray through the center of the image plane
    // We need this to compute the depth
    Ray centerRay;
    cameraSample.image = 0.5f;
    camera->getRay(centerRay, cameraSample);

    // Generate a ray through the pixel
    Ray ray;
    cameraSample.image = clamp(point, 0.f, 1.f);
    camera->getRay(ray, cameraSample);

    // Find hits
    for (int i = 0; i < maxHits; ++i)
    {
      Props hit = queryRay(ray);
      if (hit.isEmpty())
        break;

      // Fill the query result
      float dist = hit.get<float>("dist");
      hit.set("depth", dist * dot(ray.dir, centerRay.dir));
      hits.pushBack(hit);

      // Is the hit point transparent?
      Vec3f T = hit.get("T", Vec3f(zero));
      if (reduceMax(T) < 1e-5f)
        break;

      // Find the next hit
      float eps = hit.get<float>("eps");
      ray.init(ray.org, ray.dir, dist + eps*16.f);
    }
  }

  void queryPixel(const Camera* camera, int x, int y, Array<Props>& hits)
  {
    const Vec2f point = (Vec2f(x, y) + 0.5f) / toFloat(imageSize);
    queryPixel(camera, point, hits);
  }

  virtual void update(const Props& props) {}

  virtual std::shared_ptr<Intersector> getIntersector() const = 0;
};

} // namespace prt
