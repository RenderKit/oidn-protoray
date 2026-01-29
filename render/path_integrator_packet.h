// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "integrator.h"

namespace prt {

template <class Sampler>
class PathIntegratorPacket : public IntegratorBase
{
private:
  // Sample dimensions
  enum
  {
    sampleDimBsdfV     = 0,
    sampleDimBsdfC     = 2,
    sampleDimLightV    = 3,
    sampleDimLightC    = 5,
    sampleDimTerminate = 6,

    sampleDimBounceSize = 7
  };

  int rrDepth;       // Russian roulette starting depth
  float minWeight;   // minimum path weight to terminate very low contribution paths

public:
  PathIntegratorPacket(const Props& props)
    : IntegratorBase(props)
  {
    maxDepth = props.get("maxDepth", 100);
    rrDepth = props.get("rrDepth", 8);
    minWeight = props.get("minWeight", 1e-4f);
  }

  int getSampleSize() const
  {
    return sampleDimBaseSize + sampleDimBounceSize * maxDepth;
  }

  Vec3vf getColor(RaySimd& ray, IntersectorPacket* intersector, IntersectorFilterSimd* filter, const Scene* scene, Sampler& sampler, IntegratorState<Sampler>& state)
  {
    Vec3vf L = zero;
    Vec3vf throughput = one;
    vint mediumId = zero;

    vbool m = one;
    int depth = 0;
    int sampleDim = sampleDimBaseSize;
    vfloat lastBsdfPdf = 1.f;
    vint lastBsdfType = BsdfType::SpecularReflection;

    for (; ;)
    {
      // Intersect the ray
      RayHint rayHint = (depth == 0) ? RayHint::Coherent : RayHint::Incoherent;
      HitSimd hit;
      intersector->intersect(m, ray, hit, state.rayStats, rayHint);

      // Check for hitting the environment lights
      vbool mMiss = andn(m, ray.isHit());
      if (any(mMiss))
      {
        prt_profile(state.rayStats.shadeSimdBatchCount++);
        prt_profile(state.rayStats.shadeSimdActiveLaneCount += bitCount(toIntMask(mMiss)));

        vfloat lightPdf;
        Vec3vf lightValue = scene->evalEnvLight(mMiss, ray.dir, lightPdf);

        vfloat weight = select((lastBsdfType & BsdfType::Smooth) == 0, vfloat(1.f), misWeight(lastBsdfPdf, lightPdf));
        set(mMiss, L, L + throughput * weight * lightValue);

        m = andn(m, mMiss);
        if (none(m)) break;
      }

      vint matId = scene->getMaterialId(m, hit);

      // Check for hitting an area light
      vbool mAreaHit = m & (matId <= scene->lightMaterialCount);
      if (any(mAreaHit))
      {
        prt_profile(state.rayStats.shadeSimdBatchCount++);
        prt_profile(state.rayStats.shadeSimdActiveLaneCount += bitCount(toIntMask(mAreaHit)));

        vfloat lightPdf;
        Vec3vf lightValue = scene->evalAreaLight(mAreaHit, ray.dir, lightPdf, ray.far, hit);

        vfloat weight = select((lastBsdfType & BsdfType::Smooth) == 0, vfloat(1.f), misWeight(lastBsdfPdf, lightPdf));
        set(mAreaHit, L, L + throughput * weight * lightValue);

        m = andn(m, mAreaHit);
        if (none(m)) break;
      }

      // Path termination
      if (depth == maxDepth) break;

      // Get the differential geometry
      ShadingContextSimd ctx;
      scene->postIntersect(m, ray, hit, ctx);
      Vec3vf wo = -ray.dir;

      // Sample a light
      Vec3vf dirWi;
      vfloat dirLightPdf;
      vfloat dirDist;
      LightSampleSimd lightSample(sampler.get3D(m, state.sampler, sampleDim+sampleDimLightV));
      Vec3vf dirLightWeight = scene->sampleLight(m, ctx, dirWi, dirLightPdf, dirDist, lightSample);
      vbool mDir = reduceMax(dirLightWeight) > 0.f;
      mDir &= dirLightPdf > 0.f;

      vfloat dirBsdfPdf = zero;
      Vec3vf dirBsdfValue = zero;

      Vec3vf wi = zero;
      vfloat bsdfPdf = zero;
      vint bsdfType = zero;
      Vec3vf bsdfWeight = zero;

      // Apply volume transmission
      if (any(m & (mediumId > 0)))
      {
        Vec3vf mediumColor = scene->getMedia().getColor(m, mediumId);
        throughput *= pow(mediumColor, ray.far);
      }

      // For each unique material ID
      vbool mMat = m;
      do
      {
        int pos = bitScan(toIntMask(mMat));
        vbool mMatCur = mMat & (matId == vint(matId[pos]));
        mMat = andn(mMat, mMatCur);

        prt_profile(state.rayStats.shadeSimdBatchCount++);
        prt_profile(state.rayStats.shadeSimdActiveLaneCount += bitCount(toIntMask(mMatCur)));

        const Material* mat = scene->getMaterial(matId[pos]);
        const BsdfSimd* bsdf = mat->getBsdf(mMatCur, ctx, wo, mediumId);
        vint bsdfFlags = bsdf->getFlags();

        // Add direct lighting
        vbool mMatCurDir = mMatCur & mDir & ((bsdfFlags & BsdfType::Smooth) != 0);
        if (LIKELY(any(mMatCurDir)))
        {
          // Evaluate the BSDF
          vfloat curDirBsdfPdf;
          Vec3vf curDirBsdfValue = bsdf->eval(mMatCurDir, ctx, wo, dirWi, curDirBsdfPdf);

          set(mMatCurDir, &dirBsdfPdf, curDirBsdfPdf);
          set(mMatCurDir, &dirBsdfValue, curDirBsdfValue);
        }

        // Sample the BSDF
        Vec3vf curWi;
        vfloat curBsdfPdf;
        vint curBsdfType;
        BsdfSampleSimd bsdfSample(sampler.get3D(mMatCur, state.sampler, sampleDim+sampleDimBsdfV));
        Vec3vf curBsdfWeight = bsdf->sample(mMatCur, ctx, wo, curWi, curBsdfPdf, curBsdfType, bsdfSample);
        mMatCur &= reduceMax(curBsdfWeight) > 0.f;

        set(mMatCur, &wi, curWi);
        set(mMatCur, &bsdfPdf, curBsdfPdf);
        set(mMatCur, &bsdfType, curBsdfType);
        set(mMatCur, &bsdfWeight, curBsdfWeight);

        // Track the medium
        mat->nextMedium(mMatCur, ctx, wo, curWi, mediumId);
      } while (any(mMat));

      // Test visibility
      mDir &= reduceMax(dirBsdfValue) > 0.f;
      if (LIKELY(any(mDir)))
      {
        RaySimd shadowRay;
        shadowRay.init(ctx.p, dirWi, ctx.eps, dirDist-ctx.eps);
        Vec3vf T = vfloat(1.f);
        float* Tptr[] = {&T.x[0], &T.y[0], &T.z[0]};
        intersector->occluded(mDir, shadowRay, state.rayStats, RayHint::Incoherent, filter, Tptr);

        mDir &= shadowRay.isNotOccluded();
        if (any(mDir))
        {
          Vec3vf dirThroughput = throughput * dirBsdfValue;

          // Russian roulette adjustment
          if (depth >= rrDepth)
          {
            vfloat contProb = min(luminance(dirThroughput*rcp(dirBsdfPdf)), vfloat(0.95f));
            dirBsdfPdf *= contProb;
          }

          vfloat weight = misWeight(dirLightPdf, dirBsdfPdf);
          set(mDir, L, L + dirThroughput * weight * dirLightWeight * T);
        }
      }

      m &= reduceMax(bsdfWeight) > 0.f;
      m &= bsdfPdf > 0.f;
      if (none(m)) break;

      // Adjust the throughput
      throughput *= bsdfWeight;

      m &= reduceMax(throughput) >= minWeight;
      if (none(m)) break;

      // Russian roulette
      if (depth >= rrDepth)
      {
        vfloat contProb = min(luminance(throughput), vfloat(0.95f));
        m &= sampler.get1D(m, state.sampler, sampleDim+sampleDimTerminate) < contProb;
        if (none(m)) break;
        throughput *= rcp(contProb);
        bsdfPdf *= contProb;
      }

      // Update lastBsdfPdf and lastBsdfType if we did NOT sample a continuation ray
      vbool mUpdateBsdf = (bsdfType & BsdfType::Continuation) == 0;
      set(mUpdateBsdf, lastBsdfPdf, bsdfPdf);
      set(mUpdateBsdf, lastBsdfType, bsdfType);

      // Extend the path
      ray.init(ctx.p, wi, ctx.eps);
      depth++;
      sampleDim += sampleDimBounceSize;
    }

    return clampL(L);
  }

private:
  // MIS power heuristic
  static prt_inline vfloat misWeight(vfloat pdfA, vfloat pdfB)
  {
    return sqr(pdfA) * rcpSafe(sqr(pdfA) + sqr(pdfB));
  }
};

} // namespace prt
