// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "optics.h"
#include "bsdf.h"
#include "ggx_distribution.h"
#include "albedo_tables.h"

namespace prt {

// Microfacet conductor BRDF with the Smith microsurface model and approximate multiple scattering.
// [Kulla and Conty, 2017, "Revisiting Physically Based Shading at Imageworks"]
// [Jakob et al., 2014, "A Comprehensive Framework for Rendering Layered Materials", Extended Technical Report]
template <class Fresnel>
class RoughConductorBrdf : public Bsdf
{
private:
  Fresnel fresnel;
  GGXDistribution<float> microfacet;
  float roughness;

  // Energy compensation [Kulla and Conty, 2017]
  float Eavg;
  Vec3f fmsScale;

public:
  prt_inline RoughConductorBrdf(const Basis3f* frame, const Fresnel& fresnel, float roughness, float anisotropy)
    : Bsdf(frame, BsdfType::GlossyReflection),
      fresnel(fresnel),
      microfacet(roughnessToAlpha(roughness, anisotropy)),
      roughness(roughness)
  {
    // Energy compensation
    Eavg = MicrofacetAlbedoTable::evalAvg(roughness);
    Vec3f Favg = fresnel.evalAvg();
    fmsScale = sqr(Favg) * Eavg / (1.f - Favg * (1.f - Eavg)); // Stephen Hill's tweak
  }

  Vec3f eval(const ShadingContext& ctx, const Vec3f& wo, const Vec3f& wi, float& pdf) const
  {
    float cosThetaO = dot(wo, getN());
    float cosThetaI = dot(wi, getN());
    if (cosThetaO <= 0.f || cosThetaI <= 0.f)
    {
      pdf = zero;
      return zero;
    }

    // Compute the microfacet normal
    Vec3f wh = normalize(wi + wo);
    float cosThetaH = dot(wh, getN());
    float cosThetaOH = dot(wo, wh);

    Vec3f wo0 = getFrame().toLocal(wo);
    Vec3f wi0 = getFrame().toLocal(wi);
    Vec3f wh0 = getFrame().toLocal(wh);

    Vec3f F = fresnel.eval(cosThetaOH);
    float whPdf;
    float D = microfacet.evalVisible(wh0, wo0, cosThetaOH, whPdf);
    float G = microfacet.evalG2(wo0, wi0);

    // Energy compensation
    float Eo = MicrofacetAlbedoTable::eval(cosThetaO, roughness);
    float Ei = MicrofacetAlbedoTable::eval(cosThetaI, roughness);
    Vec3f fms = fmsScale * ((1.f - Eo) * (1.f - Ei) * rcp(float(pi) * (1.f - Eavg)) * cosThetaI);

    pdf = whPdf * rcp(4.f*abs(cosThetaOH));
    return F * (D * G * rcp(4.f*cosThetaO)) + fms;
  }

  Vec3f sample(const ShadingContext& ctx, const Vec3f& wo, Vec3f& wi, float& pdf, int& type, const BsdfSample& s) const
  {
    float cosThetaO = dot(wo, getN());
    if (cosThetaO <= 0.f)
      return zero;

    Vec3f wo0 = getFrame().toLocal(wo);

    // Sample the microfacet normal
    float whPdf;
    Vec3f wh = getFrame().toGlobal(microfacet.sampleVisible(wo0, whPdf, s.v));

    wi = reflect(wo, wh);
    float cosThetaI = dot(wi, getN());
    if ((cosThetaI <= 0.f) || (dot(wi, ctx.Ng) <= 0.f))
      return zero;
    float cosThetaOH = dot(wo, wh);
    Vec3f wi0 = getFrame().toLocal(wi);

    Vec3f F = fresnel.eval(cosThetaOH);
    float G = microfacet.evalG2(wo0, wi0);

    // Energy compensation
    float Eo = MicrofacetAlbedoTable::eval(cosThetaO, roughness);
    float Ei = MicrofacetAlbedoTable::eval(cosThetaI, roughness);
    Vec3f fms = fmsScale * ((1.f - Eo) * (1.f - Ei) * rcp(float(pi) * (1.f - Eavg)) * cosThetaI);

    type = BsdfType::GlossyReflection;
    pdf = whPdf * rcp(4.f*abs(cosThetaOH));
    return F * (G * rcpSafe(microfacet.evalG1(wo0))) + (fms * rcp(pdf));
  }
};

template <class Fresnel>
class RoughConductorBrdfSimd : public BsdfSimd
{
private:
  Fresnel fresnel;
  GGXDistribution<vfloat> microfacet;
  vfloat roughness;

