// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "bsdf/diffuse.h"
#include "bsdf/rough_diffuse.h"
#include "bsdf/coating.h"
#include "bsdf/rough_coating.h"
#include "bsdf/transparent.h"
#include "bsdf/multi_bsdf.h"
#include "math/noise.h"
#include "marble.h"

namespace prt {

namespace {

class MarbleColor : public Texture
{
private:
  Vec3f color0;
  Vec3f color1;
  Vec3f color2;
  float contrast;
  int octaves;
  float scale;

public:
  MarbleColor(const Props& props)
  {
    //color0 = props.get("color0", Vec3f(0.005f, 0.02f, 0.07f)*0.5f);
    color0 = props.get("color0", Vec3f(0.005f, 0.02f, 0.11f)*0.5f);
    //color1 = props.get("color1", Vec3f(0.9f, 0.85f, 0.8f));
    color1 = props.get("color1", Vec3f(0.9f, 0.82f, 0.71f));
    color2 = props.get("color2", Vec3f(0.85f, 0.85f, 0.85f));
    contrast = props.get("contrast", 0.8f);
    octaves = props.get("octaves", 10);
    scale = props.get("scale", 10.0f);
  }

  Vec3f get3f(const ShadingContext& ctx) const
  {
    Vec3f p = ctx.p * scale;
    Vec3f q = fbm3(p, octaves);
    float f = fbm(p + 4.0f*q, octaves);
    float f2 = fbm(p + 1.5f*f, octaves);

    Vec3f c = lerp(color1, color2, pow(f2, contrast));
    c = lerp(color0, c, pow(f, contrast));
    return c;
  }

  Vec3vf get3f(vbool m, const ShadingContextSimd& ctx) const
  {
    Vec3vf p = ctx.p * vfloat(scale);
    Vec3vf q = fbm3(p, octaves);
    vfloat f = fbm(p + vfloat(4.0f)*q, octaves);
    vfloat f2 = fbm(p + vfloat(1.5f)*f, octaves);

    Vec3vf c = lerp(Vec3vf(color1), Vec3vf(color2), pow(f2, contrast));
    c = lerp(Vec3vf(color0), c, pow(f, contrast));
    return c;
  }
};

} // namespace

MarbleMaterial::MarbleMaterial(const Props& props, MaterialFactory& factory)
  : Material(props, factory)
{
  extEta = props.get("ext.eta", 1.0f);
  coatEta = props.get("coat.eta", 1.6f);

  //color = getParam3f("color", Vec3f(0.9f));
  color = MaterialParam3f(std::make_shared<MarbleColor>(props));
  roughness = getParam1f("roughness", 0.1f);
  normal = getParamNormal("normal", "bump");
  baseNormal = getParamNormal("base.normal", "base.bump");

  // Coating
  coatColor = getParam3f("coat.color", 1.0f);
  coatThickness = props.get("coat.thickness", 1.0f);
  coatRoughness = getParam1f("coat.roughness", 0.2f);
  coatNormal = getParamNormal("coat.normal", "coat.bump");

  // Opacity
  opacity = getParam1f("opacity", 0.0f);
}

Bsdf* MarbleMaterial::getBsdf(ShadingContext& ctx, const Vec3f& wo) const
{
  const Basis3f* frame = ctx.makeFrame(normal.get(ctx), wo);
  const Basis3f* baseFrame = baseNormal ? ctx.makeFrame(baseNormal.get(ctx), wo) : frame;
  Bsdf* bsdf;

  if (roughness)
    bsdf = ctx.make<RoughDiffuseBrdf>(baseFrame, color.get(ctx), roughness.get(ctx));
  else
    bsdf = ctx.make<DiffuseBrdf>(baseFrame, color.get(ctx));

  if (coatEta != extEta)
  {
    const Basis3f* coatFrame = coatNormal ? ctx.makeFrame(coatNormal.get(ctx), wo) : frame;
    if (coatRoughness)
      bsdf = ctx.make<RoughCoatingBsdf>(coatFrame, bsdf, extEta/coatEta, coatColor.get(ctx), coatThickness, coatRoughness.get(ctx), 0.f, 1.f);
    else
      bsdf = ctx.make<CoatingBsdf>(coatFrame, bsdf, extEta/coatEta, coatColor.get(ctx), coatThickness, 1.f);
  }

  if (opacity)
  {
    float alpha = opacity.get(ctx);
    MultiBsdf* blend = ctx.make<MultiBsdf>();
    blend->add(bsdf, alpha);
    blend->add(ctx.make<TransparentBtdf>(), 1.f-alpha);
    bsdf = blend;
  }

  return bsdf;
}

BsdfSimd* MarbleMaterial::getBsdf(vbool m, ShadingContextSimd& ctx, const Vec3vf& wo) const
{
  const Basis3vf* frame = ctx.makeFrame(m, normal.get(m, ctx), wo);
  const Basis3vf* baseFrame = baseNormal ? ctx.makeFrame(m, baseNormal.get(m, ctx), wo) : frame;
  BsdfSimd* bsdf;

  if (roughness)
    bsdf = ctx.make<RoughDiffuseBrdfSimd>(m, baseFrame, color.get(m, ctx), roughness.get(m, ctx));
  else
    bsdf = ctx.make<DiffuseBrdfSimd>(m, baseFrame, color.get(m, ctx));

  if (coatEta != extEta)
  {
    const Basis3vf* coatFrame = coatNormal ? ctx.makeFrame(m, coatNormal.get(m, ctx), wo) : frame;
    if (coatRoughness)
      bsdf = ctx.make<RoughCoatingBsdfSimd>(m, coatFrame, bsdf, extEta/coatEta, coatColor.get(m, ctx), coatThickness, coatRoughness.get(m, ctx), 0.f, 1.f);
    else
      bsdf = ctx.make<CoatingBsdfSimd>(m, coatFrame, bsdf, extEta/coatEta, coatColor.get(m, ctx), coatThickness, 1.f);
  }

  if (opacity)
  {
    vfloat alpha = opacity.get(m, ctx);
    MultiBsdfSimd* blend = ctx.make<MultiBsdfSimd>();
    blend->add(m, bsdf, alpha);
    blend->add(m, ctx.make<TransparentBtdfSimd>(), 1.f-alpha);
    bsdf = blend;
  }

  return bsdf;
}

} // namespace prt

