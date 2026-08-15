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
  float nearClipScale; // nearClip / focalDistance

public:
  ThinLensCamera(const Props& props)
    : PinholeCamera(props)
  {
    lensRadius = props.get("lensRadius", 0.0f);
    focalDistance = props.get("focalDistance", 1.0f);
    nearClipScale = nearClip / focalDistance;
  }

  // The lens is coplanar with the camera origin, and the unnormalized direction has a viewing
  // direction component of exactly focalDistance, thus the ray reaches the near plane at
  // nearClipScale * dirLength
  prt_inline void getRay(Ray& ray, const CameraSample& s) const
  {
    Vec2f lens = uniformSampleDisk(s.lens) * lensRadius;
    Vec3f lensOffset = basis * Vec3f(lens.x, lens.y, 0.0f);
    Vec3f dir = focalDistance * (imageO + s.image.x * imageDx + s.image.y * imageDy) - lensOffset;
    float dirLength = length(dir);
    ray.init(origin + lensOffset, dir * rcp(dirLength), nearClipScale * dirLength);
  }

  prt_inline void getRay(RaySimd& ray, const CameraSampleSimd& s) const
  {
    Vec2vf lens = uniformSampleDisk(s.lens) * vfloat(lensRadius);
    Vec3vf lensOffset = Basis3vf(basis) * Vec3vf(lens.x, lens.y, 0.0f);
    Vec3vf dir = vfloat(focalDistance) * (Vec3vf(imageO) + s.image.x * Vec3vf(imageDx) + s.image.y * Vec3vf(imageDy)) - lensOffset;
    vfloat dirLength = length(dir);
    ray.init(Vec3vf(origin) + lensOffset, dir * rcp(dirLength), vfloat(nearClipScale) * dirLength);
  }
};

} // namespace prt
