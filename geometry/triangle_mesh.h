// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/array.h"
#include "sys/string.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"
#include "math/box3.h"
#include "triangle.h"
#include "geometry.h"

namespace prt {

struct TriangleMesh : public Geometry
{
  typedef Triangle Prim;

  static const int blobId = 0x04df8666;

  // Triangles
  Array<Vec3i> indices; // v0 v1 v2

  // Vertices
  int vertexCount {0};
  Array<float, size_t> vertexAttribs;
  int vertexStride {0};          // in floats
  float* normals {nullptr};      // Vec3f
  std::vector<float*> texcoords; // Vec2f

  // Materials
  Array<int> materialIds;
  Array<std::string> materialNames;

  Box3f bounds {empty};

  void alloc(int triangleCount, int vertexCount, bool hasNormals, int numTexcoords, bool hasMaterialIds = true);

  prt_inline int getPrimCount() const { return indices.getSize(); }
  prt_inline int getVertexCount() const { return vertexCount; }

  prt_inline Vec3f getPosition(int i) const { return *(const Vec3f*)(vertexAttribs.getData() + (size_t)i*vertexStride); }
  prt_inline Vec3f getNormal  (int i) const { return *(const Vec3f*)(normals   + (size_t)i*vertexStride); }
  prt_inline Vec2f getTexcoord(int t, int i) const { return *(const Vec2f*)(texcoords[t] + (size_t)i*vertexStride); }

  prt_inline Vec3vf getPosition(vbool m, vint i) const
  {
    Vec3vf r;

    #pragma unroll
    for (int k = 0; k < simdSize; ++k)
    {
      const float* ptr = vertexAttribs.getData() + (size_t)i[k]*vertexStride;
      r.x[k] = ptr[0];
      r.y[k] = ptr[1];
      r.z[k] = ptr[2];
    }

    return r;
  }

  prt_inline Vec3vf getNormal(vbool m, vint i) const
  {
    Vec3vf r;

    #pragma unroll
    for (int k = 0; k < simdSize; ++k)
    {
      const float* ptr = normals + (size_t)i[k]*vertexStride;
      r.x[k] = ptr[0];
      r.y[k] = ptr[1];
      r.z[k] = ptr[2];
    }

    return r;
  }

  prt_inline Vec2vf getTexcoord(vbool m, int t, vint i) const
  {
    Vec2vf r;

    #pragma unroll
    for (int k = 0; k < simdSize; ++k)
    {
      const float* ptr = texcoords[t] + (size_t)i[k]*vertexStride;
      r.x[k] = ptr[0];
      r.y[k] = ptr[1];
    }

    return r;
  }

  prt_inline void setPosition(int i, const Vec3f& v) { *(Vec3f*)(vertexAttribs.getData() + (size_t)i*vertexStride) = v; }
  prt_inline void setNormal  (int i, const Vec3f& v) { *(Vec3f*)(normals   + (size_t)i*vertexStride) = v; }
  prt_inline void setTexcoord(int t, int i, const Vec2f& v) { *(Vec2f*)(texcoords[t] + (size_t)i*vertexStride) = v; }

  prt_inline Triangle getPrim(int i) const
  {
    return Triangle(getPosition(indices[i][0]),
                    getPosition(indices[i][1]),
                    getPosition(indices[i][2]));
  }

  prt_inline Box3f getBounds() override;
  prt_inline const Array<std::string>& getMaterialNames() const { return materialNames; }

  void postIntersect(const Ray& ray, const Hit& hit, ShadingContext& ctx, int level) const override;
  void postIntersect(vbool m, const RaySimd& ray, const HitSimd& hit, ShadingContextSimd& ctx, int level) const override;

  void postIntersect(const Ray& ray, const Hit& hit, SimpleShadingContext& ctx, int level) const override;
  void postIntersect(vbool m, const RaySimd& ray, const HitSimd& hit, SimpleShadingContextSimd& ctx, int level) const override;

  int getMaterialId(const Hit& hit, int level) const override;
  vint getMaterialId(vbool m, const HitSimd& hit, int level) const override;

  std::shared_ptr<Geometry> clone(Affine3f transform = one) const override;
};

Stream& operator >>(Stream& ism, TriangleMesh& mesh);
Stream& operator <<(Stream& osm, const TriangleMesh& mesh);

} // namespace prt
