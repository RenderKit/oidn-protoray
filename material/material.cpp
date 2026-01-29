// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "material.h"
#include "bsdf/diffuse.h"

namespace prt {

Material::Material(const Props& props, MaterialFactory& factory, int type)
  : mediumIntId(0),
    mediumExtId(0),
    type(type),
    props(props),
    factory(factory)
{
}

MaterialParam3f Material::getParam3f(const std::string& name, const Vec3f& def, ColorSpace space)
{
  std::shared_ptr<Texture> texture;
  std::string mapName = name + "Map";
  if (props.exists(mapName) && factory.isTexturingEnabled())
    texture = factory.makeTexture(props.get(mapName), getImageTextureParams(name, space));

  Vec3f value = props.get(name, def);

  if (props.exists(name))
  {
    if (texture)
      return MaterialParam3f(texture, value);
  }
  else if (texture)
  {
    return MaterialParam3f(texture);
  }

  return MaterialParam3f(value);
}

MaterialParam1f Material::getParam1f(const std::string& name, float def, ColorSpace space)
{
  std::shared_ptr<Texture> texture;
  std::string mapName = name + "Map";
  if (props.exists(mapName) && factory.isTexturingEnabled())
    texture = factory.makeTexture(props.get(mapName), getImageTextureParams(name, space));

  float value = props.get(name, def);

  if (props.exists(name))
  {
    if (texture)
      return MaterialParam1f(texture, value);
  }
  else if (texture)
  {
    return MaterialParam1f(texture);
  }

  return MaterialParam1f(value);
}

MaterialParam1f Material::getParamAlpha(const std::string& name, float def, bool hasScale)
{
  std::shared_ptr<Texture> texture;
  std::string mapName = name + "Map";
  if (props.exists(mapName) && factory.isTexturingEnabled())
  {
    ImageTextureParams params = getImageTextureParams(name, ColorSpace::Linear);
    if (params.swizzle == Swizzle::Default)
      params.swizzle = Swizzle::A;
    texture = factory.makeTexture(props.get(mapName), params);
  }

  float value = hasScale ? props.get(name, def) : def;

  if (props.exists(name))
  {
    if (texture)
      return MaterialParam1f(texture, value);
  }
  else if (texture)
  {
    return MaterialParam1f(texture);
  }

  return MaterialParam1f(value);
}

MaterialParamNormal Material::getParamNormal(const std::string& normalName, const std::string& bumpName)
{
  // Try to load normal map
  std::string normalMapName = normalName + "Map";
  if (props.exists(normalMapName) && factory.isTexturingEnabled())
  {
    auto texture = factory.makeTexture(props.get(normalMapName),
                     getImageTextureParams(normalName, ColorSpace::SignedLinear));
    Vec2f scale = factory.isFlipNormalMapYEnabled() ? Vec2f(1.f, -1.f) : 1.f;
    scale *= props.get(normalName, 1.f);
    return MaterialParamNormal(texture, scale);
  }

  // Try to load bump map
  std::string bumpMapName = bumpName + "Map";
  if (props.exists(bumpMapName) && factory.isTexturingEnabled())
  {
    auto texture = factory.makeBumpNormalTexture(props.get(bumpMapName),
                     getImageTextureParams(bumpName, ColorSpace::SignedLinear));
    float scale = props.get(bumpName, 1.0f);
    return MaterialParamNormal(texture, scale);
  }

  return MaterialParamNormal();
}

ImageTextureParams Material::getImageTextureParams(const std::string& name, ColorSpace space)
{
  const std::string swizzleStr = props.get(name + "MapSwizzle", "");
  Swizzle swizzle;
  if (swizzleStr == "")
    swizzle = Swizzle::Default;
  else if (swizzleStr == "r" || swizzleStr == "R")
    swizzle = Swizzle::R;
  else if (swizzleStr == "g" || swizzleStr == "G")
    swizzle = Swizzle::G;
  else if (swizzleStr == "b" || swizzleStr == "B")
    swizzle = Swizzle::B;
  else if (swizzleStr == "a" || swizzleStr == "A")
    swizzle = Swizzle::A;
  else
    throw std::runtime_error("unsupported swizzle value for texture map: " + swizzleStr);

  const int uvId = props.get(name + "MapUV", 0);
  const Vec2f uvOffset = props.get(name + "MapUVOffset", Vec2f(zero));
  const Vec2f uvScale  = props.get(name + "MapUVScale",  Vec2f(one));
  return ImageTextureParams(space, swizzle, TextureAddress::Repeat, TextureAddress::Repeat,
                            uvId, uvOffset, uvScale);
}

Bsdf* Material::getBsdf(ShadingContext& ctx, const Vec3f& wo) const
{
  return ctx.make<DiffuseBrdf>(ctx.makeFrame(wo), zero);
}

BsdfSimd* Material::getBsdf(vbool m, ShadingContextSimd& ctx, const Vec3vf& wo) const
{
  return ctx.make<DiffuseBrdfSimd>(m, ctx.makeFrame(m, wo), zero);
}

} // namespace prt
