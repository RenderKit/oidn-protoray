// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/common.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/basis3.h"

namespace prt {

struct Ray
{
  Vec3f org;  // origin
  float near; // near distance
  Vec3f dir;  // direction
  float far;  // hit distance

  prt_inline float* getOrgX() { return &org.x; }
  prt_inline float* getOrgY() { return &org.y; }
  prt_inline float* getOrgZ() { return &org.z; }
  prt_inline float* getDirX() { return &dir.x; }
  prt_inline float* getDirY() { return &dir.y; }
  prt_inline float* getDirZ() { return &dir.z; }
  prt_inline float* getNear() { return &near; }
  prt_inline float* getFar()  { return &far; }

  prt_inline void init(const Vec3f& org, const Vec3f& dir)
  {
    this->org  = org;
    this->dir  = dir;
    this->near = 0.f;
    this->far  = posMax;
  }

  prt_inline void init(const Vec3f& org, const Vec3f& dir, float near)
  {
    this->org  = org;
    this->dir  = dir;
    this->near = near;
    this->far  = posMax;
  }

  prt_inline void init(const Vec3f& org, const Vec3f& dir, float near, float far)
  {
    this->org  = org;
    this->dir  = dir;
    this->near = near;
    this->far  = far;
  }

  prt_inline bool isHit() const
  {
    return far < float(posMax);
  }

  prt_inline Vec3f getHitPoint() const
  {
    return org + far * dir;
  }

  prt_inline Vec3f getHitPoint(float& eps) const
  {
    Vec3f p = getHitPoint();
    eps = max(far, reduceMax(abs(p))) * 0x1.fp-18;
    return p;
  }

  prt_inline bool isOccluded() const
  {
    return far == 0.0f;
  }

  prt_inline bool isNotOccluded() const
  {
    return far > 0.0f;
  }
};

struct Hit
{
  int primId; // primitive ID
  int geomId; // geometry ID
  int instId; // instance ID
  float u;    // barycentric u coordinate
  float v;    // barycentric v coordinate

  prt_inline int*   getPrimId() { return &primId; }
  prt_inline int*   getGeomId() { return &geomId; }
  prt_inline int*   getInstId() { return &instId; }
  prt_inline float* getU()      { return &u; }
  prt_inline float* getV()      { return &v; }

  prt_inline int getInstGeomId(int level) const
  {
    if (level > 0 || instId < 0)
      return geomId;
    return instId;
  }
};

} // namespace prt
