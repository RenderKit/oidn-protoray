// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "math/interpolation.h"

namespace prt {

// Directional and average albedo tables for BSDFs
// [Kulla and Conty, 2017, "Revisiting Physically Based Shading at Imageworks"]

// Microfacet GGX albedo table
struct MicrofacetAlbedoTable
{
  static constexpr int size = 32;
  static float* dir; // directional 2D table (cosThetaO, roughness)
  static float* avg; // average 1D table (roughness)

  static prt_inline float eval(float cosThetaO, float roughness)
  {
    const Vec2f p = Vec2f(cosThetaO, roughness) * float(size-1);
    return interp2DLinear(p, dir, size);
  }

  static prt_inline vfloat eval(vbool m, vfloat cosThetaO, vfloat roughness)
  {
    const Vec2vf p = Vec2vf(cosThetaO, roughness) * vfloat(size-1);
    return interp2DLinear(m, p, dir, size);
  }

  static prt_inline float evalAvg(float roughness)
  {
    const float x = roughness * float(size-1);
    return interp1DLinear(x, avg, size);
  }

  static prt_inline vfloat evalAvg(vbool m, vfloat roughness)
  {
    const vfloat x = roughness * vfloat(size-1);
    return interp1DLinear(m, x, avg, size);
  }

  static void precompute();
};

// Microfacet GGX dielectric albedo table
struct MicrofacetDielectricAlbedoTable
{
  static constexpr int size = 16;
  static constexpr float minIor = 1.f;
  static constexpr float maxIor = 3.f;

  // eta in [1/3, 1]
  static float* etaDir; // directional 3D table (cosThetaO, eta, roughness)
  static float* etaAvg; // average 2D table (eta, roughness)

  // eta in [1, 3]
  static float* rcpEtaDir; // directional 3D table (cosThetaO, eta, roughness)
  static float* rcpEtaAvg; // average 2D table (eta, roughness)

  static prt_inline float eval(float cosThetaO, float eta, float roughness)
  {
    if (eta <= 1.f)
    {
      const float minEta = rcp(maxIor);
      const float maxEta = rcp(minIor);
      const float etaParam = (eta - minEta) / (maxEta - minEta);
      const Vec3f p = Vec3f(cosThetaO, etaParam, roughness) * float(size-1);
      return interp3DLinear(p, etaDir, size);
    }
    else
    {
      const float minEta = minIor;
      const float maxEta = maxIor;
      const float etaParam = (eta - minEta) / (maxEta - minEta);
      const Vec3f p = Vec3f(cosThetaO, etaParam, roughness) * float(size-1);
      return interp3DLinear(p, rcpEtaDir, size);
    }
  }

  static prt_inline vfloat eval(vbool m, vfloat cosThetaO, vfloat eta, vfloat roughness)
  {
    vfloat res = zero;

    vbool mEta = m & (eta <= 1.f);
    if (any(mEta))
    {
      const float minEta = rcp(maxIor);
      const float maxEta = rcp(minIor);
      const vfloat etaParam = (eta - minEta) / (maxEta - minEta);
      const Vec3vf p = Vec3vf(cosThetaO, etaParam, roughness) * vfloat(size-1);
      res = interp3DLinear(mEta, p, etaDir, size);
    }

    vbool mRcpEta = andn(m, mEta);
    if (any(mRcpEta))
    {
      const float minEta = minIor;
      const float maxEta = maxIor;
      const vfloat etaParam = (eta - minEta) / (maxEta - minEta);
      const Vec3vf p = Vec3vf(cosThetaO, etaParam, roughness) * vfloat(size-1);
      set(mRcpEta, res, interp3DLinear(mRcpEta, p, rcpEtaDir, size));
    }

    return res;
  }

  static prt_inline float evalAvg(float eta, float roughness)
  {
    if (eta <= 1.f)
    {
      const float minEta = rcp(maxIor);
      const float maxEta = rcp(minIor);
      const float etaParam = (eta - minEta) / (maxEta - minEta);
      const Vec2f p = Vec2f(etaParam, roughness) * float(size-1);
      return interp2DLinear(p, etaAvg, size);
    }
    else
    {
      const float minEta = minIor;
      const float maxEta = maxIor;
      const float etaParam = (eta - minEta) / (maxEta - minEta);
      const Vec2f p = Vec2f(etaParam, roughness) * float(size-1);
      return interp2DLinear(p, rcpEtaAvg, size);
    }
  }

  static prt_inline vfloat evalAvg(vbool m, vfloat eta, vfloat roughness)
  {
    vfloat res = zero;

    vbool mEta = m & (eta <= 1.f);
    if (any(mEta))
    {
      const float minEta = rcp(maxIor);
      const float maxEta = rcp(minIor);
      const vfloat etaParam = (eta - minEta) / (maxEta - minEta);
      const Vec2vf p = Vec2vf(etaParam, roughness) * vfloat(size-1);
      res = interp2DLinear(mEta, p, etaAvg, size);
    }

    vbool mRcpEta = andn(m, mEta);
    if (any(mRcpEta))
    {
      const float minEta = minIor;
      const float maxEta = maxIor;
      const vfloat etaParam = (eta - minEta) / (maxEta - minEta);
      const Vec2vf p = Vec2vf(etaParam, roughness) * vfloat(size-1);
      set(mRcpEta, res, interp2DLinear(mRcpEta, p, rcpEtaAvg, size));
    }

    return res;
  }

  static void precompute();
};

// Microfacet GGX dielectric reflection-only albedo table
struct MicrofacetDielectricReflectionAlbedoTable
{
  static constexpr int size = 16;
  static constexpr float minIor = 1.f;
  static constexpr float maxIor = 3.f;

