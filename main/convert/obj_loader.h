// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/common.h"
#include "sys/array.h"
#include "sys/string.h"
#include "geometry/triangle_mesh.h"
#include "geometry/triangle_soup.h"

namespace prt {

class ObjLoader : Uncopyable
{
private:
  struct ObjVertex
  {
    int p; // position
    int t; // texcoord
    int n; // normal
  };

  struct ObjTriangle
  {
    ObjVertex v[3];
    int matId;
  };

  char* buffer;
  int bufferSize;
  Array<Vec3f> positionList;
  Array<Vec3f> normalList;
  Array<Vec2f> texcoordList;
  Array<ObjTriangle> triangleList;
  Array<std::string> materialNameList;
  Array<ObjVertex> faceVertexList; // temporary
  int currentMaterialId;
  int degenerateTriangleCount;

public:
  ObjLoader();
  ~ObjLoader();

  bool load(const std::string& filename, TriangleSoup& soup, bool compressed = false);

private:
  bool parseObj(const std::string& filename, bool compressed);
  bool parsePosition();
  bool parseTexcoord();
  bool parseNormal();
  bool parseFace();
  bool parseUseMtl();
  int convertIndex(int i, int size);

  bool makeSoup(TriangleSoup& soup);
  void cleanup();
};

} // namespace prt
