// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sampling/shape_sampler.h"
#include "pinhole_camera.h"

namespace prt {

class ThinLensCamera : public PinholeCamera
{
public:
  float lensRadius;
  float focalDistance;

public:
  ThinLensCamera(const Props& props)
    : PinholeCamera(props)
  {
    lensRadius = props.get("lensRadius", 0.0f);
    focalDistance = props.get("focalDistance", 1.0f);
  }

  prt_inline void getRay(Ray& ray, const CameraSample& s) const
  {
    Vec2f lens = uniformSampleDisk(s.lens) * lensRadius;
    Vec3f begin = origin + basis * Vec3f(lens.x, lens.y, 0.0f);
    Vec3f end = origin + focalDistance * (imageO + s.image.x * imageDx + s.image.y * imageDy);
    ray.init(begin, normalize(end - begin), nearClip);
  }

  prt_inline void getRay(RaySimd& ray, const CameraSampleSimd& s) const
  {
    Vec2vf lens = uniformSampleDisk(s.lens) * vfloat(lensRadius);
    Vec3vf begin = Vec3vf(origin) + Basis3vf(basis) * Vec3vf(lens.x, lens.y, 0.0f);
    Vec3vf end = Vec3vf(origin) + vfloat(focalDistance) * (Vec3vf(imageO) + s.image.x * Vec3vf(imageDx) + s.image.y * Vec3vf(imageDy));
    ray.init(begin, normalize(end - begin), nearClip);
  }
};

} // namespace prt
