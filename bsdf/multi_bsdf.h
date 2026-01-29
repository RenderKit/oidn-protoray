// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "bsdf.h"

namespace prt {

class MultiBsdf : public Bsdf
{
private:
  static const int maxBsdfs = 8;

  const Bsdf* bsdfs[maxBsdfs];
  float scales[maxBsdfs];
  float importances[maxBsdfs];
  float importanceSum;
  int numBsdfs;

public:
  prt_inline MultiBsdf()
    : importanceSum(0.f),
      numBsdfs(0) {}

  prt_inline void add(const Bsdf* bsdf, float scale = 1.f)
  {
    assert(numBsdfs < maxBsdfs);
    if (numBsdfs >= maxBsdfs)
      return;

    flags |= bsdf->getFlags();
    bsdfs[numBsdfs] = bsdf;
    scales[numBsdfs] = scale;
    float importance = bsdf->getImportance() * scale;
    importances[numBsdfs] = importance;
    importanceSum += importance;
    numBsdfs++;
  }

  Vec3f eval(const ShadingContext& ctx, const Vec3f& wo, const Vec3f& wi, float& pdf) const override
  {
    Vec3f value = zero;
    pdf = zero;

    for (int i = 0; i < numBsdfs; ++i)
    {
      if (importances[i] > 0.f)
      {
        const Bsdf* curBsdf = bsdfs[i];
        float curPdf;
        Vec3f curValue = curBsdf->eval(ctx, wo, wi, curPdf);
        value += curValue * scales[i];
        pdf += curPdf * importances[i];
      }
    }

    pdf *= rcp(importanceSum);
    return value;
  }

  Vec3f sample(const ShadingContext& ctx, const Vec3f& wo, Vec3f& wi, float& pdf, int& type, const BsdfSample& s) const override
  {
    if (importanceSum == 0.f) // also handles case numBsdfs == 0
      return zero;

    if (numBsdfs == 1)
    {
      const Bsdf* bsdf = bsdfs[0];
      Vec3f weight = bsdf->sample(ctx, wo, wi, pdf, type, s);
      return weight * scales[0];
    }
    else
    {
      // Choose which BSDF to sample
      float x = s.c * importanceSum;
      int choice = 0;
      float prefixSum = importances[0];
      while ((choice < numBsdfs-1) && (x > prefixSum))
        prefixSum += importances[++choice];

      float importance = importances[choice];
      float scale = scales[choice];

      // Remap the sample
      BsdfSample s1 = s;
      s1.c = (x + importance - prefixSum) * rcp(importance);

      // Sample the chosen BSDF
      const Bsdf* bsdf = bsdfs[choice];
      Vec3f weight = bsdf->sample(ctx, wo, wi, pdf, type, s1);
      weight *= scale;

      if ((reduceMax(weight) <= 0.f) || (pdf <= 0.f))
        return zero;

      // If the sampled BSDF is a delta distribution (specular), we cannot add the contributions from the other BSDFs
      // [Pharr et al., 2016, "Physically Based Rendering", 3rd Edition, Section 14.1.6]
      if (type & BsdfType::Delta)
      {
        // Scale by rcp(selection pdf)
        weight *= importanceSum * rcp(importance);
        return weight;
      }

      // Compute the overall weight and pdf
      Vec3f value = weight * pdf;
      pdf *= importance;

      for (int i = 0; i < numBsdfs; ++i)
      {
        if ((i != choice) && (importances[i] > 0.f))
        {
          const Bsdf* curBsdf = bsdfs[i];
          float curPdf;
          Vec3f curValue = curBsdf->eval(ctx, wo, wi, curPdf);
          value += curValue * scales[i];
          pdf += curPdf * importances[i];
        }
      }

      pdf *= rcp(importanceSum);
      weight = value * rcp(pdf);
      return weight;
    }
  }

  Vec3f getTransparency(const ShadingContext& ctx, const Vec3f& wo) const override
  {
    Vec3f value = zero;

    for (int i = 0; i < numBsdfs; ++i)
    {
      if (importances[i] > 0.f)
      {
        const Bsdf* curBsdf = bsdfs[i];
        Vec3f curValue = curBsdf->getTransparency(ctx, wo);
        value += curValue * scales[i];
      }
    }

    return value;
  }

  float getImportance() const override
  {
    return importanceSum;
  }
};

class MultiBsdfSimd : public BsdfSimd
{
private:
  static const int maxBsdfs = 8;

  const BsdfSimd* bsdfs[maxBsdfs];
  vfloat scales[maxBsdfs];
  vfloat importances[maxBsdfs];
  vfloat importanceSum;
  int numBsdfs;

public:
  prt_inline MultiBsdfSimd()
    : importanceSum(0.f),
      numBsdfs(0) {}

