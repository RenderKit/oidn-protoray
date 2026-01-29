// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ray_stats.h"
#include "ray.h"
#include "ray_simd.h"

namespace prt {

enum class RayHint
{
  Incoherent,
  Coherent,

  None = Incoherent
};

class Intersector
{
public:
  virtual ~Intersector() {}
};

class IntersectorFilter
{
public:
  virtual ~IntersectorFilter() = default;
  virtual bool filter(const Ray& ray, const Hit& hit, void* payload) = 0;
};

class IntersectorFilterSimd
{
public:
  virtual ~IntersectorFilterSimd() = default;
  virtual vbool filter(vbool m, vint rayId, const RaySimd& ray, const HitSimd& hit, void* payload) = 0;
};

} // namespace prt