  // Energy compensation [Kulla and Conty, 2017]
  vfloat Eavg;
  Vec3vf fmsScale;

public:
  prt_inline RoughConductorBrdfSimd(vbool m, const Basis3vf* frame, const Fresnel& fresnel, vfloat roughness, vfloat anisotropy)
    : BsdfSimd(frame, BsdfType::GlossyReflection),
      fresnel(fresnel),
      microfacet(roughnessToAlpha(roughness, anisotropy)),
      roughness(roughness)
  {
    // Energy compensation
    Eavg = MicrofacetAlbedoTable::evalAvg(m, roughness);
    Vec3vf Favg = fresnel.evalAvg();
    fmsScale = sqr(Favg) * Eavg / (vfloat(1.f) - Favg * (vfloat(1.f) - Eavg)); // Stephen Hill's tweak
  }

  Vec3vf eval(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo, const Vec3vf& wi, vfloat& pdf) const override
  {
    Vec3vf value = zero;
    pdf = zero;

    vfloat cosThetaO = dot(wo, getN());
    vfloat cosThetaI = dot(wi, getN());
    m &= (cosThetaO > 0.f) & (cosThetaI > 0.f);
    if (none(m))
      return value;

    // Compute the microfacet normal
    Vec3vf wh = normalize(wi + wo);
    vfloat cosThetaH = dot(wh, getN());
    vfloat cosThetaOH = dot(wo, wh);

    Vec3vf wo0 = getFrame().toLocal(wo);
    Vec3vf wi0 = getFrame().toLocal(wi);
    Vec3vf wh0 = getFrame().toLocal(wh);

    Vec3vf F = fresnel.eval(cosThetaOH);
    vfloat whPdf;
    vfloat D = microfacet.evalVisible(wh0, wo0, cosThetaOH, whPdf);
    vfloat G = microfacet.evalG2(wo0, wi0);

    // Energy compensation
    vfloat Eo = MicrofacetAlbedoTable::eval(m, cosThetaO, roughness);
    vfloat Ei = MicrofacetAlbedoTable::eval(m, cosThetaI, roughness);
    Vec3vf fms = fmsScale * ((1.f - Eo) * (1.f - Ei) * rcp(float(pi) * (1.f - Eavg)) * cosThetaI);

    set(m, pdf, whPdf * rcp(4.f*abs(cosThetaOH)));
    set(m, value, F * (D * G * rcp(4.f*cosThetaO)) + fms);
    return value;
  }

  Vec3vf sample(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo, Vec3vf& wi, vfloat& pdf, vint& type, const BsdfSampleSimd& s) const override
  {
    Vec3vf weight = zero;

    vfloat cosThetaO = dot(wo, getN());
    m &= cosThetaO > 0.f;
    if (none(m))
      return weight;

    Vec3vf wo0 = getFrame().toLocal(wo);

    // Sample the microfacet normal
    vfloat whPdf;
    Vec3vf wh = getFrame().toGlobal(microfacet.sampleVisible(wo0, whPdf, s.v));

    wi = reflect(wo, wh);
    vfloat cosThetaI = dot(wi, getN());
    m &= (cosThetaI > 0.f) & (dot(wi, ctx.Ng) > 0.f);
    if (none(m))
      return weight;
    vfloat cosThetaOH = dot(wo, wh);
    Vec3vf wi0 = getFrame().toLocal(wi);

    Vec3vf F = fresnel.eval(cosThetaOH);
    vfloat G = microfacet.evalG2(wo0, wi0);

    // Energy compensation
    vfloat Eo = MicrofacetAlbedoTable::eval(m, cosThetaO, roughness);
    vfloat Ei = MicrofacetAlbedoTable::eval(m, cosThetaI, roughness);
    Vec3vf fms = fmsScale * ((1.f - Eo) * (1.f - Ei) * rcp(float(pi) * (1.f - Eavg)) * cosThetaI);

    type = BsdfType::GlossyReflection;
    set(m, pdf, whPdf * rcp(4.f*abs(cosThetaOH)));
    set(m, weight, F * (G * rcpSafe(microfacet.evalG1(wo0))) + (fms * rcp(pdf)));
    return weight;
  }

  Vec3vf getAlbedo(vbool m) const override
  {
    return fresnel.getAlbedo();
  }

  Vec3vf getSpecularAlbedo(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo) const override
  {
    vfloat cosThetaO = dot(wo, getN());
    m &= cosThetaO > 0.f;
    if (none(m))
      return zero;

    Vec3vf r = fresnel.getR();
    Vec3vf g = fresnel.getG();
    vfloat RoR = MicrofacetConductorArtisticECAlbedoTable::eval(m, cosThetaO, r.x, g.x, roughness);
    vfloat RoG = MicrofacetConductorArtisticECAlbedoTable::eval(m, cosThetaO, r.y, g.y, roughness);
    vfloat RoB = MicrofacetConductorArtisticECAlbedoTable::eval(m, cosThetaO, r.z, g.z, roughness);
    return select(m, Vec3vf(RoR, RoG, RoB), Vec3vf(zero));
  }

  vfloat getRoughness(vbool m) const override
  {
    return roughness;
  }
};

} // namespace prt
