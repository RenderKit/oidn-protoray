// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "sys/blob.h"
#include "triangle_mesh.h"

namespace prt {

void TriangleMesh::alloc(int triangleCount, int vertexCount, bool hasNormals, int numTexcoords, bool hasMaterialIds)
{
  // Allocate indices
  indices.alloc(triangleCount);
  materialIds.alloc(hasMaterialIds ? triangleCount : 1);

  // Allocate vertex attribs
  this->vertexCount = vertexCount;
  vertexStride = 3; // positions
  if (hasNormals)
    vertexStride += 3;
  vertexStride += 2 * numTexcoords;

  vertexAttribs.alloc((size_t)vertexCount * vertexStride);

  // Setup vertex layout
  float* vertexAttribPtr = vertexAttribs.getData() + 3;

  if (hasNormals)
  {
    normals = vertexAttribPtr;
    vertexAttribPtr += 3;
  }
  else
  {
    normals = nullptr;
  }

  texcoords.clear();
  for (int i = 0; i < numTexcoords; ++i)
  {
    texcoords.push_back(vertexAttribPtr);
    vertexAttribPtr += 2;
  }
}

void TriangleMesh::postIntersect(const Ray& ray, const Hit& hit, ShadingContext& ctx, int level) const
{
  prefetchL1(&indices[hit.primId]);
  Vec3i tri = indices[hit.primId];

  // Get the barycentric coordinates
  float b1 = hit.u;
  float b2 = hit.v;
  float b0 = 1.f - b1 - b2;

  // Compute the hit point
  ctx.p = ray.getHitPoint(ctx.eps);

  // Compute the normals
  Vec3f p0 = getPosition(tri[0]);
  Vec3f p1 = getPosition(tri[1]);
  Vec3f p2 = getPosition(tri[2]);

  Vec3f dp1 = p1 - p0;
  Vec3f dp2 = p2 - p0;

  ctx.Ng = normalize(cross(dp1, dp2));

  if (normals)
  {
    Vec3f n0 = getNormal(tri[0]);
    Vec3f n1 = getNormal(tri[1]);
    Vec3f n2 = getNormal(tri[2]);

    ctx.f.N = normalize(b0*n0 + b1*n1 + b2*n2);
    if (dot(ctx.f.N, ctx.Ng) < 0.f)
      ctx.f.N = -ctx.f.N;
  }
  else
  {
    ctx.f.N = ctx.Ng;
  }

  // Compute the UVs
  if (texcoords[0])
  {
    Vec2f uv0 = getTexcoord(0, tri[0]);
    Vec2f uv1 = getTexcoord(0, tri[1]);
    Vec2f uv2 = getTexcoord(0, tri[2]);

    ctx.uv[0] = b0*uv0 + b1*uv1 + b2*uv2;

    // Compute partial derivatives
    float du1 = uv1.x - uv0.x;
    float du2 = uv2.x - uv0.x;
    float dv1 = uv1.y - uv0.y;
    float dv2 = uv2.y - uv0.y;

    float det = du1 * dv2 - dv1 * du2;
    if (det != 0.f)
    {
      float invDet = rcp(det);
      Vec3f dpdu = (dv2 * dp1 - dv1 * dp2) * invDet;
      Vec3f dpdv = (du1 * dp2 - du2 * dp1) * invDet;

      // Compute orthonormal frame using modified Gram-Schmidt
      ctx.f.U = dpdu - dot(dpdu, ctx.f.N) * ctx.f.N;
      float Ulen2 = lengthSqr(ctx.f.U);
      if (Ulen2 > 0.f)
      {
        ctx.f.U *= rsqrt(Ulen2); // normalize
        ctx.f.V = dpdv - dot(dpdv, ctx.f.N) * ctx.f.N;
        ctx.f.V -= dot(ctx.f.V, ctx.f.U) * ctx.f.U;
        float Vlen2 = lengthSqr(ctx.f.V);
        if (Vlen2 > 0.f)
          ctx.f.V *= rsqrt(Vlen2); // normalize
        else
          makeFrame(ctx.f.U, ctx.f.V, ctx.f.N);
      }
      else
      {
        makeFrame(ctx.f.U, ctx.f.V, ctx.f.N);
      }
    }
    else
    {
      makeFrame(ctx.f.U, ctx.f.V, ctx.f.N);
    }
  }
  else
  {
    ctx.uv[0] = Vec2f(hit.u, hit.v);
    makeFrame(ctx.f.U, ctx.f.V, ctx.f.N);
  }

  for (int t = 1; t < maxNumUVs; ++t)
  {
    if (t < static_cast<int>(texcoords.size()))
    {
      Vec2f uv0 = getTexcoord(t, tri[0]);
      Vec2f uv1 = getTexcoord(t, tri[1]);
      Vec2f uv2 = getTexcoord(t, tri[2]);

      ctx.uv[t] = b0*uv0 + b1*uv1 + b2*uv2;
    }
    else
    {
      ctx.uv[t] = Vec2f(hit.u, hit.v);
    }
  }
}

void TriangleMesh::postIntersect(vbool m, const RaySimd& ray, const HitSimd& hit, ShadingContextSimd& ctx, int level) const
{
  Vec3vi tri;
  tri[0] = gather(m, &indices[0][0], hit.primId*3);
  tri[1] = gather(m, &indices[0][1], hit.primId*3);
  tri[2] = gather(m, &indices[0][2], hit.primId*3);

  // Get the barycentric coordinates
  vfloat b1 = hit.u;
  vfloat b2 = hit.v;
  vfloat b0 = 1.f - b1 - b2;

  // Compute the hit point
  set(m, ctx.p, ray.getHitPoint(ctx.eps));

  // Compute the normals
  Vec3vf p0 = getPosition(m, tri[0]);
  Vec3vf p1 = getPosition(m, tri[1]);
  Vec3vf p2 = getPosition(m, tri[2]);

  Vec3vf dp1 = p1 - p0;
  Vec3vf dp2 = p2 - p0;

  Vec3vf Ng = normalize(cross(dp1, dp2));
  Vec3vf N;
  if (normals)
  {
    Vec3vf n0 = getNormal(m, tri[0]);
    Vec3vf n1 = getNormal(m, tri[1]);
    Vec3vf n2 = getNormal(m, tri[2]);

    N = normalize(b0*n0 + b1*n1 + b2*n2);
    set(dot(N, Ng) < 0.f, N, -N);
  }
  else
  {
    N = Ng;
  }

  set(m, ctx.Ng, Ng);
  set(m, ctx.f.N, N);

  if (texcoords[0])
  {
    Vec2vf uv0 = getTexcoord(m, 0, tri[0]);
    Vec2vf uv1 = getTexcoord(m, 0, tri[1]);
    Vec2vf uv2 = getTexcoord(m, 0, tri[2]);
    set(m, ctx.uv[0], b0*uv0 + b1*uv1 + b2*uv2);

    // Compute partial derivatives
    vfloat du1 = uv1.x - uv0.x;
    vfloat du2 = uv2.x - uv0.x;
    vfloat dv1 = uv1.y - uv0.y;
    vfloat dv2 = uv2.y - uv0.y;

    vfloat det = du1 * dv2 - dv1 * du2;
    vfloat invDet = rcp(det);

    Vec3vf dpdu = (dv2 * dp1 - dv1 * dp2) * invDet;
    Vec3vf dpdv = (du1 * dp2 - du2 * dp1) * invDet;

    // Compute orthonormal frame using modified Gram-Schmidt
    Vec3vf U = dpdu - dot(dpdu, N) * N;
    vfloat Ulen2 = lengthSqr(U);
    U *= rsqrt(Ulen2); // normalize
    Vec3vf V = dpdv - dot(dpdv, N) * N;
    V -= dot(V, U) * U;
    vfloat Vlen2 = lengthSqr(V);
    V *= rsqrt(Vlen2); // normalize

    vbool mFixFrame = m & ((det == 0.f) | (Ulen2 == 0.f) | Vlen2 == 0.f);
    if (any(mFixFrame))
    {
      Vec3vf U2, V2;
      makeFrame(U2, V2, N);
      set(mFixFrame, U, U2);
      set(mFixFrame, V, V2);
    }

    set(m, ctx.f.U, U);
    set(m, ctx.f.V, V);
  }
  else
  {
    set(m, ctx.uv[0], Vec2vf(hit.u, hit.v));
    Vec3vf U, V;
    makeFrame(U, V, N);
    set(m, ctx.f.U, U);
    set(m, ctx.f.V, V);
  }

  for (int t = 1; t < maxNumUVs; ++t)
  {
    if (t < static_cast<int>(texcoords.size()))
    {
      Vec2vf uv0 = getTexcoord(m, t, tri[0]);
      Vec2vf uv1 = getTexcoord(m, t, tri[1]);
      Vec2vf uv2 = getTexcoord(m, t, tri[2]);
      set(m, ctx.uv[t], b0*uv0 + b1*uv1 + b2*uv2);
    }
    else
    {
      set(m, ctx.uv[t], Vec2vf(hit.u, hit.v));
    }
  }
}

void TriangleMesh::postIntersect(const Ray& ray, const Hit& hit, SimpleShadingContext& ctx, int level) const
{
  prefetchL1(&indices[hit.primId]);
  Vec3i tri = indices[hit.primId];

  // Compute the hit point
  ctx.p = ray.getHitPoint(ctx.eps);

  // Compute the normals
  Vec3f p0 = getPosition(tri[0]);
  Vec3f p1 = getPosition(tri[1]);
  Vec3f p2 = getPosition(tri[2]);

  Vec3f dp1 = p1 - p0;
  Vec3f dp2 = p2 - p0;

  ctx.Ng = normalize(cross(dp1, dp2));
}

void TriangleMesh::postIntersect(vbool m, const RaySimd& ray, const HitSimd& hit, SimpleShadingContextSimd& ctx, int level) const
{
  Vec3vi tri;
  tri[0] = gather(m, &indices[0][0], hit.primId*3);
  tri[1] = gather(m, &indices[0][1], hit.primId*3);
  tri[2] = gather(m, &indices[0][2], hit.primId*3);

  // Compute the hit point
  set(m, ctx.p, ray.getHitPoint(ctx.eps));

  // Compute the normals
  Vec3vf p0 = getPosition(m, tri[0]);
  Vec3vf p1 = getPosition(m, tri[1]);
  Vec3vf p2 = getPosition(m, tri[2]);

  Vec3vf dp1 = p1 - p0;
  Vec3vf dp2 = p2 - p0;

  Vec3vf Ng = normalize(cross(dp1, dp2));
  set(m, ctx.Ng, Ng);
}

prt_inline Box3f TriangleMesh::getBounds()
{
  if (bounds.isValid())
    return bounds;

  bounds = empty;
  for (int i = 0; i < indices.getSize(); ++i)
  {
    Triangle tri = getPrim(i);
    bounds.grow(tri.getBounds());
  }
  return bounds;
}

int TriangleMesh::getMaterialId(const Hit& hit, int level) const
{
  if (materialIds.getSize() == 1)
    return materialIds[0];
  return materialIds[hit.primId];
}

vint TriangleMesh::getMaterialId(vbool m, const HitSimd& hit, int level) const
{
  if (materialIds.getSize() == 1)
    return materialIds[0];
  return gather(m, materialIds.getData(), hit.primId);
}

std::shared_ptr<Geometry> TriangleMesh::clone(Affine3f transform) const
{
  std::shared_ptr<TriangleMesh> newMesh = std::make_shared<TriangleMesh>();
  newMesh->alloc(getPrimCount(), getVertexCount(), normals != nullptr, static_cast<int>(texcoords.size()), materialIds.getSize() > 1);

  // Copy indices
  for (int i = 0; i < getPrimCount(); ++i)
    newMesh->indices[i] = indices[i];

  // Copy/transform vertex positions
  for (int i = 0; i < getVertexCount(); ++i)
    newMesh->setPosition(i, xfmPoint(transform, getPosition(i)));

  // Copy/transform vertex normals
  if (normals != nullptr)
  {
    for (int i = 0; i < getVertexCount(); ++i)
      newMesh->setNormal(i, xfmNormal(transform, getNormal(i)));
  }

  // Copy vertex texcoords
  for (int t = 0; t < static_cast<int>(texcoords.size()); ++t)
  {
    for (int i = 0; i < getVertexCount(); ++i)
      newMesh->setTexcoord(t, i, getTexcoord(t, i));
  }

  // Copy material IDs
  for (int i = 0; i < materialIds.getSize(); ++i)
    newMesh->materialIds[i] = materialIds[i];

  return newMesh;
}

Stream& operator >>(Stream& ism, TriangleMesh& mesh)
{
  ism >> mesh.bounds;

  int triangleCount, vertexCount;
  bool hasNormals;
  int8_t numTexcoords;
  ism >> triangleCount >> vertexCount >> hasNormals >> numTexcoords;
  if (triangleCount < 0 || vertexCount < 0 || numTexcoords < 0)
    throw std::runtime_error("invalid triangle mesh data in stream");

  mesh.alloc(triangleCount, vertexCount, hasNormals, numTexcoords);
  ism.readFull(mesh.indices.getData(), mesh.indices.getSize() * sizeof(Vec3i));
  ism.readFull(mesh.materialIds.getData(), mesh.materialIds.getSize() * sizeof(int));
  ism.readFull(mesh.vertexAttribs.getData(), mesh.vertexAttribs.getSize() * sizeof(float));

  ism >> mesh.materialNames;
  return ism;
}

Stream& operator <<(Stream& osm, const TriangleMesh& mesh)
{
  osm << mesh.bounds;

  int triangleCount = mesh.indices.getSize();
  bool hasNormals = mesh.normals;
  int8_t numTexcoords = static_cast<int8_t>(mesh.texcoords.size());
  osm << triangleCount << mesh.vertexCount << hasNormals << numTexcoords;

  osm.write(mesh.indices.getData(), mesh.indices.getSize() * sizeof(Vec3i));
  osm.write(mesh.materialIds.getData(), mesh.materialIds.getSize() * sizeof(int));
  osm.write(mesh.vertexAttribs.getData(), mesh.vertexAttribs.getSize() * sizeof(float));

  osm << mesh.materialNames;
  return osm;
}

} // namespace prt

