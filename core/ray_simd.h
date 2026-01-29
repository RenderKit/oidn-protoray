// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/common.h"
#include "math/simd.h"
#include "math/vec3.h"
#include "math/basis3.h"

namespace prt {

struct RaySimd
{
  Vec3vf org;  // origin
  vfloat near; // near distance
  Vec3vf dir;  // direction
  vfloat far;  // hit distance

  prt_inline float* getOrgX() { return &org.x[0]; }
  prt_inline float* getOrgY() { return &org.y[0]; }
  prt_inline float* getOrgZ() { return &org.z[0]; }
  prt_inline float* getDirX() { return &dir.x[0]; }
  prt_inline float* getDirY() { return &dir.y[0]; }
  prt_inline float* getDirZ() { return &dir.z[0]; }
  prt_inline float* getNear() { return &near[0]; }
  prt_inline float* getFar()  { return &far[0]; }

  prt_inline void init(const Vec3vf& org, const Vec3vf& dir)
  {
    this->org  = org;
    this->dir  = dir;
    this->near = 0.f;
    this->far  = posMax;
  }

  prt_inline void init(const Vec3vf& org, const Vec3vf& dir, vfloat near)
  {
    this->org  = org;
    this->dir  = dir;
    this->near = near;
    this->far  = posMax;
  }

  prt_inline void init(const Vec3vf& org, const Vec3vf& dir, vfloat near, vfloat far)
  {
    this->org  = org;
    this->dir  = dir;
    this->near = near;
    this->far  = far;
  }

  prt_inline vbool isHit() const
  {
    return far < float(posMax);
  }

  prt_inline Vec3vf getHitPoint() const
  {
    return org + far * dir;
  }

  prt_inline Vec3vf getHitPoint(vfloat& eps) const
  {
    Vec3vf p = getHitPoint();
    eps = max(far, reduceMax(abs(p))) * 0x1.fp-18;
    return p;
  }

  prt_inline vbool isOccluded() const
  {
    return far == zero;
  }

  prt_inline vbool isNotOccluded() const
  {
    return far > zero;
  }
};

struct HitSimd
{
  vint primId; // primitive ID
  vint geomId; // geometry ID
  vint instId; // instance ID
  vfloat u;    // barycentric u coordinate
  vfloat v;    // barycentric v coordinate

  prt_inline int*   getPrimId() { return &primId[0]; }
  prt_inline int*   getGeomId() { return &geomId[0]; }
  prt_inline int*   getInstId() { return &instId[0]; }
  prt_inline float* getU()      { return &u[0]; }
  prt_inline float* getV()      { return &v[0]; }

  prt_inline vint getInstGeomId(int level) const
  {
    if (level > 0)
      return geomId;
    return select(instId < 0, geomId, instId);
  }
};

} // namespace prt
