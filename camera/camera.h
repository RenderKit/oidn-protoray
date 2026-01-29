// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "math/vec2.h"
#include "math/mat4.h"
#include "math/basis3.h"
#include "core/ray.h"
#include "core/ray_simd.h"

namespace prt {

struct CameraSample
{
  Vec2f image;
  Vec2f lens;

  prt_inline CameraSample() {}
  prt_inline CameraSample(const Vec2f& image, const Vec2f& lens) : image(image), lens(lens) {}
};

struct CameraSampleSimd
{
  Vec2vf image;
  Vec2vf lens;

  prt_inline CameraSampleSimd() {}
  prt_inline CameraSampleSimd(const Vec2vf& image, const Vec2vf& lens) : image(image), lens(lens) {}
};

class Camera
{
public:
  Vec3f origin;
  Basis3f basis;
  float nearClip;
  //float farClip;
  Mat4f worldToRaster;

  virtual ~Camera() {}

  virtual void getRay(Ray& ray, const CameraSample& s) const = 0;
  virtual void getRay(RaySimd& ray, const CameraSampleSimd& s) const = 0;
};

} // namespace prt
