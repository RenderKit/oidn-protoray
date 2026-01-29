// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "bsdf/multi_bsdf.h"
#include "bsdf/diffuse.h"
#include "bsdf/rough_diffuse.h"
#include "bsdf/conductor.h"
#include "bsdf/rough_conductor.h"
#include "bsdf/dielectric.h"
#include "bsdf/rough_dielectric.h"
#include "bsdf/thin_dielectric.h"
#include "bsdf/thin_rough_dielectric.h"
#include "bsdf/coating.h"
#include "bsdf/rough_coating.h"
#include "bsdf/sheen.h"
#include "bsdf/transparent.h"
#include "principled.h"

namespace prt {

PrincipledMaterial::PrincipledMaterial(const Props& props, MaterialFactory& factory)
  : Material(props, factory)
{
  baseColorPar = getParam3f("baseColor", Vec3f(0.8f));
  edgeColorPar = getParam3f("edgeColor", Vec3f(1.f));
  metallicPar = getParam1f("metallic", 0.f);
  diffusePar = getParam1f("diffuse", 1.f);
  specularPar = getParam1f("specular", 1.f);
  outsideIor = max(props.get("outsideIor", 1.f), 0.f);
  if (outsideIor < 1.f) outsideIor = rcp(outsideIor);
  ior = max(props.get("ior", outsideIor), 0.f);
  if (ior < 1.f) ior = rcp(ior);
  transmissionPar = getParam1f("transmission", 0.f);
  transmissionColor = clamp(props.get("transmissionColor", Vec3f(1.f)));
  transmissionDepth = max(props.get("transmissionDepth", 1.f), 0.f);
  roughnessPar = getParam1f("roughness", 0.f);
  normalPar = getParamNormal("normal", "bump");
  baseNormalPar = getParamNormal("baseNormal", "baseBump");

  // Clear coat
  coatPar = getParam1f("coat", 0.f);
  coatIor = max(props.get("coatIor", 1.5f), 0.f);
  if (coatIor < 1.f) coatIor = rcp(coatIor);
  coatColorPar = getParam3f("coatColor", 1.f);
  coatThicknessPar = getParam1f("coatThickness", 1.f);
  coatRoughnessPar = getParam1f("coatRoughness", 0.f);
  coatNormalPar = getParamNormal("coatNormal", "coatBump");

  // Sheen
  sheenPar = getParam1f("sheen", 0.f);
  sheenColorPar = getParam3f("sheenColor", 1.f);
  sheenTintPar = getParam1f("sheenTint", 0.f);
  sheenRoughnessPar = getParam1f("sheenRoughness", 0.2f);

  // Thin
  thin = props.get("thin", 0);
  backlightPar = getParam1f("backlight", 0.f);
  thicknessPar = getParam1f("thickness", 1.f);

  opacityPar = getParamAlpha("opacity", 1.f);
  if (opacityPar.isUniform() && baseColorPar.texture && baseColorPar.texture->hasAlpha())
    opacityPar = getParamAlpha("baseColor", 1.f, false);
  if (opacityPar != 1.f)
  {
    type |= MaterialType::Transparent;
    if (!opacityPar.isUniform())
      type |= MaterialType::Cutout;
  }

  if (transmissionPar)
  {
    type |= MaterialType::Transmissive;

    if (thin)
    {
      type |= MaterialType::Transparent;
    }
    else
    {
      type |= MaterialType::MediaInterface;

      Vec3f mediumColor = pow(transmissionColor.get(), transmissionDepth);
      mediumInt = factory.makeMedium(mediumColor, ior, mediumIntId);
      mediumExt = factory.makeMedium(one, outsideIor, mediumExtId);
    }
  }

#ifdef PRT_MUTATION_SUPPORT
  // Save original values
  iorOrg = ior;
  coatIorOrg = coatIor;
#endif
}

Bsdf* PrincipledMaterial::getBsdf(ShadingContext& ctx, const Vec3f& wo) const
{
  return getBsdfLobes<true>(ctx, wo);
}

template <bool wantOpaque>
prt_inline Bsdf* PrincipledMaterial::getBsdfLobes(ShadingContext& ctx, const Vec3f& wo) const
{
  Bsdf* bsdf = nullptr;

  float opacity = clamp(opacityPar.get(ctx));
  float transparency = 1.f - opacity;

  if (opacity > eps)
  {
    const Basis3f* frame = ctx.makeFrame(normalPar.get(ctx), wo);
    const Basis3f* baseFrame = baseNormalPar ? ctx.makeFrame(baseNormalPar.get(ctx), wo) : frame;
    Vec3f baseColor = clamp(baseColorPar.get(ctx));
    float metallic = clamp(metallicPar.get(ctx));
    float specular = clamp(specularPar.get(ctx));
    float roughness = clamp(roughnessPar.get(ctx));
    bool fromOutside = thin ? true : !ctx.backfacing;

    MultiBsdf* baseBsdf = ctx.make<MultiBsdf>();

    // Dielectric base
    float dielectric = 1.f - metallic;
    if (dielectric > eps)
    {
      float transmission = clamp(transmissionPar.get(ctx));

      float eta = fromOutside ? (outsideIor * rcp(ior)) : (ior * rcp(outsideIor));
      eta = clamp(eta, 1.f/3.f, 3.f); // clamp to common range due to LUTs

      // Plastic base
      if (wantOpaque)
      {
        float plastic = dielectric * (1.f - transmission);
        if (plastic > eps)
        {
          Bsdf* plasticBsdf = nullptr;

          // Diffuse
          float diffuse = clamp(diffusePar.get(ctx));
          Vec3f diffuseColor = baseColor * diffuse;
          float backlight = thin ? clamp(backlightPar.get(ctx), 0.f, 2.f) : 0.f;
          float diffuseTrans = backlight * 0.5f;
          float diffuseRefl = 1.f - diffuseTrans;

          if (diffuseRefl > eps)
          {
            if (roughnessPar)
              plasticBsdf = ctx.make<RoughDiffuseBrdf>(baseFrame, diffuseColor, roughness);
            else
              plasticBsdf = ctx.make<DiffuseBrdf>(baseFrame, diffuseColor);
          }

          if (diffuseTrans > eps)
          {
            Bsdf* backlightBsdf = ctx.make<DiffuseBtdf>(baseFrame, diffuseColor);
            if (plasticBsdf)
            {
              MultiBsdf* blendBsdf = ctx.make<MultiBsdf>();
              blendBsdf->add(plasticBsdf, diffuseRefl);
              blendBsdf->add(backlightBsdf, diffuseTrans);
              plasticBsdf = blendBsdf;
            }
            else
              plasticBsdf = backlightBsdf;
          }

          // Specular
          if ((abs(ior-outsideIor) > eps) && (specular > eps))
          {
            if (roughnessPar)
              plasticBsdf = ctx.make<RoughCoatingBsdf>(baseFrame, plasticBsdf, eta, Vec3f(1.f), 1.f, roughness, 0.f, specular);
            else
              plasticBsdf = ctx.make<CoatingBsdf>(baseFrame, plasticBsdf, eta, Vec3f(1.f), 1.f, specular);
          }

          baseBsdf->add(plasticBsdf, plastic);
        }
      }

      // Glass base
      float glass = dielectric * transmission * specular;
      if (glass > eps)
      {
        Bsdf* glassBsdf;

        if (abs(ior-outsideIor) > eps)
        {
          if (thin)
          {
            float thickness = max(thicknessPar.get(ctx), 0.f);
            if (roughnessPar)
              glassBsdf = ctx.make<ThinRoughDielectricBsdf>(baseFrame, eta, roughness, 0.f, transmissionColor.get(), thickness / transmissionDepth);
            else
              glassBsdf = ctx.make<ThinDielectricBsdf>(baseFrame, eta, transmissionColor.get(), thickness / transmissionDepth);
          }
          else
          {
            if (roughnessPar)
              glassBsdf = ctx.make<RoughDielectricBsdf>(baseFrame, eta, roughness, 0.f);
            else
              glassBsdf = ctx.make<DielectricBsdf>(baseFrame, eta);
          }
        }
        else
        {
          glassBsdf = ctx.make<TransparentBtdf>();
        }

        baseBsdf->add(glassBsdf, glass);
      }
    }

    // Conductor base
    if (wantOpaque)
    {
      float conductor = metallic * specular;
      if (conductor > eps)
      {
        Vec3f edgeColor = clamp(edgeColorPar.get(ctx));
        FresnelConductorArtistic<float> fresnel(baseColor, edgeColor);
        Bsdf* conductorBsdf;
        if (roughnessPar)
          conductorBsdf = ctx.make<RoughConductorBrdf<FresnelConductorArtistic<float>>>(baseFrame, fresnel, roughness, 0.f);
        else
          conductorBsdf = ctx.make<ConductorBrdf<FresnelConductorArtistic<float>>>(baseFrame, fresnel);
        baseBsdf->add(conductorBsdf, conductor);
      }
    }

    bsdf = baseBsdf;

    // Coatings
    const Basis3f* coatFrame = frame;

    // Clear coat
    float coat = clamp(coatPar.get(ctx));
    if ((coat > eps) && (abs(coatIor-outsideIor) > eps))
    {
      if (coatNormalPar)
        coatFrame = ctx.makeFrame(coatNormalPar.get(ctx), wo);

      float coatEta = fromOutside ? (outsideIor * rcp(coatIor)) : (coatIor * rcp(outsideIor));
      coatEta = clamp(coatEta, 1.f/3.f, 3.f); // clamp to common range due to LUTs

      Vec3f coatColor = clamp(coatColorPar.get(ctx));
      float coatThickness = max(coatThicknessPar.get(ctx), 0.f);
      float coatRoughness = clamp(coatRoughnessPar.get(ctx));

      if (coatRoughnessPar)
        bsdf = ctx.make<RoughCoatingBsdf>(coatFrame, bsdf, coatEta, coatColor, coatThickness, coatRoughness, 0.f, coat);
      else
        bsdf = ctx.make<CoatingBsdf>(coatFrame, bsdf, coatEta, coatColor, coatThickness, coat);
    }

    // Sheen
    float sheen = clamp(sheenPar.get(ctx));
    if (sheen > eps)
    {
      Vec3f sheenColor = clamp(sheenColorPar.get(ctx));
      float sheenTint = clamp(sheenTintPar.get(ctx));
      sheenColor = lerp(sheenColor, baseColor, sheenTint);
      float sheenRoughness = clamp(sheenRoughnessPar.get(ctx));

      bsdf = ctx.make<SheenBsdf>(coatFrame, bsdf, sheenColor, sheenRoughness, sheen);
    }
  }

  // Transparency
  if (transparency > eps)
  {
    Bsdf* transparencyBsdf = ctx.make<TransparentBtdf>();
    if (bsdf)
    {
      MultiBsdf* blendBsdf = ctx.make<MultiBsdf>();
      blendBsdf->add(bsdf, opacity);
      blendBsdf->add(transparencyBsdf, transparency);
      bsdf = blendBsdf;
    }
    else
      bsdf = transparencyBsdf;
  }

  return bsdf;
}

BsdfSimd* PrincipledMaterial::getBsdf(vbool m, ShadingContextSimd& ctx, const Vec3vf& wo) const
{
  return getBsdfLobes<true>(m, ctx, wo);
}

template <bool wantOpaque>
prt_inline BsdfSimd* PrincipledMaterial::getBsdfLobes(vbool m, ShadingContextSimd& ctx, const Vec3vf& wo) const
{
  BsdfSimd* bsdf = nullptr;

  vfloat opacity = clamp(opacityPar.get(m, ctx));
  vfloat transparency = 1.f - opacity;
  vbool transparencyMask = m & (transparency > eps);

  m &= opacity > eps;
  if (any(m))
  {
    const Basis3vf* frame = ctx.makeFrame(m, normalPar.get(m, ctx), wo);
    const Basis3vf* baseFrame = baseNormalPar ? ctx.makeFrame(m, baseNormalPar.get(m, ctx), wo) : frame;
    Vec3vf baseColor = clamp(baseColorPar.get(m, ctx));
    vfloat metallic = clamp(metallicPar.get(m, ctx));
    vfloat specular = clamp(specularPar.get(m, ctx));
    vfloat roughness = clamp(roughnessPar.get(m, ctx));
    vbool fromOutside = thin ? one : !ctx.backfacing;

    MultiBsdfSimd* baseBsdf = ctx.make<MultiBsdfSimd>();

    // Dielectric base
    vfloat dielectric = 1.f - metallic;
    vbool dielectricMask = m & (dielectric > eps);
    if (any(dielectricMask))
    {
      vfloat transmission = clamp(transmissionPar.get(dielectricMask, ctx));

      vfloat eta = select(fromOutside, vfloat(outsideIor * rcp(ior)), vfloat(ior * rcp(outsideIor)));
      eta = clamp(eta, vfloat(1.f/3.f), vfloat(3.f)); // clamp to common range due to LUTs

      // Plastic base
      if (wantOpaque)
      {
        vfloat plastic = dielectric * (1.f - transmission);
        vbool plasticMask = dielectricMask & (plastic > eps);
        if (any(plasticMask))
        {
          BsdfSimd* plasticBsdf = nullptr;

          // Diffuse
          vfloat diffuse = clamp(diffusePar.get(plasticMask, ctx));
          Vec3vf diffuseColor = baseColor * diffuse;
          vfloat backlight = thin ? clamp(backlightPar.get(plasticMask, ctx), vfloat(0.f), vfloat(2.f)) : vfloat(0.f);
          vfloat diffuseTrans = backlight * 0.5f;
          vfloat diffuseRefl = 1.f - diffuseTrans;

          vbool diffuseReflMask = plasticMask & (diffuseRefl > eps);
          if (any(diffuseReflMask))
          {
            if (roughnessPar)
              plasticBsdf = ctx.make<RoughDiffuseBrdfSimd>(diffuseReflMask, baseFrame, diffuseColor, roughness);
            else
              plasticBsdf = ctx.make<DiffuseBrdfSimd>(diffuseReflMask, baseFrame, diffuseColor);
          }

          vbool diffuseTransMask = plasticMask & (diffuseTrans > eps);
          if (any(diffuseTransMask))
          {
            BsdfSimd* backlightBsdf = ctx.make<DiffuseBtdfSimd>(diffuseTransMask, baseFrame, diffuseColor);
            if (plasticBsdf)
            {
              MultiBsdfSimd* blendBsdf = ctx.make<MultiBsdfSimd>();
              blendBsdf->add(diffuseReflMask, plasticBsdf, diffuseRefl);
              blendBsdf->add(diffuseTransMask, backlightBsdf, diffuseTrans);
              plasticBsdf = blendBsdf;
            }
            else
              plasticBsdf = backlightBsdf;
          }

          // Specular
          if ((abs(ior-outsideIor) > eps) && any(plasticMask & (specular > eps)))
          {
            if (roughnessPar)
              plasticBsdf = ctx.make<RoughCoatingBsdfSimd>(plasticMask, baseFrame, plasticBsdf, eta, Vec3f(1.f), 1.f, roughness, 0.f, specular);
            else
              plasticBsdf = ctx.make<CoatingBsdfSimd>(plasticMask, baseFrame, plasticBsdf, eta, Vec3f(1.f), 1.f, specular);
          }

          baseBsdf->add(plasticMask, plasticBsdf, plastic);
        }
      }

      // Glass base
      vfloat glass = dielectric * transmission * specular;
      vbool glassMask = dielectricMask & (glass > eps);
      if (any(glassMask))
      {
        BsdfSimd* glassBsdf;

        if (abs(ior-outsideIor) > eps)
        {
          if (thin)
          {
            vfloat thickness = max(thicknessPar.get(m, ctx), 0.f);
            if (roughnessPar)
              glassBsdf = ctx.make<ThinRoughDielectricBsdfSimd>(glassMask, baseFrame, eta, roughness, 0.f, transmissionColor.get(), thickness / transmissionDepth);
            else
              glassBsdf = ctx.make<ThinDielectricBsdfSimd>(glassMask, baseFrame, eta, transmissionColor.get(), thickness / transmissionDepth);
          }
          else
          {
            if (roughnessPar)
              glassBsdf = ctx.make<RoughDielectricBsdfSimd>(glassMask, baseFrame, eta, roughness, 0.f);
            else
              glassBsdf = ctx.make<DielectricBsdfSimd>(glassMask, baseFrame, eta);
          }
        }
        else
        {
          glassBsdf = ctx.make<TransparentBtdfSimd>();
        }

        baseBsdf->add(glassMask, glassBsdf, glass);
      }
    }

    // Conductor base
    if (wantOpaque)
    {
      vfloat conductor = metallic * specular;
      vbool conductorMask = m & (conductor > eps);
      if (any(conductorMask))
      {
        Vec3vf edgeColor = clamp(edgeColorPar.get(conductorMask, ctx));
        FresnelConductorArtistic<vfloat> fresnel(baseColor, edgeColor);
        BsdfSimd* conductorBsdf;
        if (roughnessPar)
          conductorBsdf = ctx.make<RoughConductorBrdfSimd<FresnelConductorArtistic<vfloat>>>(conductorMask, baseFrame, fresnel, roughness, 0.f);
        else
          conductorBsdf = ctx.make<ConductorBrdfSimd<FresnelConductorArtistic<vfloat>>>(conductorMask, baseFrame, fresnel);
        baseBsdf->add(conductorMask, conductorBsdf, conductor);
      }
    }

    bsdf = baseBsdf;

    // Coatings
    const Basis3vf* coatFrame = frame;

    // Clear coat
    vfloat coat = clamp(coatPar.get(m, ctx));
    if (any(m & (coat > eps)) && (abs(coatIor-outsideIor) > eps))
    {
      if (coatNormalPar)
        coatFrame = ctx.makeFrame(m, coatNormalPar.get(m, ctx), wo);

      vfloat coatEta = select(fromOutside, vfloat(outsideIor * rcp(coatIor)), vfloat(coatIor * rcp(outsideIor)));
      coatEta = clamp(coatEta, vfloat(1.f/3.f), vfloat(3.f)); // clamp to common range due to LUTs

      Vec3vf coatColor = clamp(coatColorPar.get(m, ctx));
      vfloat coatThickness = max(coatThicknessPar.get(m, ctx), 0.f);
      vfloat coatRoughness = clamp(coatRoughnessPar.get(m, ctx));

      if (coatRoughnessPar)
        bsdf = ctx.make<RoughCoatingBsdfSimd>(m, coatFrame, bsdf, coatEta, coatColor, coatThickness, coatRoughness, 0.f, coat);
      else
        bsdf = ctx.make<CoatingBsdfSimd>(m, coatFrame, bsdf, coatEta, coatColor, coatThickness, coat);
    }

    // Sheen
    vfloat sheen = clamp(sheenPar.get(m, ctx));
    if (any(m & (sheen > eps)))
    {
      Vec3vf sheenColor = clamp(sheenColorPar.get(m, ctx));
      vfloat sheenTint = clamp(sheenTintPar.get(m, ctx));
      sheenColor = lerp(sheenColor, baseColor, sheenTint);
      vfloat sheenRoughness = clamp(sheenRoughnessPar.get(m, ctx));

      bsdf = ctx.make<SheenBsdfSimd>(m, coatFrame, bsdf, sheenColor, sheenRoughness, sheen);
    }
  }

  // Transparency
  if (any(transparencyMask))
  {
    BsdfSimd* transparencyBsdf = ctx.make<TransparentBtdfSimd>();
    if (bsdf)
    {
      MultiBsdfSimd* blendBsdf = ctx.make<MultiBsdfSimd>();
      blendBsdf->add(m, bsdf, opacity);
      blendBsdf->add(transparencyMask, transparencyBsdf, transparency);
      bsdf = blendBsdf;
    }
    else
      bsdf = transparencyBsdf;
  }

  return bsdf;
}

void PrincipledMaterial::mutate(Random& rng)
{
#ifdef PRT_MUTATION_SUPPORT
  baseColorPar.mutate(rng);
  edgeColorPar.mutate(rng);
  if (metallicPar != 0.f && metallicPar != 1.f)
    metallicPar.mutate(rng, 0.f, 1.f);
  if (specularPar != 0.f && specularPar != 1.f)
    specularPar.mutate(rng, 0.f, 1.f);

  float maxRoughness = (rng.get1f() < 0.35f) ? (transmissionPar ? 0.1f : 0.4f) : (transmissionPar ? 0.8f : 1.f);
  if (roughnessPar || !transmissionPar)
    maxRoughness = roughnessPar.mutate(rng, 0.f, maxRoughness) * 0.8f;

  if ((abs(iorOrg-outsideIor) > eps && rng.get1f() < 0.9f) || (!coatPar && rng.get1f() < 0.1f))
  {
    const float minIor = 1.f;
    const float maxIor = max(2.f, iorOrg);
    ior = lerp(minIor, maxIor, rng.get1f()) * outsideIor;
  }
  else
    ior = outsideIor;

  if (transmissionPar)
  {
    transmissionColor.mutate(rng);

    if (mediumInt)
    {
      mediumInt->color = pow(transmissionColor.get(), transmissionDepth);
      mediumInt->ior = ior;
    }
  }

  if (coatPar && abs(coatIorOrg-outsideIor) > eps)
  {
    const float minCoatIor = 1.f;
    const float maxCoatIor = max(2.f, coatIorOrg);
    coatIor = lerp(minCoatIor, maxCoatIor, rng.get1f()) * outsideIor;
    coatColorPar.mutate(rng);

    maxRoughness = min((rng.get1f() < 0.5f) ? 0.15f : 0.5f, maxRoughness);
    maxRoughness = coatRoughnessPar.mutate(rng, 0.f, maxRoughness) * 0.8f;
  }

  if (sheenPar)
  {
    sheenPar.mutate(rng, 0.f, 1.f);
    sheenColorPar.mutate(rng);
    sheenTintPar.mutate(rng, 0.f, 1.f);
    sheenRoughnessPar.mutate(rng, 0.1f, 1.f);
  }

  if (thin && backlightPar)
    backlightPar.mutate(rng, 0.f, 1.f);

  type &= ~MaterialType::FirstHitAov;
  if (rng.get1f() < 0.4f)
    type |= MaterialType::FirstHitAov;
#endif
}

Vec3f PrincipledMaterial::getTransparency(ShadingContext& ctx, const Vec3f& wo) const
{
  if (type & MaterialType::Transmissive)
  {
    ctx.begin();
    Bsdf* bsdf = getBsdfLobes<false>(ctx, wo);
    return bsdf->getTransparency(ctx, wo);
  }
  else
  {
    float opacity = clamp(opacityPar.get(ctx));
    return 1.f - opacity;
  }
}

Vec3vf PrincipledMaterial::getTransparency(vbool m, ShadingContextSimd& ctx, const Vec3vf& wo) const
{
  if (type & MaterialType::Transmissive)
  {
    ctx.begin();
    BsdfSimd* bsdf = getBsdfLobes<false>(m, ctx, wo);
    return bsdf->getTransparency(m, ctx, wo);
  }
  else
  {
    vfloat opacity = clamp(opacityPar.get(m, ctx));
    return vfloat(1.f) - opacity;
  }
}

} // namespace prt
