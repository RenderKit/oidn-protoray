// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "bsdf/multi_bsdf.h"
#include "bsdf/diffuse.h"
#include "bsdf/rough_diffuse.h"
#include "bsdf/conductor.h"
#include "bsdf/rough_conductor.h"
#include "bsdf/coating.h"
#include "bsdf/rough_coating.h"
#include "flakes.h"
#include "car_paint.h"

namespace prt {

CarPaintMaterial::CarPaintMaterial(const Props& props, MaterialFactory& factory)
  : Material(props, factory)
{
  baseColorPar = getParam3f("baseColor", Vec3f(0.8f));
  roughnessPar = getParam1f("roughness", 0.f);
  outsideIor = max(props.get("outsideIor", 1.f), 0.f);
  if (outsideIor < 1.f) outsideIor = rcp(outsideIor);
  normalPar = getParamNormal("normal", "bump");

  // Flakes
  float flakeScale = max(props.get("flakeScale", 100.f), 0.f);
  float flakeDensity = clamp(props.get("flakeDensity", 0.f));
  float flakeSpread = max(props.get("flakeSpread", 0.3f), 0.f);
  float flakeJitter = clamp(props.get("flakeJitter", 0.75f));
  flakes = std::make_shared<Flakes>(flakeScale, flakeDensity, flakeSpread, flakeJitter);
  flakeRoughnessPar = getParam1f("flakeRoughness", 0.3f);

  // Clear coat
  coatPar = getParam1f("coat", 1.f);
  coatIor = max(props.get("coatIor", 1.5f), 0.f);
  if (coatIor < 1.f) coatIor = rcp(coatIor);
  coatColorPar = getParam3f("coatColor", 1.f);
  coatThicknessPar = getParam1f("coatThickness", 1.f);
  coatRoughnessPar = getParam1f("coatRoughness", 0.f);
  coatNormalPar = getParamNormal("coatNormal", "coatBump");

  // Flip-flop
  flipflopColorPar = getParam3f("flipflopColor", Vec3f(1.f));
  flipflopFalloffPar = getParam1f("flipflopFalloff", 1.f);

#ifdef PRT_MUTATION_SUPPORT
  // Save original values
  coatIorOrg = coatIor;
#endif
}

Bsdf* CarPaintMaterial::getBsdf(ShadingContext& ctx, const Vec3f& wo) const
{
  Bsdf* bsdf = nullptr;

  // Metallic flakes in the clear coat layer
  const Basis3f* flakeFrame = nullptr;
  int flakeVisible = 0;

  if (flakes->density > eps)
  {
    Vec3f flakeN = flakes->eval(ctx.p, flakeVisible);
    if (flakeVisible)
    {
      flakeFrame = ctx.makeFrame(flakeN, wo);

      // Flakes are made of aluminum
      const Vec3f flakeFrontColor(0.912f, 0.914f, 0.920f);
      const Vec3f flakeEdgeColor(0.970f, 0.979f, 0.988f);
      FresnelConductorArtistic<float> flakeFresnel(flakeFrontColor, flakeEdgeColor);

      float flakeRoughness = clamp(flakeRoughnessPar.get(ctx));
      if (flakeRoughnessPar)
        bsdf = ctx.make<RoughConductorBrdf<FresnelConductorArtistic<float>>>(flakeFrame, flakeFresnel, flakeRoughness, 0.f);
      else
        bsdf = ctx.make<ConductorBrdf<FresnelConductorArtistic<float>>>(flakeFrame, flakeFresnel);
    }
  }

  // Base diffuse layer
  if (!flakeVisible)
  {
    const Basis3f* frame = ctx.makeFrame(normalPar.get(ctx), wo);
    Vec3f baseColor = clamp(baseColorPar.get(ctx));
    float roughness = clamp(roughnessPar.get(ctx));

    if (roughnessPar)
      bsdf = ctx.make<RoughDiffuseBrdf>(frame, baseColor, roughness);
    else
      bsdf = ctx.make<DiffuseBrdf>(frame, baseColor);
  }

  // Clear coat
  float coat = clamp(coatPar.get(ctx));
  if ((coat > eps) && (abs(coatIor-outsideIor) > eps))
  {
    float coatEta = outsideIor * rcp(coatIor);
    coatEta = clamp(coatEta, 1.f/3.f, 3.f); // clamp to common range due to LUTs

    Vec3f coatColor = clamp(coatColorPar.get(ctx));
    Vec3f coatFinalColor = coatColor;
    if (flakeVisible)
    {
      float flipflopFalloff = clamp(flipflopFalloffPar.get(ctx));
      if (flipflopFalloff < 1.f-eps)
      {
        // Pearlescent flakes
        Vec3f flipflopColor = clamp(flipflopColorPar.get(ctx));
        float cosThetaO = max(dot(wo, flakeFrame->N), 0.f);
        float weight = pow(1.f - cosThetaO, rcp(1.f - flipflopFalloff)); // use Schlick for the blending weight
        coatFinalColor = lerp(coatColor, flipflopColor, weight);
      }
    }

    const Basis3f* coatFrame = ctx.makeFrame(coatNormalPar.get(ctx), wo);
    float coatThickness = max(coatThicknessPar.get(ctx), 0.f);
    float coatRoughness = clamp(coatRoughnessPar.get(ctx));

    if (coatRoughnessPar)
      bsdf = ctx.make<RoughCoatingBsdf>(coatFrame, bsdf, coatEta, coatFinalColor, coatThickness, coatRoughness, 0.f, coat);
    else
      bsdf = ctx.make<CoatingBsdf>(coatFrame, bsdf, coatEta, coatFinalColor, coatThickness, coat);
  }

  return bsdf;
}

BsdfSimd* CarPaintMaterial::getBsdf(vbool m, ShadingContextSimd& ctx, const Vec3vf& wo) const
{
  MultiBsdfSimd* multiBsdf = ctx.make<MultiBsdfSimd>();
  BsdfSimd* bsdf = multiBsdf;

  // Metallic flakes in the clear coat layer
  const Basis3vf* flakeFrame = nullptr;
  vint flakeVisible = 0;
  vbool flakeMask = zero;

  if (flakes->density > eps)
  {
    Vec3vf flakeN = flakes->eval(ctx.p, flakeVisible);
    flakeMask = m & (flakeVisible != 0);
    if (any(flakeMask))
    {
      flakeFrame = ctx.makeFrame(flakeMask, flakeN, wo);

      // Flakes are made of aluminum
      const Vec3f flakeFrontColor(0.912f, 0.914f, 0.920f);
      const Vec3f flakeEdgeColor(0.970f, 0.979f, 0.988f);
      FresnelConductorArtistic<vfloat> flakeFresnel(flakeFrontColor, flakeEdgeColor);

      vfloat flakeRoughness = clamp(flakeRoughnessPar.get(flakeMask, ctx));
      if (flakeRoughnessPar)
        multiBsdf->add(flakeMask, ctx.make<RoughConductorBrdfSimd<FresnelConductorArtistic<vfloat>>>(flakeMask, flakeFrame, flakeFresnel, flakeRoughness, 0.f));
      else
        multiBsdf->add(flakeMask, ctx.make<ConductorBrdfSimd<FresnelConductorArtistic<vfloat>>>(flakeMask, flakeFrame, flakeFresnel));
    }
  }

  // Base diffuse layer
  vbool baseMask = m & (flakeVisible == 0);
  if (any(baseMask))
  {
    const Basis3vf* frame = ctx.makeFrame(baseMask, normalPar.get(baseMask, ctx), wo);
    Vec3vf baseColor = clamp(baseColorPar.get(baseMask, ctx));
    vfloat roughness = clamp(roughnessPar.get(baseMask, ctx));

    if (roughnessPar)
      multiBsdf->add(baseMask, ctx.make<RoughDiffuseBrdfSimd>(baseMask, frame, baseColor, roughness));
    else
      multiBsdf->add(baseMask, ctx.make<DiffuseBrdfSimd>(baseMask, frame, baseColor));
  }

  // Clear coat
  vfloat coat = clamp(coatPar.get(m, ctx));
  if (any(m & (coat > eps)) && (abs(coatIor-outsideIor) > eps))
  {
    float coatEta = outsideIor * rcp(coatIor);
    coatEta = clamp(coatEta, 1.f/3.f, 3.f); // clamp to common range due to LUTs

    Vec3vf coatColor = clamp(coatColorPar.get(m, ctx));
    Vec3vf coatFinalColor = coatColor;
    if (any(flakeMask))
    {
      vfloat flipflopFalloff = clamp(flipflopFalloffPar.get(flakeMask, ctx));
      vbool flipflopMask = flakeMask & (flipflopFalloff < 1.f-eps);
      if (any(flipflopMask))
      {
        // Pearlescent flakes
        Vec3vf flipflopColor = clamp(flipflopColorPar.get(flipflopMask, ctx));
        vfloat cosThetaO = max(dot(wo, flakeFrame->N), 0.f);
        vfloat weight = pow(1.f - cosThetaO, rcp(1.f - flipflopFalloff)); // use Schlick for the blending weight
        set(flipflopMask, coatFinalColor, lerp(coatColor, flipflopColor, weight));
      }
    }

    const Basis3vf* coatFrame = ctx.makeFrame(m, coatNormalPar.get(m, ctx), wo);
    vfloat coatThickness = max(coatThicknessPar.get(m, ctx), 0.f);
    vfloat coatRoughness = clamp(coatRoughnessPar.get(m, ctx));

    if (coatRoughnessPar)
      bsdf = ctx.make<RoughCoatingBsdfSimd>(m, coatFrame, bsdf, coatEta, coatFinalColor, coatThickness, coatRoughness, 0.f, coat);
    else
      bsdf = ctx.make<CoatingBsdfSimd>(m, coatFrame, bsdf, coatEta, coatFinalColor, coatThickness, coat);
  }

  return bsdf;
}

void CarPaintMaterial::mutate(Random& rng)
{
#ifdef PRT_MUTATION_SUPPORT
  baseColorPar.mutate(rng);
  roughnessPar.mutate(rng, 0.f, 1.f);

  if (coatPar && abs(coatIorOrg-outsideIor) > eps)
  {
    const float minCoatIor = 1.f;
    const float maxCoatIor = max(2.f, coatIorOrg);
    coatIor = lerp(minCoatIor, maxCoatIor, rng.get1f()) * outsideIor;
    coatColorPar.mutate(rng);
    coatRoughnessPar.mutate(rng, 0.f, (rng.get1f() < 0.75f) ? 0.1f : 0.5f);

    flakes->mutate(rng);
    flakeRoughnessPar.mutate(rng, 0.2f, 0.5f);

    if (flipflopFalloffPar != 1.f)
    {
      flipflopColorPar.mutate(rng);
      if (flipflopFalloffPar != 0.f)
        flipflopFalloffPar.mutate(rng, 0.f, 1.f);
    }
  }

  type &= ~MaterialType::FirstHitAov;
  if (rng.get1f() < 0.4f)
    type |= MaterialType::FirstHitAov;
#endif
}

} // namespace prt
