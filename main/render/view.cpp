// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <cstdio>
#include "sys/logging.h"
#include "view.h"

namespace prt {

struct ViewV1
{
  Vec3f pos;
  float pitch;
  float yaw;
  float fovY;
  float radius; // lens radius
  float focus;  // focal distance
};

void saveViewSet(const std::string& filename, const ViewSet& viewSet)
{
  FILE* file = fopen(filename.c_str(), "wb");
  if (file == 0)
  {
    LogError() << "Could not save view set: " << filename;
    return;
  }

  if (fwrite(&viewSet, sizeof(ViewSet), 1, file) != 1)
    LogError() << "Could not save view set: " << filename;

  fclose(file);
}

void loadViewSet(const std::string& filename, ViewSet& viewSet)
{
  FILE* file = fopen(filename.c_str(), "rb");
  if (file == 0)
    return; // silent

  Log() << "Loading view set: " << filename;

  // Get file size
  fseek(file, 0, SEEK_END);
  size_t fileSize = ftell(file);
  fseek(file, 0, SEEK_SET);

  if (fileSize >= sizeof(ViewSet))
  {
    // Current version
    if (fread(&viewSet, sizeof(ViewSet), 1, file) != 1)
    {
      LogError() << "Could not load view set: " << filename;
      fclose(file);
      return;
    }

    fclose(file);
    return;
  }

  // Legacy version
  for (int i = 0; i < viewSet.size; ++i)
  {
    ViewV1 viewV1;
    if (fread(&viewV1, sizeof(ViewV1), 1, file) != 1)
    {
      LogError() << "Could not load view set: " << filename;
      break;
    }

    viewSet.views[i].pos    = viewV1.pos;
    viewSet.views[i].yaw    = viewV1.yaw;
    viewSet.views[i].pitch  = viewV1.pitch;
    viewSet.views[i].roll   = 0.f; // not saved in V1
    viewSet.views[i].fovY   = viewV1.fovY;
    viewSet.views[i].radius = viewV1.radius;
    viewSet.views[i].focus  = viewV1.focus;

    viewSet.views2[i] = viewSet.views[i];
    viewSet.lightDirs[i]  = Vec3f(qnan);
    viewSet.lightDirs2[i] = Vec3f(qnan);
  }

  fclose(file);
}

void savePoi(const std::string& filename, const Poi& poi)
{
  FILE* file = fopen(filename.c_str(), "at");
  if (file == 0)
  {
    LogError() << "Could not save POI set: " << filename;
    return;
  }

  fprintf(file, "%.6f %.6f %.6f %.6f %.6f %.6f %.6f\n", poi.p.x, poi.p.y, poi.p.z, poi.N.x, poi.N.y, poi.N.z, poi.dist);
  fclose(file);
}

void loadPoiSet(const std::string& filename, Array<Poi>& poiSet)
{
  Log() << "Loading POI set: " << filename;

  FILE* file = fopen(filename.c_str(), "rt");
  if (file == 0)
    return; // silent

  poiSet.clear();
  char line[2048];

  while (!feof(file))
  {
    Poi poi;
    poi.N = zero;
    poi.dist = posInf;

    if (!fgets(line, sizeof(line), file))
      break;
    int n = sscanf(line, "%f %f %f %f %f %f %f", &poi.p.x, &poi.p.y, &poi.p.z, &poi.N.x, &poi.N.y, &poi.N.z, &poi.dist);
    if (n == EOF || n == 0)
      break;

    if (n != 3 && n != 6 && n != 7)
    {
      LogError() << "Error loading POI set";
      fclose(file);
      return;
    }

    if (lengthSqr(poi.N) > 0.f)
      poi.N = normalize(poi.N);

    poiSet.pushBack(poi);
  }

  fclose(file);

  Log() << "Loaded " << poiSet.getSize() << " POIs";
}

} // namespace prt