  // eta in [1/3, 1]
  static float* etaDir; // directional 3D table (cosThetaO, eta, roughness)
  static float* etaAvg; // average 2D table (eta, roughness)

  static prt_inline float eval(float cosThetaO, float eta, float roughness)
  {
    const float minEta = rcp(maxIor);
    const float maxEta = rcp(minIor);
    const float etaParam = (eta - minEta) / (maxEta - minEta);
    const Vec3f p = Vec3f(cosThetaO, etaParam, roughness) * float(size-1);
    return interp3DLinear(p, etaDir, size);
  }

  static prt_inline vfloat eval(vbool m, vfloat cosThetaO, vfloat eta, vfloat roughness)
  {
    const float minEta = rcp(maxIor);
    const float maxEta = rcp(minIor);
    const vfloat etaParam = (eta - minEta) / (maxEta - minEta);
    const Vec3vf p = Vec3vf(cosThetaO, etaParam, roughness) * vfloat(size-1);
    return interp3DLinear(m, p, etaDir, size);
  }

  static prt_inline float evalAvg(float eta, float roughness)
  {
    const float minEta = rcp(maxIor);
    const float maxEta = rcp(minIor);
    const float etaParam = (eta - minEta) / (maxEta - minEta);
    const Vec2f p = Vec2f(etaParam, roughness) * float(size-1);
    return interp2DLinear(p, etaAvg, size);
  }

  static prt_inline vfloat evalAvg(vbool m, vfloat eta, vfloat roughness)
  {
    const float minEta = rcp(maxIor);
    const float maxEta = rcp(minIor);
    const vfloat etaParam = (eta - minEta) / (maxEta - minEta);
    const Vec2vf p = Vec2vf(etaParam, roughness) * vfloat(size-1);
    return interp2DLinear(m, p, etaAvg, size);
  }

  static void precompute();
};

// Microfacet GGX dielectric reflection-only albedo w/ energy compensation table
struct MicrofacetDielectricReflectionECAlbedoTable
{
  static constexpr int size = 16;
  static constexpr float minIor = 1.f;
  static constexpr float maxIor = 3.f;

  // eta in [1/3, 1]
  static float* etaDir; // directional 3D table (cosThetaO, eta, roughness)
  static float* etaAvg; // average 2D table (eta, roughness)

  static prt_inline float eval(float cosThetaO, float eta, float roughness)
  {
    const float minEta = rcp(maxIor);
    const float maxEta = rcp(minIor);
    const float etaParam = (eta - minEta) / (maxEta - minEta);
    const Vec3f p = Vec3f(cosThetaO, etaParam, roughness) * float(size-1);
    return interp3DLinear(p, etaDir, size);
  }

  static prt_inline vfloat eval(vbool m, vfloat cosThetaO, vfloat eta, vfloat roughness)
  {
    const float minEta = rcp(maxIor);
    const float maxEta = rcp(minIor);
    const vfloat etaParam = (eta - minEta) / (maxEta - minEta);
    const Vec3vf p = Vec3vf(cosThetaO, etaParam, roughness) * vfloat(size-1);
    return interp3DLinear(m, p, etaDir, size);
  }

  static prt_inline float evalAvg(float eta, float roughness)
  {
    const float minEta = rcp(maxIor);
    const float maxEta = rcp(minIor);
    const float etaParam = (eta - minEta) / (maxEta - minEta);
    const Vec2f p = Vec2f(etaParam, roughness) * float(size-1);
    return interp2DLinear(p, etaAvg, size);
  }

  static prt_inline vfloat evalAvg(vbool m, vfloat eta, vfloat roughness)
  {
    const float minEta = rcp(maxIor);
    const float maxEta = rcp(minIor);
    const vfloat etaParam = (eta - minEta) / (maxEta - minEta);
    const Vec2vf p = Vec2vf(etaParam, roughness) * vfloat(size-1);
    return interp2DLinear(m, p, etaAvg, size);
  }

  static void precompute();
};

// Microfacet GGX artistic conductor albedo w/ energy compensation table
struct MicrofacetConductorArtisticECAlbedoTable
{
  static constexpr int size = 16;

  static float* dir; // directional 4D table (cosThetaO, r, g, roughness)

  static prt_inline float eval(float cosThetaO, float r, float g, float roughness)
  {
    const Vec4f p = Vec4f(cosThetaO, r, g, roughness) * float(size-1);
    return interp4DLinear(p, dir, size);
  }

  static prt_inline vfloat eval(vbool m, vfloat cosThetaO, vfloat r, vfloat g, vfloat roughness)
  {
    const Vec4vf p = Vec4vf(cosThetaO, r, g, roughness) * vfloat(size-1);
    return interp4DLinear(m, p, dir, size);
  }

  static void precompute();
};

// Microfacet sheen albedo table
struct MicrofacetSheenAlbedoTable
{
  static constexpr int size = 16;

  static float* dir; // directional 2D table (cosThetaO, roughness)

  static prt_inline float eval(float cosThetaO, float roughness)
  {
    const Vec2f p = Vec2f(cosThetaO, roughness) * float(size-1);
    return interp2DLinear(p, dir, size);
  }

  static prt_inline vfloat eval(vbool m, vfloat cosThetaO, vfloat roughness)
  {
    const Vec2vf p = Vec2vf(cosThetaO, roughness) * vfloat(size-1);
    return interp2DLinear(m, p, dir, size);
  }

  static void precompute();
};

void precomputeAlbedoTables();

} // namespace prt
