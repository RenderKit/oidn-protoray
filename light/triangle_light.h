// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/array.h"
#include "geometry/triangle_mesh.h"
#include "sampling/shape_sampler.h"
#include "sampling/distribution1d.h"
#include "material/material.h"
#include "light.h"

namespace prt {

class TriangleLight : public AreaLight
{
private:
  struct EmissiveTriangle
  {
    Vec3f v0;
    Vec3f v1;
    Vec3f v2;
    Vec3f normal; // not normalized
    Vec3f color;
  };

  static const int triangleStride = sizeof(EmissiveTriangle)/4;

  Array<EmissiveTriangle> triangles; // emissive triangles
  Distribution1D distribution;       // emissive triangle sampling distributions
  Array<int> triangleMap;            // maps original triangle index to emissive triangle index
  std::shared_ptr<TriangleMesh> mesh;

public:
  TriangleLight(const std::shared_ptr<TriangleMesh>& mesh, const Array<std::shared_ptr<Material>>& materials)
    : mesh(mesh)
  {
    triangleMap.alloc(mesh->getPrimCount());
    update(materials);
  }

  void update(const Array<std::shared_ptr<Material>>& materials) override
  {
    Array<float> weights;
    triangles.clear();

    for (int i = 0; i < mesh->getPrimCount(); ++i)
    {
      const int matId = mesh->materialIds[(mesh->materialIds.getSize() > 1) ? i : 0];
      const Material* mat = materials[matId].get();
      if (mat->getType() & MaterialType::Emissive)
      {
        Triangle tri = mesh->getPrim(i);

        EmissiveTriangle etri;
        etri.v0 = tri.v[0];
        etri.v1 = tri.v[1];
        etri.v2 = tri.v[2];
        etri.normal = tri.getNormal();
        etri.color = mat->getEmissiveColor();

        triangleMap[i] = triangles.pushBack(etri);
        weights.pushBack(length(etri.normal) * luminance(etri.color)); // power
      }
      else
      {
        triangleMap[i] = -1;
      }
    }

    distribution.init(weights.getData(), weights.getSize());
  }

  Vec3f eval(const Vec3f& wi, float& pdf, float dist, const Hit& hit) const override
  {
    int triId = triangleMap[hit.primId];
    float pickProb = distribution.pdf(triId);

    const EmissiveTriangle& tri = triangles[triId];
    float dirDotN = dot(wi, tri.normal);
    if (dirDotN >= 0)
    {
      pdf = zero;
      return zero;
    }

    pdf = 2.0f*dist*dist*rcpSafe(abs(dirDotN)) * pickProb;
    return tri.color;
  }

  Vec3vf eval(vbool m, const Vec3vf& wi, vfloat& pdf, vfloat dist, const HitSimd& hit) const override
  {
    Vec3vf value = zero;
    pdf = zero;

    vint triId = gather(m, triangleMap.getData(), hit.primId);
    vfloat pickProb = distribution.pdf(m, triId);

    Vec3vf normal = gather3(m, triangles[0].normal.getData(), triId * triangleStride);
    vfloat dirDotN = dot(wi, normal);
    m &= dirDotN < 0.0f;
    if (none(m)) return value;

    Vec3vf color = gather3(m, triangles[0].color.getData(), triId * triangleStride);
    set(m, pdf, 2.0f*dist*dist*rcpSafe(abs(dirDotN)) * pickProb);
    set(m, value, color);
    return value;
  }

  Vec3f sample(const ShadingContext& ctx, Vec3f& wi, float& pdf, float& dist, const LightSample& s) const override
  {
    // Pick a triangle to sample
    float pickProb;
    int triId = distribution.sample(pickProb, s.c);

    // Sample the triangle
    const EmissiveTriangle& tri = triangles[triId];
    Vec3f dir = uniformSampleTriangle(s.v, tri.v0, tri.v1, tri.v2) - ctx.p;
    dist = length(dir);
    float dirDotN = dot(dir, tri.normal);
    if (dirDotN >= 0) return zero;

    wi = dir * rcp(dist);

    // pdf = dist^2 / (area*cosTheta)
    pdf = 2.0f*dist*dist*dist*rcpSafe(abs(dirDotN)) * pickProb;
    return tri.color * rcp(pdf);
  }

  Vec3vf sample(vbool m, const ShadingContextSimd& ctx, Vec3vf& wi, vfloat& pdf, vfloat& dist, const LightSampleSimd& s) const override
  {
    Vec3vf weight = zero;
    pdf = zero;

    // Pick a triangle to sample
    vfloat pickProb;
    vint triId = distribution.sample(m, pickProb, s.c);

    // Sample the triangle
    Vec3vf v0 = gather3(m, triangles[0].v0.getData(), triId * triangleStride);
    Vec3vf v1 = gather3(m, triangles[0].v1.getData(), triId * triangleStride);
    Vec3vf v2 = gather3(m, triangles[0].v2.getData(), triId * triangleStride);
    Vec3vf dir = uniformSampleTriangle(s.v, v0, v1, v2) - ctx.p;
    dist = length(dir);
    Vec3vf normal = gather3(m, triangles[0].normal.getData(), triId * triangleStride);
    vfloat dirDotN = dot(dir, normal);
    m &= dirDotN < 0.0f;
    if (none(m)) return weight;

    wi = dir * rcp(dist);

    // pdf = dist^2 / (area*cosTheta)
    Vec3vf color = gather3(m, triangles[0].color.getData(), triId * triangleStride);
    set(m, pdf, 2.0f*dist*dist*dist*rcpSafe(abs(dirDotN)) * pickProb);
    set(m, weight, color * rcp(pdf));
    return weight;
  }

  float getPower() const override
  {
    float sumL = zero;

    for (int i = 0; i < triangles.getSize(); ++i)
    {
      const EmissiveTriangle& tri = triangles[i];
      sumL += length(tri.normal) * luminance(tri.color);
    }

    return float(pi) * sumL;
  }

  prt_inline int getTriangleCount() const
  {
    return triangles.getSize();
  }
};

} // namespace prt
