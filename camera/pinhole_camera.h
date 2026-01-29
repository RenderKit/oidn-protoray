// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/props.h"
#include "math/vec2.h"
#include "math/basis3.h"
#include "math/affine3.h"
#include "camera.h"

namespace prt {

class PinholeCamera : public Camera
{
public:
  Vec3f imageO;
  Vec3f imageDx;
  Vec3f imageDy;

public:
  PinholeCamera(const Props& props)
  {
    Vec3f position = props.get<Vec3f>("position");
    basis = props.get<Basis3f>("basis");
    nearClip = props.get<float>("nearClip");
    //farClip = props.get<float>("farClip");
    const float fov = props.get<float>("fov");
    const float rasterWidth  = float(props.get<int>("width"));
    const float rasterHeight = float(props.get<int>("height"));
    const float aspectRatio = rasterWidth / rasterHeight;

    const float imageHalfHeight = tan(0.5f*fov);
    const float imageHalfWidth = imageHalfHeight * aspectRatio;

    origin  = position;
    imageO  = basis * Vec3f(-imageHalfWidth, imageHalfHeight, -1.f);
    imageDx = basis * Vec3f(2.f*imageHalfWidth, 0.f, 0.f);
    imageDy = basis * Vec3f(0.f, -2.*imageHalfHeight, 0.f);

    const Mat4f worldToCamera = Affine3f(basis, position).toLocalMat();

    const Mat4f cameraToRaster =
      {rasterWidth / (2.f*imageHalfWidth), 0,    -rasterWidth/2.f,  0,
       0, -rasterHeight / (2.f*imageHalfHeight), -rasterHeight/2.f, 0,
       //0, 0, -farClip / (farClip - nearClip), -farClip * nearClip / (farClip - nearClip),
       0, 0, 0, nearClip, // reverse infinite perspective depth
       0, 0, -1, 0};

    worldToRaster = cameraToRaster * worldToCamera;
  }

  prt_inline void getRay(Ray& ray, const CameraSample& s) const
  {
    Vec3f dir = normalize(imageO + s.image.x * imageDx + s.image.y * imageDy);
    ray.init(origin, dir, nearClip);
  }

  prt_inline void getRay(RaySimd& ray, const CameraSampleSimd& s) const
  {
    Vec3vf dir = normalize(Vec3vf(imageO) + s.image.x * Vec3vf(imageDx) + s.image.y * Vec3vf(imageDy));
    ray.init(origin, dir, nearClip);
  }
};

} // namespace prt