  prt_inline void add(vbool m, const BsdfSimd* bsdf, vfloat scale = 1.f)
  {
    assert(numBsdfs < maxBsdfs);
    if (numBsdfs >= maxBsdfs)
      return;

    flags |= select(m, bsdf->getFlags(), 0);
    bsdfs[numBsdfs] = bsdf;
    scales[numBsdfs] = select(m, scale, 0.f);
    vfloat importance = bsdf->getImportance(m) * scale;
    importances[numBsdfs] = select(m, importance, 0.f);
    importanceSum += importances[numBsdfs];
    numBsdfs++;
  }

  Vec3vf eval(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo, const Vec3vf& wi, vfloat& pdf) const override
  {
    Vec3vf value = zero;
    pdf = zero;

    for (int i = 0; i < numBsdfs; ++i)
    {
      vbool mCur = m & (importances[i] > 0.f);
      if (any(mCur))
      {
        const BsdfSimd* curBsdf = bsdfs[i];
        vfloat curPdf;
        Vec3vf curValue = curBsdf->eval(mCur, ctx, wo, wi, curPdf);
        set(mCur, value, value + curValue * scales[i]);
        set(mCur, pdf, pdf + curPdf * importances[i]);
      }
    }

    pdf *= rcp(importanceSum);
    return value;
  }

  Vec3vf sample(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo, Vec3vf& wi, vfloat& pdf, vint& type, const BsdfSampleSimd& s) const override
  {
    m &= importanceSum > 0.f;
    if (none(m)) // also handles case numBsdfs == 0
      return zero;

    if (numBsdfs == 1)
    {
      const BsdfSimd* bsdf = bsdfs[0];
      Vec3vf curWeight = bsdf->sample(m, ctx, wo, wi, pdf, type, s);
      Vec3vf weight = select(m, curWeight, Vec3vf(zero));
      weight *= scales[0];
      return weight;
    }
    else
    {
      // Choose which BSDF to sample
      vfloat x = s.c * importanceSum;
      vint choice = 0;
      vfloat prefixSum = importances[0];
      vfloat importance = importances[0];
      vfloat scale = scales[0];
      for (int i = 1; i < numBsdfs; ++i)
      {
        vbool mAdd = m & (x > prefixSum);
        if (none(mAdd)) break;
        set(mAdd, choice, vint(i));
        set(mAdd, prefixSum, prefixSum + importances[i]);
        set(mAdd, importance, importances[i]);
        set(mAdd, scale, scales[i]);
      }

      // Remap the sample
      BsdfSampleSimd s1 = s;
      s1.c = (x + importance - prefixSum) * rcp(importance);

      // Sample the chosen BSDF
      Vec3vf weight = zero;
      wi = zero;
      pdf = zero;
      type = zero;

      vbool mSample = m;
      while (any(mSample))
      {
        vbool mCur;
        int i = nextUnique(mSample, choice, mCur);
        const BsdfSimd* bsdf = bsdfs[i];

        Vec3vf curWi;
        vfloat curPdf;
        vint curType;
        Vec3vf curWeight = bsdf->sample(mCur, ctx, wo, curWi, curPdf, curType, s1);
        mCur &= reduceMax(curWeight) > 0.f;

        set(mCur, wi, curWi);
        set(mCur, pdf, curPdf);
        set(mCur, type, curType);
        set(mCur, weight, curWeight);
      }

      weight *= scale;

      m &= (reduceMax(weight) > 0.f) & (pdf > 0.f);
      if (none(m))
        return zero;

      // If the sampled BSDF is a delta distribution (specular), we cannot add the contributions from the other BSDFs
      // [Pharr et al., 2016, "Physically Based Rendering", 3rd Edition, Section 14.1.6]
      vbool mSpec = m & ((type & vint(BsdfType::Delta)) != 0);
      if (any(mSpec))
      {
        // Scale by rcp(selection pdf)
        set(mSpec, weight, weight * importanceSum * rcp(importance));
        m = andn(m, mSpec);
        if (none(m))
          return weight;
      }

      // Compute the overall weight and pdf
      Vec3vf value = weight * pdf;
      set(m, pdf, pdf * importance);

      for (int i = 0; i < numBsdfs; ++i)
      {
        vbool mCur = m & (i != choice) & (importances[i] > 0.f);
        if (any(mCur))
        {
          const BsdfSimd* curBsdf = bsdfs[i];
          vfloat curPdf;
          Vec3vf curValue = curBsdf->eval(mCur, ctx, wo, wi, curPdf);
          set(mCur, value, value + curValue * scales[i]);
          set(mCur, pdf, pdf + curPdf * importances[i]);
        }
      }

      set(m, pdf, pdf * rcp(importanceSum));
      set(m, weight, value * rcp(pdf));
      return weight;
    }
  }

