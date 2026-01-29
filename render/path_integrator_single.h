// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/logging.h"
#include "sampling/shape_sampler.h"
#include "integrator.h"

namespace prt {

// Path tracer
template <class Sampler>
class PathIntegratorSingle : public IntegratorBase
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
  int maxDiffuseDepth;
  float minWeight;   // minimum path weight to terminate very low contribution paths

public:
  PathIntegratorSingle(const Props& props)
    : IntegratorBase(props)
  {
    maxDepth = props.get("maxDepth", 100);
    rrDepth = props.get("rrDepth", 8);
    maxDiffuseDepth = props.get("maxDiffuseDepth", INT32_MAX);
    minWeight = props.get("minWeight", 1e-4f);
  }

  int getSampleSize() const
  {
    return sampleDimBaseSize + sampleDimBounceSize * maxDepth;
  }

  Vec3f getColor(Ray& ray, IntersectorSingle* intersector, IntersectorFilter* filter, const Scene* scene, Sampler& sampler, IntegratorState<Sampler>& state)
  {
    Vec3f L = zero;
    Vec3f throughput = one;
    int mediumId = zero;

    int depth = 0;
    int diffuseDepth = 0;
    int sampleDim = sampleDimBaseSize;
    float lastBsdfPdf = 1.f;
    int lastBsdfType = BsdfType::SpecularReflection;

    for (; ;)
    {
      // Intersect the ray
      RayHint rayHint = (depth == 0) ? RayHint::Coherent : RayHint::Incoherent;
      Hit hit;
      intersector->intersect(ray, hit, state.rayStats, rayHint);

      // Check for hitting the environment lights
      if (!ray.isHit())
      {
        float lightPdf;
        Vec3f lightValue = scene->evalEnvLight(ray.dir, lightPdf);

        float weight = (lastBsdfType & BsdfType::Smooth) ? misWeight(lastBsdfPdf, lightPdf) : 1.f;
        L += throughput * weight * lightValue;
        break;
      }

      // Get the differential geometry
      ShadingContext ctx;
      scene->postIntersect(ray, hit, ctx);
      int matId = scene->getMaterialId(hit);
      const Material* mat = scene->getMaterial(matId);

      // Check for hitting an area light
      if (mat->getType() & MaterialType::Emissive)
      {
        float lightPdf;
        Vec3f lightValue = scene->evalAreaLight(ray.dir, lightPdf, ray.far, hit);

        float weight = (lastBsdfType & BsdfType::Smooth) ? misWeight(lastBsdfPdf, lightPdf) : 1.f;
        L += throughput * weight * lightValue;
        break;
      }

      // Path termination
      if (depth == maxDepth) break;

      // Get the BSDF
      Vec3f wo = -ray.dir;
      const Bsdf* bsdf = mat->getBsdf(ctx, wo, mediumId);
      int bsdfFlags = bsdf->getFlags();

      // Path termination
      if (bsdfFlags & BsdfType::Diffuse)
      {
        if (diffuseDepth == maxDiffuseDepth)
          break;
        diffuseDepth++;
      }

      // Apply volume transmission
      if (mediumId > 0)
      {
        Vec3f mediumColor = scene->getMedia().getColor(mediumId);
        throughput *= pow(mediumColor, ray.far);
      }

      // Add direct lighting
      if (bsdfFlags & BsdfType::Smooth)
      {
        // Sample a light
        Vec3f dirWi;
        float dirLightPdf;
        float dirDist;
        LightSample s(sampler.get3D(state.sampler, sampleDim+sampleDimLightV));
        Vec3f dirLightWeight = scene->sampleLight(ctx, dirWi, dirLightPdf, dirDist, s);
        if (reduceMax(dirLightWeight) > 0.f && dirLightPdf > 0.f)
        {
          // Evaluate the BSDF
          float dirBsdfPdf;
          Vec3f dirBsdfValue = bsdf->eval(ctx, wo, dirWi, dirBsdfPdf);
          if (reduceMax(dirBsdfValue) > 0.f)
          {
            // Test visibility
            Ray shadowRay;
            shadowRay.init(ctx.p, dirWi, ctx.eps, dirDist-ctx.eps);
            Vec3f T = 1.f;
            intersector->occluded(shadowRay, state.rayStats, RayHint::Incoherent, filter, &T);
            if (!shadowRay.isOccluded())
            {
              Vec3f dirThroughput = throughput * dirBsdfValue;

              // Russian roulette adjustment
              if (depth >= rrDepth)
              {
                float contProb = min(luminance(dirThroughput*rcp(dirBsdfPdf)), 0.95f);
                dirBsdfPdf *= contProb;
              }

              float weight = misWeight(dirLightPdf, dirBsdfPdf);
              L += dirThroughput * weight * dirLightWeight * T;
            }
          }
        }
      }

      // Sample the BSDF
      Vec3f wi;
      float bsdfPdf;
      int bsdfType;
      BsdfSample s(sampler.get3D(state.sampler, sampleDim+sampleDimBsdfV));
      Vec3f bsdfWeight = bsdf->sample(ctx, wo, wi, bsdfPdf, bsdfType, s);
      if (!(reduceMax(bsdfWeight) > 0.f && bsdfPdf > 0.f)) break;

      // Adjust the throughput
      throughput *= bsdfWeight;
      if (reduceMax(throughput) < minWeight) break;

      // Russian roulette
      if (depth >= rrDepth)
      {
        float contProb = min(luminance(throughput), 0.95f);
        if (sampler.get1D(state.sampler, sampleDim+sampleDimTerminate) >= contProb) break;
        throughput *= rcp(contProb);
        bsdfPdf *= contProb;
      }

      // Track the medium
      mat->nextMedium(ctx, wo, wi, mediumId);

      // Update lastBsdfPdf and lastBsdfType if we did NOT sample a continuation ray
      if (!(bsdfType & BsdfType::Continuation))
      {
        lastBsdfPdf = bsdfPdf;
        lastBsdfType = bsdfType;
      }

      // Extend the path
      ray.init(ctx.p, wi, ctx.eps);
      depth++;
      sampleDim += sampleDimBounceSize;
    }

    return clampL(L);
  }

private:
  // MIS power heuristic
  static prt_inline float misWeight(float pdfA, float pdfB)
  {
    return sqr(pdfA) * rcpSafe(sqr(pdfA) + sqr(pdfB));
  }
};

} // namespace prt
