// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "math/box3.h"
#include "vertex.h"

namespace prt {

struct Triangle
{
  Vec3f v[3];

  Triangle() {}

  Triangle(const Vec3f& v0, const Vec3f& v1, const Vec3f& v2)
  {
    v[0] = v0;
    v[1] = v1;
    v[2] = v2;
  }

  Box3f getBounds() const
  {
    return Box3f(min(min(v[0], v[1]), v[2]),
                 max(max(v[0], v[1]), v[2]));
  }

  Vec3f getCenter() const
  {
    return (v[0] + v[1] + v[2]) * (1.0f / 3.0f);
  }

  Vec3f getNormal() const
  {
    return cross(v[1] - v[0], v[2] - v[0]);
  }
};

struct FatTriangle
{
  FatVertex v[3];
  int matId;

  Vec3f getCenter() const
  {
    return (v[0].pos + v[1].pos + v[2].pos) * (1.0f / 3.0f);
  }

  Box3f getBounds() const
  {
    return Box3f(min(min(v[0].pos, v[1].pos), v[2].pos),
                 max(max(v[0].pos, v[1].pos), v[2].pos));
  }

  Vec3f getNormal() const
  {
    return cross(v[1].pos - v[0].pos, v[2].pos - v[0].pos);
  }
};

struct FatIndexedTriangle
{
  int v[3];
  int matId;
};

void splitPrim(const Triangle& tri, const Box3f& box, int axis, float pos, Box3f& leftBox, Box3f& rightBox);

} // namespace prt
