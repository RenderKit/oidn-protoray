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
#include "standard.h"

namespace prt {

StandardMaterial::StandardMaterial(const Props& props, MaterialFactory& factory,
                                   StandardMaterialMode mode)
  : Material(props, factory),
    mode(mode)
{
  if (mode == StandardMaterialMode::Default)
  {
    // Ambient
    ambientIor = max(props.get("ambientIOR", 1.f), 0.f);
    if (ambientIor < 1.f) ambientIor = rcp(ambientIor);

    // Base
    baseWeightPar = getParam1f("baseWeight", 1.f);
    baseColorPar = getParam3f("baseColor", Vec3f(0.8f));
    metalnessPar = getParam1f("metalness", 0.f);
    diffuseRoughnessPar = getParam1f("diffuseRoughness", 0.f);
    normalPar = getParamNormal("normal", "bump");

    // Specular
    specularWeightPar = getParam1f("specularWeight", 1.f);
    specularColorPar = getParam3f("specularColor", Vec3f(1.f));
    specularRoughnessPar = getParam1f("specularRoughness", 0.3f);
    specularIor = max(props.get("specularIOR", 1.5f), 0.f);
    if (specularIor < 1.f) specularIor = rcp(specularIor);

    // Transmission
    transmissionWeightPar = getParam1f("transmissionWeight", 0.f);
    transmissionColorPar = clamp(props.get("transmissionColor", Vec3f(1.f)));
    transmissionDepth = max(props.get("transmissionDepth", 1.f), 0.f);

    // Subsurface
    subsurfaceWeightPar = getParam1f("subsurfaceWeight", 0.f);
    subsurfaceColorPar = getParam3f("subsurfaceColor", Vec3f(0.8f));
    subsurfaceAnisotropyPar = getParam1f("subsurfaceAnisotropy", 0.f);

    // Coat
    coatWeightPar = getParam1f("coatWeight", 0.f);
    coatColorPar = getParam3f("coatColor", 1.f);
    coatRoughnessPar = getParam1f("coatRoughness", 0.f);
    coatNormalPar = getParamNormal("coatNormal", "coatBump");
    coatIor = max(props.get("coatIOR", 1.6f), 0.f);
    if (coatIor < 1.f) coatIor = rcp(coatIor);
    coatThicknessPar.scale = 0.5f;

    // Sheen
    sheenWeightPar = getParam1f("sheenWeight", 0.f);
    sheenColorPar = getParam3f("sheenColor", 1.f);
    sheenRoughnessPar = getParam1f("sheenRoughness", 0.3f);

    // Emission
    emissionLuminance = props.get("emissionLuminance", 0.f);
    emissionColor = props.get("emissionColor", Vec3f(1.f));
    if (emissionLuminance > 0.f && factory.isEmissiveEnabled())
      type |= MaterialType::Emissive;

    // Geometry
    thinWalled = props.get("thinWalled", 0);
    thicknessPar.scale = 1.f;
    opacityPar = getParamAlpha("opacity", 1.f);
  }
  else if (mode == StandardMaterialMode::Legacy)
  {
    // Outside
    ambientIor = max(props.get("outsideIor", 1.f), 0.f);
    if (ambientIor < 1.f) ambientIor = rcp(ambientIor);

    // Base
    baseColorPar = getParam3f("baseColor", Vec3f(0.8f));
    baseWeightPar = getParam1f("diffuse", 1.f);
    metalnessPar = getParam1f("metallic", 0.f);
    diffuseRoughnessPar = getParam1f("roughness", 0.f);
    normalPar = getParamNormal("normal", "bump");

    // Specular
    specularWeightPar = getParam1f("specular", 1.f);
    specularColorPar = getParam3f("edgeColor", Vec3f(1.f));
    specularRoughnessPar = getParam1f("roughness", 0.f);
    specularIor = max(props.get("ior", ambientIor), 0.f);
    if (specularIor < 1.f) specularIor = rcp(specularIor);

    // Transmission
    transmissionWeightPar = getParam1f("transmission", 0.f);
    transmissionColorPar = clamp(props.get("transmissionColor", Vec3f(1.f)));
    transmissionDepth = max(props.get("transmissionDepth", 1.f), 0.f);

    // Subsurface
    float backlight = props.get("backlight", 0.f);
    if (backlight > 0.f)
    {
      subsurfaceWeightPar.scale = 1.f;
      subsurfaceColorPar = baseColorPar;
      subsurfaceAnisotropyPar.scale = backlight - 1.f;
    }

    // Coat
    coatWeightPar = getParam1f("coat", 0.f);
    coatColorPar = getParam3f("coatColor", 1.f);
    coatRoughnessPar = getParam1f("coatRoughness", 0.f);
    coatNormalPar = getParamNormal("coatNormal", "coatBump");
    coatIor = max(props.get("coatIor", 1.5f), 0.f);
    if (coatIor < 1.f) coatIor = rcp(coatIor);
    coatThicknessPar = getParam1f("coatThickness", 1.f);

    if (!coatNormalPar)
      coatNormalPar = normalPar;

    MaterialParamNormal baseNormalPar = getParamNormal("baseNormal", "baseBump");
    if (baseNormalPar)
      normalPar = baseNormalPar;

    // Sheen
    sheenWeightPar = getParam1f("sheen", 0.f);
    sheenColorPar = getParam3f("sheenColor", 1.f);
    sheenRoughnessPar = getParam1f("sheenRoughness", 0.2f);
    sheenTintPar = getParam1f("sheenTint", 0.f);

    // Emission (not supported)
    emissionLuminance = 0.f;
    emissionColor = Vec3f(0.f);

    // Geometry
    thinWalled = props.get("thin", 0);
    thicknessPar = getParam1f("thickness", 1.f);

    opacityPar = getParamAlpha("opacity", 1.f);
    if (opacityPar.isUniform() && baseColorPar.texture && baseColorPar.texture->hasAlpha())
      opacityPar = getParamAlpha("baseColor", 1.f, false);
  }
  else
    throw std::logic_error("unknown StandardMaterialMode");

  if (opacityPar != 1.f)
  {
    type |= MaterialType::Transparent;
    if (!opacityPar.isUniform())
      type |= MaterialType::Cutout;
  }

  if (transmissionWeightPar)
  {
    type |= MaterialType::Transmissive;

    if (thinWalled)
    {
      type |= MaterialType::Transparent;
    }
    else
    {
      type |= MaterialType::MediaInterface;

      Vec3f mediumColor = pow(transmissionColorPar.get(), transmissionDepth);
      mediumInt = factory.makeMedium(mediumColor, specularIor, mediumIntId);
      mediumExt = factory.makeMedium(one, ambientIor, mediumExtId);
    }
  }

#ifdef PRT_MUTATION_SUPPORT
  // Save original values
  specularIorOrg = specularIor;
  coatIorOrg = coatIor;
  emissionColorOrg = emissionColor;
#endif
}

Bsdf* StandardMaterial::getBsdf(ShadingContext& ctx, const Vec3f& wo) const
{
  return getBsdfLobes<true>(ctx, wo);
}

template <bool wantOpaque>
prt_inline Bsdf* StandardMaterial::getBsdfLobes(ShadingContext& ctx, const Vec3f& wo) const
{
  Bsdf* bsdf = nullptr;

  float opacity = clamp(opacityPar.get(ctx));
  float transparency = 1.f - opacity;

  if (opacity > eps)
  {
    Vec3f normal = normalPar.get(ctx);
    const Basis3f* frame = ctx.makeFrame(normal, wo);
    float base = clamp(baseWeightPar.get(ctx));
    Vec3f baseColor = clamp(baseColorPar.get(ctx));
    float metalness = clamp(metalnessPar.get(ctx));
    float specular = clamp(specularWeightPar.get(ctx));
    float specularRoughness = clamp(specularRoughnessPar.get(ctx));
    bool fromOutside = thinWalled ? true : !ctx.backfacing;

    MultiBsdf* baseBsdf = ctx.make<MultiBsdf>();

    // Dielectric base
    float dielectric = 1.f - metalness;
    if (dielectric > eps)
    {
      float transmission = clamp(transmissionWeightPar.get(ctx));

      float eta = fromOutside ? (ambientIor * rcp(specularIor)) : (specularIor * rcp(ambientIor));
      eta = clamp(eta, 1.f/3.f, 3.f); // clamp to common range due to LUTs

      // Opaque base
      if (wantOpaque)
      {
        float opaque = dielectric * (1.f - transmission);
        if (opaque > eps)
        {
          Bsdf* opaqueBsdf = nullptr;
          float subsurface = clamp(subsurfaceWeightPar.get(ctx));
          float diffuse = 1.f - subsurface;
          float diffuseRoughness = clamp(diffuseRoughnessPar.get(ctx));

          // Diffuse
          if (diffuse > eps)
          {
            Vec3f diffuseColor = baseColor * base;

            if (diffuseRoughnessPar)
              opaqueBsdf = ctx.make<RoughDiffuseBrdf>(frame, diffuseColor, diffuseRoughness);
            else
              opaqueBsdf = ctx.make<DiffuseBrdf>(frame, diffuseColor);
          }

          // Subsurface (not real subsurface scattering, always assumes thin-walled geometry)
          if (subsurface > eps)
          {
            Bsdf* subsurfaceBsdf = nullptr;
            Vec3f subsurfaceColor = clamp(subsurfaceColorPar.get(ctx));
            float subsurfaceAnisotropy = clamp(subsurfaceAnisotropyPar.get(ctx), -1.f, 1.f);

            float subsurfaceTrans = (1.f + subsurfaceAnisotropy) * 0.5f;
            float subsurfaceRefl = 1.f - subsurfaceTrans;

            if (subsurfaceRefl > eps)
            {
              if (diffuseRoughnessPar)
                subsurfaceBsdf = ctx.make<RoughDiffuseBrdf>(frame, subsurfaceColor, diffuseRoughness);
              else
                subsurfaceBsdf = ctx.make<DiffuseBrdf>(frame, subsurfaceColor);
            }

            if (subsurfaceTrans > eps)
            {
              Bsdf* subsurfaceTransBsdf = ctx.make<DiffuseBtdf>(frame, subsurfaceColor);
              if (subsurfaceBsdf)
              {
                MultiBsdf* blendBsdf = ctx.make<MultiBsdf>();
                blendBsdf->add(subsurfaceBsdf, subsurfaceRefl);
                blendBsdf->add(subsurfaceTransBsdf, subsurfaceTrans);
                subsurfaceBsdf = blendBsdf;
              }
              else
                subsurfaceBsdf = subsurfaceTransBsdf;
            }

            if (opaqueBsdf)
            {
              MultiBsdf* blendBsdf = ctx.make<MultiBsdf>();
              blendBsdf->add(opaqueBsdf, diffuse);
              blendBsdf->add(subsurfaceBsdf, subsurface);
              opaqueBsdf = blendBsdf;
            }
            else
              opaqueBsdf = subsurfaceBsdf;
          }

          // Specular
          if ((abs(specularIor-ambientIor) > eps) && (specular > eps))
          {
            if (specularRoughnessPar)
              opaqueBsdf = ctx.make<RoughCoatingBsdf>(frame, opaqueBsdf, eta, Vec3f(1.f), 1.f, specularRoughness, 0.f, specular);
            else
              opaqueBsdf = ctx.make<CoatingBsdf>(frame, opaqueBsdf, eta, Vec3f(1.f), 1.f, specular);
          }

          baseBsdf->add(opaqueBsdf, opaque);
        }
      }

      // Translucent base
      float translucent = dielectric * transmission * specular;
      if (translucent > eps)
      {
        Bsdf* translucentBsdf;

        if (abs(specularIor-ambientIor) > eps)
        {
          if (thinWalled)
          {
            float thickness = max(thicknessPar.get(ctx), 0.f);
            if (specularRoughnessPar)
              translucentBsdf = ctx.make<ThinRoughDielectricBsdf>(frame, eta, specularRoughness, 0.f, transmissionColorPar.get(), thickness / transmissionDepth);
            else
              translucentBsdf = ctx.make<ThinDielectricBsdf>(frame, eta, transmissionColorPar.get(), thickness / transmissionDepth);
          }
          else
          {
            if (specularRoughnessPar)
              translucentBsdf = ctx.make<RoughDielectricBsdf>(frame, eta, specularRoughness, 0.f);
            else
              translucentBsdf = ctx.make<DielectricBsdf>(frame, eta);
          }
        }
        else
        {
          translucentBsdf = ctx.make<TransparentBtdf>();
        }

        baseBsdf->add(translucentBsdf, translucent);
      }
    }

    // Metal base
    if (wantOpaque)
    {
      float metal = metalness * specular;
      if (metal > eps)
      {
        Vec3f specularColor = clamp(specularColorPar.get(ctx));
        FresnelConductorArtistic<float> fresnel(baseColor * base, specularColor);
        Bsdf* metalBsdf;
        if (specularRoughnessPar)
          metalBsdf = ctx.make<RoughConductorBrdf<FresnelConductorArtistic<float>>>(frame, fresnel, specularRoughness, 0.f);
        else
          metalBsdf = ctx.make<ConductorBrdf<FresnelConductorArtistic<float>>>(frame, fresnel);
        baseBsdf->add(metalBsdf, metal);
      }
    }

    bsdf = baseBsdf;

    // Coat
    float coat = clamp(coatWeightPar.get(ctx));
    Vec3f coatNormal = coatNormalPar.get(ctx);
    if ((coat > eps) && (abs(coatIor-ambientIor) > eps))
    {
      const Basis3f* coatFrame = ctx.makeFrame(coatNormal, wo);

      float coatEta = fromOutside ? (ambientIor * rcp(coatIor)) : (coatIor * rcp(ambientIor));
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
    float sheen = clamp(sheenWeightPar.get(ctx));
    if (sheen > eps)
    {
      Vec3f sheenNormal = normalize(lerp(normal, coatNormal, coat));
      const Basis3f* sheenFrame = ctx.makeFrame(sheenNormal, wo);

      Vec3f sheenColor = clamp(sheenColorPar.get(ctx));
      float sheenTint = clamp(sheenTintPar.get(ctx));
      sheenColor = lerp(sheenColor, baseColor, sheenTint);
      float sheenRoughness = clamp(sheenRoughnessPar.get(ctx));

      bsdf = ctx.make<SheenBsdf>(sheenFrame, bsdf, sheenColor, sheenRoughness, sheen);
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

BsdfSimd* StandardMaterial::getBsdf(vbool m, ShadingContextSimd& ctx, const Vec3vf& wo) const
{
  return getBsdfLobes<true>(m, ctx, wo);
}

template <bool wantOpaque>
prt_inline BsdfSimd* StandardMaterial::getBsdfLobes(vbool m, ShadingContextSimd& ctx, const Vec3vf& wo) const
{
  BsdfSimd* bsdf = nullptr;

  vfloat opacity = clamp(opacityPar.get(m, ctx));
  vfloat transparency = 1.f - opacity;
  vbool transparencyMask = m & (transparency > eps);

  m &= opacity > eps;
  if (any(m))
  {
    Vec3vf normal = normalPar.get(m, ctx);
    const Basis3vf* frame = ctx.makeFrame(m, normal, wo);
    vfloat base = clamp(baseWeightPar.get(m, ctx));
    Vec3vf baseColor = clamp(baseColorPar.get(m, ctx));
    vfloat metalness = clamp(metalnessPar.get(m, ctx));
    vfloat specular = clamp(specularWeightPar.get(m, ctx));
    vfloat specularRoughness = clamp(specularRoughnessPar.get(m, ctx));
    vbool fromOutside = thinWalled ? one : !ctx.backfacing;

    MultiBsdfSimd* baseBsdf = ctx.make<MultiBsdfSimd>();

    // Dielectric base
    vfloat dielectric = 1.f - metalness;
    vbool dielectricMask = m & (dielectric > eps);
    if (any(dielectricMask))
    {
      vfloat transmission = clamp(transmissionWeightPar.get(dielectricMask, ctx));

      vfloat eta = select(fromOutside, vfloat(ambientIor * rcp(specularIor)), vfloat(specularIor * rcp(ambientIor)));
      eta = clamp(eta, vfloat(1.f/3.f), vfloat(3.f)); // clamp to common range due to LUTs

      // Opaque base
      if (wantOpaque)
      {
        vfloat opaque = dielectric * (1.f - transmission);
        vbool opaqueMask = dielectricMask & (opaque > eps);
        if (any(opaqueMask))
        {
          BsdfSimd* opaqueBsdf = nullptr;
          vfloat subsurface = clamp(subsurfaceWeightPar.get(opaqueMask, ctx));
          vfloat diffuse = 1.f - subsurface;
          vfloat diffuseRoughness = clamp(diffuseRoughnessPar.get(opaqueMask, ctx));

          // Diffuse
          vbool diffuseMask = opaqueMask & (diffuse > eps);
          if (any(diffuseMask))
          {
            Vec3vf diffuseColor = baseColor * base;

            if (diffuseRoughnessPar)
              opaqueBsdf = ctx.make<RoughDiffuseBrdfSimd>(diffuseMask, frame, diffuseColor, diffuseRoughness);
            else
              opaqueBsdf = ctx.make<DiffuseBrdfSimd>(diffuseMask, frame, diffuseColor);
          }

          // Subsurface (not real subsurface scattering, always assumes thin-walled geometry)
          vbool subsurfaceMask = opaqueMask & (subsurface > eps);
          if (any(subsurfaceMask))
          {
            BsdfSimd* subsurfaceBsdf = nullptr;
            Vec3vf subsurfaceColor = clamp(subsurfaceColorPar.get(subsurfaceMask, ctx));
            vfloat subsurfaceAnisotropy = clamp(subsurfaceAnisotropyPar.get(subsurfaceMask, ctx), vfloat(-1.f), vfloat(1.f));

            vfloat subsurfaceTrans = (1.f + subsurfaceAnisotropy) * 0.5f;
            vfloat subsurfaceRefl = 1.f - subsurfaceTrans;

            vbool subsurfaceReflMask = subsurfaceMask & (subsurfaceRefl > eps);
            if (any(subsurfaceReflMask))
            {
              if (diffuseRoughnessPar)
                subsurfaceBsdf = ctx.make<RoughDiffuseBrdfSimd>(subsurfaceReflMask, frame, subsurfaceColor, diffuseRoughness);
              else
                subsurfaceBsdf = ctx.make<DiffuseBrdfSimd>(subsurfaceReflMask, frame, subsurfaceColor);
            }

            vbool subsurfaceTransMask = subsurfaceMask & (subsurfaceTrans > eps);
            if (any(subsurfaceTransMask))
            {
              BsdfSimd* subsurfaceTransBsdf = ctx.make<DiffuseBtdfSimd>(subsurfaceTransMask, frame, subsurfaceColor);
              if (subsurfaceBsdf)
              {
                MultiBsdfSimd* blendBsdf = ctx.make<MultiBsdfSimd>();
                blendBsdf->add(subsurfaceReflMask, subsurfaceBsdf, subsurfaceRefl);
                blendBsdf->add(subsurfaceTransMask, subsurfaceTransBsdf, subsurfaceTrans);
                subsurfaceBsdf = blendBsdf;
              }
              else
                subsurfaceBsdf = subsurfaceTransBsdf;
            }

            if (opaqueBsdf)
            {
              MultiBsdfSimd* blendBsdf = ctx.make<MultiBsdfSimd>();
              blendBsdf->add(diffuseMask, opaqueBsdf, diffuse);
              blendBsdf->add(subsurfaceMask, subsurfaceBsdf, subsurface);
              opaqueBsdf = blendBsdf;
            }
            else
              opaqueBsdf = subsurfaceBsdf;
          }

          // Specular
          if ((abs(specularIor-ambientIor) > eps) && any(opaqueMask & (specular > eps)))
          {
            if (specularRoughnessPar)
              opaqueBsdf = ctx.make<RoughCoatingBsdfSimd>(opaqueMask, frame, opaqueBsdf, eta, Vec3f(1.f), 1.f, specularRoughness, 0.f, specular);
            else
              opaqueBsdf = ctx.make<CoatingBsdfSimd>(opaqueMask, frame, opaqueBsdf, eta, Vec3f(1.f), 1.f, specular);
          }

          baseBsdf->add(opaqueMask, opaqueBsdf, opaque);
        }
      }

      // Translucent base
      vfloat translucent = dielectric * transmission * specular;
      vbool translucentMask = dielectricMask & (translucent > eps);
      if (any(translucentMask))
      {
        BsdfSimd* translucentBsdf;

        if (abs(specularIor-ambientIor) > eps)
        {
          if (thinWalled)
          {
            vfloat thickness = max(thicknessPar.get(m, ctx), 0.f);
            if (specularRoughnessPar)
              translucentBsdf = ctx.make<ThinRoughDielectricBsdfSimd>(translucentMask, frame, eta, specularRoughness, 0.f, transmissionColorPar.get(), thickness / transmissionDepth);
            else
              translucentBsdf = ctx.make<ThinDielectricBsdfSimd>(translucentMask, frame, eta, transmissionColorPar.get(), thickness / transmissionDepth);
          }
          else
          {
            if (specularRoughnessPar)
              translucentBsdf = ctx.make<RoughDielectricBsdfSimd>(translucentMask, frame, eta, specularRoughness, 0.f);
            else
              translucentBsdf = ctx.make<DielectricBsdfSimd>(translucentMask, frame, eta);
          }
        }
        else
        {
          translucentBsdf = ctx.make<TransparentBtdfSimd>();
        }

        baseBsdf->add(translucentMask, translucentBsdf, translucent);
      }
    }

    // Metal base
    if (wantOpaque)
    {
      vfloat metal = metalness * specular;
      vbool metalMask = m & (metal > eps);
      if (any(metalMask))
      {
        Vec3vf specularColor = clamp(specularColorPar.get(metalMask, ctx));
        FresnelConductorArtistic<vfloat> fresnel(baseColor * base, specularColor);
        BsdfSimd* metalBsdf;
        if (specularRoughnessPar)
          metalBsdf = ctx.make<RoughConductorBrdfSimd<FresnelConductorArtistic<vfloat>>>(metalMask, frame, fresnel, specularRoughness, 0.f);
        else
          metalBsdf = ctx.make<ConductorBrdfSimd<FresnelConductorArtistic<vfloat>>>(metalMask, frame, fresnel);
        baseBsdf->add(metalMask, metalBsdf, metal);
      }
    }

    bsdf = baseBsdf;

    // Coat
    vfloat coat = clamp(coatWeightPar.get(m, ctx));
    Vec3vf coatNormal = coatNormalPar.get(m, ctx);
    if (any(m & (coat > eps)) && (abs(coatIor-ambientIor) > eps))
    {
      const Basis3vf* coatFrame = ctx.makeFrame(m, coatNormal, wo);

      vfloat coatEta = select(fromOutside, vfloat(ambientIor * rcp(coatIor)), vfloat(coatIor * rcp(ambientIor)));
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
    vfloat sheen = clamp(sheenWeightPar.get(m, ctx));
    if (any(m & (sheen > eps)))
    {
      Vec3vf sheenNormal = normalize(lerp(normal, coatNormal, coat));
      const Basis3vf* sheenFrame = ctx.makeFrame(m, sheenNormal, wo);

      Vec3vf sheenColor = clamp(sheenColorPar.get(m, ctx));
      vfloat sheenTint = clamp(sheenTintPar.get(m, ctx));
      sheenColor = lerp(sheenColor, baseColor, sheenTint);
      vfloat sheenRoughness = clamp(sheenRoughnessPar.get(m, ctx));

      bsdf = ctx.make<SheenBsdfSimd>(m, sheenFrame, bsdf, sheenColor, sheenRoughness, sheen);
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

Vec3f StandardMaterial::getTransparency(ShadingContext& ctx, const Vec3f& wo) const
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

Vec3vf StandardMaterial::getTransparency(vbool m, ShadingContextSimd& ctx, const Vec3vf& wo) const
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

Vec3f StandardMaterial::getEmissiveColor() const
{
  return emissionColor * emissionLuminance;
}

void StandardMaterial::mutate(Random& rng)
{
#ifdef PRT_MUTATION_SUPPORT
  baseColorPar.mutate(rng);
  specularColorPar.mutate(rng);
  if (metalnessPar != 0.f && metalnessPar != 1.f)
    metalnessPar.mutate(rng, 0.f, 1.f);
  diffuseRoughnessPar.mutate(rng, 0.f, 1.f);

  if (specularWeightPar != 0.f && specularWeightPar != 1.f)
    specularWeightPar.mutate(rng, 0.f, 1.f);
  float maxRoughness = (rng.get1f() < 0.35f) ? (transmissionWeightPar ? 0.1f : 0.4f) : (transmissionWeightPar ? 0.8f : 1.f);
  if (specularRoughnessPar || !transmissionWeightPar)
    maxRoughness = specularRoughnessPar.mutate(rng, 0.f, maxRoughness) * 0.8f;

  if ((abs(specularIorOrg-ambientIor) > eps && rng.get1f() < 0.9f) || (!coatWeightPar && rng.get1f() < 0.1f))
  {
    const float minIor = 1.f;
    const float maxIor = max(2.f, specularIorOrg);
    specularIor = lerp(minIor, maxIor, rng.get1f()) * ambientIor;
  }
  else
    specularIor = ambientIor;

  if (transmissionWeightPar)
  {
    transmissionColorPar.mutate(rng);

    if (mediumInt)
    {
      mediumInt->color = pow(transmissionColorPar.get(), transmissionDepth);
      mediumInt->ior = specularIor;
    }
  }

  if (subsurfaceWeightPar)
  {
    if (subsurfaceWeightPar != 1.f)
      subsurfaceWeightPar.mutate(rng, 0.f, 1.f);
    subsurfaceColorPar.mutate(rng);
    subsurfaceAnisotropyPar.mutate(rng, -1.f, 1.f);
  }

  if (coatWeightPar && abs(coatIorOrg-ambientIor) > eps)
  {
    const float minCoatIor = 1.f;
    const float maxCoatIor = max(2.f, coatIorOrg);
    coatIor = lerp(minCoatIor, maxCoatIor, rng.get1f()) * ambientIor;
    coatColorPar.mutate(rng);

    maxRoughness = min((rng.get1f() < 0.5f) ? 0.15f : 0.5f, maxRoughness);
    maxRoughness = coatRoughnessPar.mutate(rng, 0.f, maxRoughness) * 0.8f;
  }

  if (sheenWeightPar)
  {
    sheenWeightPar.mutate(rng, 0.f, 1.f);
    sheenColorPar.mutate(rng);
    sheenTintPar.mutate(rng, 0.f, 1.f);
    sheenRoughnessPar.mutate(rng, 0.1f, 1.f);
  }

  if (emissionLuminance > 0.f)
  {
    float saturation = 0.f;
    float hue = 0.f;

    if (rng.get1f() < 0.8f)
    {
      // Colored light
      saturation = (rng.get1f() < 0.7f) ? rng.get1f() : rng.get1f(0.f, 0.5f);
      hue = rng.get1f();
    }

    float scale = rng.get1f(0.f, 2.f);

    emissionColor = rgbToHsv(emissionColorOrg);
    emissionColor.x += hue;
    emissionColor.y = saturation;
    emissionColor = hsvToRgb(emissionColor);
    emissionColor *= scale;
  }

  type &= ~MaterialType::FirstHitAov;
  if (rng.get1f() < 0.4f)
    type |= MaterialType::FirstHitAov;
#endif
}

} // namespace prt
