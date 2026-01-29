// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/common.h"
#include "sys/array.h"
#include "math/vec3.h"

namespace prt {

struct View
{
  Vec3f pos    {qnan};
  float yaw    {qnan};
  float pitch  {qnan};
  float roll   {qnan};
  float fovY   {qnan};
  float radius {qnan}; // lens radius
  float focus  {qnan}; // focal distance

  operator bool() const
  {
    return std::isfinite(fovY);
  }
};

struct ViewSet
{
  static const int size = 10;

  View views[size];
  View views2[size];
  Vec3f lightDirs[size];
  Vec3f lightDirs2[size];

  ViewSet()
  {
    for (int i = 0; i < size; ++i)
    {
      lightDirs[i]  = Vec3f(qnan);
      lightDirs2[i] = Vec3f(qnan);
    }
  }
};

struct Poi
{
  Vec3f p;    // position
  Vec3f N;    // normal
  float dist; // distance

  Poi() {}
  Poi(Zero) : p(zero), N(zero), dist(posInf) {}
  Poi(const Vec3f& p, const Vec3f& N, float dist) : p(p), N(N), dist(dist) {}
};

void saveViewSet(const std::string& filename, const ViewSet& viewSet);
void loadViewSet(const std::string& filename, ViewSet& viewSet);

void savePoi(const std::string& filename, const Poi& poi);
void loadPoiSet(const std::string& filename, Array<Poi>& poiSet);

} // namespace prt
