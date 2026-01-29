// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/array.h"
#include "sys/fixed_hash_map.h"
#include "math/box3.h"
#include "triangle_mesh.h"
#include "triangle_soup.h"

namespace prt {

class TriangleMeshBuilder
{
public:
  TriangleMeshBuilder(const TriangleSoup& soup, TriangleMesh& mesh);

private:
  struct Fragment
  {
    Vec3f position;
    int id;

    Fragment() {}
    Fragment(const Vec3f& position, int id) : position(position), id(id) {}
  };

  Array<FatVertex> vertices;
  Array<FatIndexedTriangle> indexedTriangles;

  Array<Fragment> vertexFragments;
  Array<Fragment> triangleFragments;

  Box3f bounds;
  bool hasNormals;
  bool hasTexcoords;
  bool hasMaterials;

  bool isReorderEnabled;

  void build(const TriangleSoup& soup, TriangleMesh& mesh);
  //void buildFast(const Array<FatTriangle>& triangles, TriangleMesh& mesh);

  void constructMeshTopology(const Array<FatTriangle>& triangles);
  void reorderFragments();
  void reorderFragments(Fragment* vertexBegin, Fragment* vertexEnd, Fragment* triangleBegin, Fragment* triangleEnd);
  void storeMesh(TriangleMesh& mesh);
  void cleanup();
};

} // namespace prt