  Vec3vf getTransparency(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo) const override
  {
    Vec3vf value = zero;

    for (int i = 0; i < numBsdfs; ++i)
    {
      vbool mCur = m & (importances[i] > 0.f);
      if (any(mCur))
      {
        const BsdfSimd* curBsdf = bsdfs[i];
        Vec3vf curValue = curBsdf->getTransparency(mCur, ctx, wo);
        set(mCur, value, value + curValue * scales[i]);
      }
    }

    return value;
  }

  vfloat getImportance(vbool m) const override
  {
    return importanceSum;
  }

  Vec3vf getAlbedo(vbool m) const override
  {
    Vec3vf value = zero;
    vfloat scaleSum = zero;
    vfloat totalScaleSum = zero;

    for (int i = 0; i < numBsdfs; ++i)
    {
      vbool mCur = m & (importances[i] > 0.f);
      if (any(mCur))
      {
        const BsdfSimd* curBsdf = bsdfs[i];
        Vec3vf curValue = curBsdf->getAlbedo(mCur);
        vfloat curScale = scales[i];

        set(mCur, totalScaleSum, totalScaleSum + curScale);
        mCur &= curValue.x >= 0.f; // ignore full transparency
        set(mCur, value, value + curValue * curScale);
        set(mCur, scaleSum, scaleSum + curScale);
      }
    }

    value *= totalScaleSum * rcpSafe(scaleSum);
    return value;
  }

  Vec3vf getDiffuseAlbedo(vbool m) const override
  {
    Vec3vf value = zero;
    vfloat scaleSum = zero;
    vfloat totalScaleSum = zero;

    for (int i = 0; i < numBsdfs; ++i)
    {
      vbool mCur = m & (importances[i] > 0.f);
      if (any(mCur))
      {
        const BsdfSimd* curBsdf = bsdfs[i];
        Vec3vf curValue = curBsdf->getDiffuseAlbedo(mCur);
        vfloat curScale = scales[i];

        set(mCur, totalScaleSum, totalScaleSum + curScale);
        mCur &= curValue.x >= 0.f; // ignore full transparency
        set(mCur, value, value + curValue * curScale);
        set(mCur, scaleSum, scaleSum + curScale);
      }
    }

    value *= totalScaleSum * rcpSafe(scaleSum);
    return value;
  }

  Vec3vf getSpecularAlbedo(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo) const override
  {
    Vec3vf value = zero;
    vfloat scaleSum = zero;
    vfloat totalScaleSum = zero;

    for (int i = 0; i < numBsdfs; ++i)
    {
      vbool mCur = m & (importances[i] > 0.f);
      if (any(mCur))
      {
        const BsdfSimd* curBsdf = bsdfs[i];
        Vec3vf curValue = curBsdf->getSpecularAlbedo(mCur, ctx, wo);
        vfloat curScale = scales[i];

        set(mCur, totalScaleSum, totalScaleSum + curScale);
        mCur &= curValue.x >= 0.f; // ignore full transparency
        set(mCur, value, value + curValue * curScale);
        set(mCur, scaleSum, scaleSum + curScale);
      }
    }

    value *= totalScaleSum * rcpSafe(scaleSum);
    return value;
  }

  Vec3vf getNormal(vbool m) const override
  {
    Vec3vf N = zero;

    // Simply return the first valid normal
    for (int i = 0; i < numBsdfs; ++i)
    {
      vbool mCur = m & (importances[i] > 0.f);
      if (any(mCur))
      {
        const BsdfSimd* curBsdf = bsdfs[i];
        Vec3vf curN = curBsdf->getNormal(mCur);
        mCur &= lengthSqr(curN) > 0.f;

        set(mCur, N, curN);

        m = andn(m, mCur);
        if (none(m))
          break;
      }
    }

    return N;
  }

  vfloat getRoughness(vbool m) const override
  {
    vfloat value = zero;
    vfloat scaleSum = zero;
    vfloat totalScaleSum = zero;

    for (int i = 0; i < numBsdfs; ++i)
    {
      vbool mCur = m & (importances[i] > 0.f);
      if (any(mCur))
      {
        const BsdfSimd* curBsdf = bsdfs[i];
        vfloat curValue = curBsdf->getRoughness(mCur);
        vfloat curScale = scales[i];

        set(mCur, totalScaleSum, totalScaleSum + curScale);
        mCur &= curValue >= 0.f; // ignore full transparency
        set(mCur, value, value + curValue * curScale);
        set(mCur, scaleSum, scaleSum + curScale);
      }
    }

    value *= totalScaleSum * rcpSafe(scaleSum);
    return value;
  }
};

} // namespace prt
