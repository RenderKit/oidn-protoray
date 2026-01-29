// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "texture.h"
#include "image.h"
#include "pixel.h"

namespace prt {

// The texture is stored in a tiled 4x4 format, upside down

prt_inline void storeTexel(int* data, int pitchInTiles, int x, int y, const int& c)
{
  int offset = ((y>>2) * pitchInTiles + (x>>2)) * 16 + (y&3) * 4 + (x&3);
  data[offset] = c;
}

prt_inline void storeTexel(uint8_t* data, int pitchInTiles, int x, int y, const int& c)
{
  int offset = ((y>>2) * pitchInTiles + (x>>2)) * 16 + (y&3) * 4 + (x&3);
  data[offset] = (uint8_t)c;
}

prt_inline void storeTexel(Vec3f* data, int pitchInTiles, int x, int y, const Vec3f& c)
{
  int offset = ((y>>2) * pitchInTiles + (x>>2)) * 48 + (y&3) * 4 + (x&3);
  float* ptr = (float*)data + offset;
  *ptr = c.x;
  *(ptr+16) = c.y;
  *(ptr+32) = c.z;
}

prt_inline int fetchTexel(const int* data, int pitchInTiles, int x, int y)
{
  int offset = ((y>>2) * pitchInTiles + (x>>2)) * 16 + (y&3) * 4 + (x&3);

  return data[offset];
}

prt_inline int fetchTexel(const uint8_t* data, int pitchInTiles, int x, int y)
{
  int offset = ((y>>2) * pitchInTiles + (x>>2)) * 16 + (y&3) * 4 + (x&3);

  return data[offset];
}

prt_inline vint fetchTexel(vbool m, const int* data, int pitchInTiles, vint x, vint y)
{
  vint offset = ((y>>2) * pitchInTiles + (x>>2)) * 16 + (y&3) * 4 + (x&3);

  return gather(m, data, offset);
}

prt_inline vint fetchTexel(vbool m, const uint8_t* data, int pitchInTiles, vint x, vint y)
{
  vint offset = ((y>>2) * pitchInTiles + (x>>2)) * 16 + (y&3) * 4 + (x&3);

  vint result = zero;
  for (int i = 0; i < simdSize; ++i)
    if (m[i]) result[i] = data[offset[i]];
  return result;
}

prt_inline Vec3f fetchTexel(const Vec3f* data, int pitchInTiles, int x, int y)
{
  int offset = ((y>>2) * pitchInTiles + (x>>2)) * 48 + (y&3) * 4 + (x&3);
  const float* ptr = (const float*)data + offset;

  return Vec3f(*ptr, *(ptr+16), *(ptr+32));
}

prt_inline Vec3vf fetchTexel(vbool m, const Vec3f* data, int pitchInTiles, vint x, vint y)
{
  const float* ptr = (const float*)data;
  vint offset = ((y>>2) * pitchInTiles + (x>>2)) * 48 + (y&3) * 4 + (x&3);

  return Vec3vf(gather(m, ptr,    offset),
                gather(m, ptr+16, offset),
                gather(m, ptr+32, offset));
}

class ImageTexture : public Texture
{
public:
  using Texture::get3f;

  // Unfiltered lookup
  virtual Vec3f get3f(const Vec2i& uv) const = 0;

  // Filtered lookup
  virtual Vec3f get3f(const Vec2f& uv) const = 0;
  virtual Vec3vf get3f(vbool m, const Vec2vf& uv) const = 0;

  virtual Vec2i getSize() const = 0;
};

enum class Swizzle
{
  Default,
  R,
  G,
  B,
  A
};

struct ImageTextureParams
{
  ColorSpace space {ColorSpace::Srgb};
  Swizzle swizzle {Swizzle::Default};
  TextureAddress addressU {TextureAddress::Repeat};
  TextureAddress addressV {TextureAddress::Repeat};
  int uvId {0};
  Vec2f uvOffset {zero};
  Vec2f uvScale  {one};

  explicit ImageTextureParams(ColorSpace space = ColorSpace::Srgb,
                              Swizzle swizzle = Swizzle::Default,
                              TextureAddress addressU = TextureAddress::Repeat,
                              TextureAddress addressV = TextureAddress::Repeat,
                              int uvId = 0,
                              Vec2f uvOffset = zero,
                              Vec2f uvScale = one)
    : space(space),
      swizzle(swizzle),
      addressU(addressU),
      addressV(addressV),
      uvId(uvId),
      uvOffset(uvOffset),
      uvScale(uvScale) {}

  explicit ImageTextureParams(TextureAddress addressU = TextureAddress::Repeat,
                              TextureAddress addressV = TextureAddress::Repeat,
                              int uvId = 0,
                              Vec2f uvOffset = zero,
                              Vec2f uvScale = one)
    : space(ColorSpace::Srgb),
      swizzle(Swizzle::Default),
      addressU(addressU),
      addressV(addressV),
      uvId(uvId),
      uvOffset(uvOffset),
      uvScale(uvScale) {}
};

template <class T, PixelFormat format, ColorSpace space, TextureAddress addressU, TextureAddress addressV>
class ImageTextureImpl : public ImageTexture
{
private:
  const T* data;      // pointer to the tiled texture data
  Vec2i size;         // image size
  Vec2f halfTexel;    // half-texel size
  int pitchInTiles;   // pitch in number of tiles
  Swizzle swizzle;    // swizzle mode
  int uvId;           // UV set index
  Vec2f uvOffset;     // UV offset
  Vec2f uvScale;      // UV scale
  std::shared_ptr<Memory> buffer; // reference to the texture buffer

public:
  ImageTextureImpl(const std::shared_ptr<Memory>& buffer, const Vec2i& size,
                   const ImageTextureParams& params)
  {
    assert(params.space == space);
    assert(params.addressU == addressU);
    assert(params.addressV == addressV);

    this->size = size;
    this->halfTexel = 0.5f / toFloat(size);
    this->buffer = buffer;
    this->data = (const T*)buffer->getData();
    this->pitchInTiles = (size.x + 3) / 4;
    this->swizzle = params.swizzle;
    if (this->swizzle == Swizzle::A && format != PixelFormat::Bgra8)
      this->swizzle = Swizzle::Default;
    this->uvId = params.uvId < maxNumUVs ? params.uvId : 0;
    this->uvOffset = params.uvOffset;
    this->uvScale  = params.uvScale;
  }

  // Nearest
  prt_inline Vec4f get(const Vec2i& uv) const
  {
    int x = (addressU == TextureAddress::Repeat) ? ((uint32_t)uv.x % size.x) : clamp(uv.x, 0, size.x-1);
    int y = (addressV == TextureAddress::Repeat) ? ((uint32_t)uv.y % size.y) : clamp(uv.y, 0, size.y-1);

    return DecodePixel<format, space>::get(fetchTexel(data, pitchInTiles, x, y));
  }

  prt_inline Vec3f get3f(const Vec2i& uv) const override
  {
    return get(uv).xyz();
  }

  // Linear
  prt_inline Vec4f get(const Vec2f& uv) const
  {
    float x = uv.x - halfTexel.x;
    float y = uv.y - halfTexel.y;

    // Repeat or clamp
    float xr = (addressU == TextureAddress::Repeat) ? ((x - floor(x)) * toFloat(size.x)) : clamp(x * toFloat(size.x), 0.0f, toFloat(size.x-1));
    float yr = (addressV == TextureAddress::Repeat) ? ((y - floor(y)) * toFloat(size.y)) : clamp(y * toFloat(size.y), 0.0f, toFloat(size.y-1));

    // Compute interpolation weights
    float sx = xr - floor(xr);
    float sy = yr - floor(yr);

    // Fetch texels
    int x0 = clamp(toInt(xr), 0, size.x-1);
    int y0 = clamp(toInt(yr), 0, size.y-1);
    int x1 = (addressU == TextureAddress::Repeat) ? (x0+1 == size.x ? 0 : x0+1) : min(x0+1, size.x-1);
    int y1 = (addressV == TextureAddress::Repeat) ? (y0+1 == size.y ? 0 : y0+1) : min(y0+1, size.y-1);

    auto c00 = DecodePixel<format, space>::get(fetchTexel(data, pitchInTiles, x0, y0));
    auto c11 = DecodePixel<format, space>::get(fetchTexel(data, pitchInTiles, x1, y1));
    auto c01 = DecodePixel<format, space>::get(fetchTexel(data, pitchInTiles, x0, y1));
    auto c10 = DecodePixel<format, space>::get(fetchTexel(data, pitchInTiles, x1, y0));

    // Interpolate texels
    auto c0 = lerp(c00, c10, sx);
    auto c1 = lerp(c01, c11, sx);

    return lerp(c0, c1, sy);
  }

  prt_inline Vec3f get3f(const Vec2f& uv) const override
  {
    return get(uv).xyz();
  }

  prt_inline Vec4vf get(vbool m, const Vec2vf& uv) const
  {
    Vec2vi vsize = size;

    vfloat x = uv.x - halfTexel.x;
    vfloat y = uv.y - halfTexel.y;

    // Repeat or clamp
    vfloat xr = (addressU == TextureAddress::Repeat) ? ((x - floor(x)) * toFloat(vsize.x)) : clamp(x * toFloat(vsize.x), vfloat(zero), toFloat(vsize.x-1));
    vfloat yr = (addressV == TextureAddress::Repeat) ? ((y - floor(y)) * toFloat(vsize.y)) : clamp(y * toFloat(vsize.y), vfloat(zero), toFloat(vsize.y-1));

    // Compute interpolation weights
    vfloat sx = xr - floor(xr);
    vfloat sy = yr - floor(yr);

    // Fetch texels
    vint x0 = clamp(toInt(xr), vint(zero), vsize.x-1);
    vint y0 = clamp(toInt(yr), vint(zero), vsize.y-1);
    vint x1 = (addressU == TextureAddress::Repeat) ? select(x0+1 == vsize.x, vint(zero), x0+1) : min(x0+1, vsize.x-1);
    vint y1 = (addressV == TextureAddress::Repeat) ? select(y0+1 == vsize.y, vint(zero), y0+1) : min(y0+1, vsize.y-1);

    auto c00 = DecodePixel<format, space>::get(fetchTexel(m, data, pitchInTiles, x0, y0));
    auto c11 = DecodePixel<format, space>::get(fetchTexel(m, data, pitchInTiles, x1, y1));
    auto c01 = DecodePixel<format, space>::get(fetchTexel(m, data, pitchInTiles, x0, y1));
    auto c10 = DecodePixel<format, space>::get(fetchTexel(m, data, pitchInTiles, x1, y0));

    // Interpolate texels
    auto c0 = lerp(c00, c10, sx);
    auto c1 = lerp(c01, c11, sx);

    return lerp(c0, c1, sy);
  }

  prt_inline Vec3vf get3f(vbool m, const Vec2vf& uv) const override
  {
    return get(m, uv).xyz();
  }

  prt_inline Vec2f getUV(const ShadingContext& ctx) const
  {
    return ctx.uv[uvId] * uvScale + uvOffset;
  }

  prt_inline Vec2vf getUV(const ShadingContextSimd& ctx) const
  {
    return ctx.uv[uvId] * Vec2vf(uvScale) + Vec2vf(uvOffset);
  }

  prt_inline Vec3f get3f(const ShadingContext& ctx) const override
  {
    return get(getUV(ctx)).xyz();
  }

  prt_inline Vec4f get4f(const ShadingContext& ctx) const override
  {
    return get(getUV(ctx));
  }

  prt_inline float get1f(const ShadingContext& ctx) const override
  {
    switch (swizzle)
    {
    case Swizzle::R:
      return get(getUV(ctx)).x;
    case Swizzle::G:
      return get(getUV(ctx)).y;
    case Swizzle::B:
      return get(getUV(ctx)).z;
    case Swizzle::A:
      return get(getUV(ctx)).w;
    default:
      return average(get(getUV(ctx)).xyz());
    }
  }

  prt_inline Vec3vf get3f(vbool m, const ShadingContextSimd& ctx) const override
  {
    return get(m, getUV(ctx)).xyz();
  }

  prt_inline Vec4vf get4f(vbool m, const ShadingContextSimd& ctx) const override
  {
    return get(m, getUV(ctx));
  }

  prt_inline vfloat get1f(vbool m, const ShadingContextSimd& ctx) const override
  {
    switch (swizzle)
    {
    case Swizzle::R:
      return get(m, getUV(ctx)).x;
    case Swizzle::G:
      return get(m, getUV(ctx)).y;
    case Swizzle::B:
      return get(m, getUV(ctx)).z;
    case Swizzle::A:
      return get(m, getUV(ctx)).w;
    default:
      return average(get(m, getUV(ctx)).xyz());
    }
  }

  prt_inline bool hasAlpha() const override
  {
    return (format == PixelFormat::Bgra8);
  }

  prt_inline Vec2i getSize() const override { return size; }
};

class ImageTextureBuffer
{
private:
  std::shared_ptr<Memory> buffer;
  Vec2i size;
  PixelFormat format;

public:
  template <class T>
  ImageTextureBuffer(const std::shared_ptr<Image<T>>& image, PixelFormat format)
  {
    this->format = format;
    store(image);
  }

  prt_inline Vec2i getSize() const { return size; }

  std::shared_ptr<ImageTexture> getTexture(const ImageTextureParams& params) const
  {
    switch (format)
    {
    case PixelFormat::Bgrx8:
      return getTextureT<int, PixelFormat::Bgrx8>(params);

    case PixelFormat::Bgra8:
      return getTextureT<int, PixelFormat::Bgra8>(params);

    case PixelFormat::R8:
      return getTextureT<uint8_t, PixelFormat::R8>(params);

    case PixelFormat::Rgb32f:
      return getTextureT<Vec3f, PixelFormat::Rgb32f>(params);

    default:
      throw std::invalid_argument("invalid pixel format");
    }
  }

private:
  template <class T>
  void store(const std::shared_ptr<Image<T>>& image)
  {
    int bufferWidth = (image->getWidth() + 3) / 4 * 4;
    int bufferHeight = (image->getHeight() + 3) / 4 * 4;

    int pitchInTiles = bufferWidth / 4;

    buffer = std::make_shared<Memory>(bufferWidth * bufferHeight * sizeof(T));
    T* data = (T*)buffer->getData();

    for (int y = 0; y < image->getHeight(); ++y)
    {
      for (int x = 0; x < image->getWidth(); ++x)
        storeTexel(data, pitchInTiles, x, y, (*image)[image->getHeight()-1-y][x]);
    }

    size = image->getSize();
  }

  template <class T, PixelFormat format>
  std::shared_ptr<ImageTexture> getTextureT(const ImageTextureParams& params) const
  {
    switch (params.space)
    {
    case ColorSpace::Srgb:
      return getTextureTColorSpace<T, format, ColorSpace::Srgb>(params);

    case ColorSpace::Linear:
      return getTextureTColorSpace<T, format, ColorSpace::Linear>(params);

    case ColorSpace::SignedLinear:
      return getTextureTColorSpace<T, format, ColorSpace::SignedLinear>(params);

    default:
      throw std::invalid_argument("invalid pixel space");
    }
  }

  template <class T, PixelFormat format, ColorSpace space>
  std::shared_ptr<ImageTexture> getTextureTColorSpace(const ImageTextureParams& params) const
  {
    if (params.addressU == TextureAddress::Repeat && params.addressV == TextureAddress::Repeat)
      return std::make_shared<ImageTextureImpl<T, format, space, TextureAddress::Repeat, TextureAddress::Repeat>>(buffer, size, params);

    if (params.addressU == TextureAddress::Repeat && params.addressV == TextureAddress::Clamp)
      return std::make_shared<ImageTextureImpl<T, format, space, TextureAddress::Repeat, TextureAddress::Clamp>>(buffer, size, params);

    if (params.addressU == TextureAddress::Clamp && params.addressV == TextureAddress::Repeat)
      return std::make_shared<ImageTextureImpl<T, format, space, TextureAddress::Clamp, TextureAddress::Repeat>>(buffer, size, params);

    if (params.addressU == TextureAddress::Clamp && params.addressV == TextureAddress::Clamp)
      return std::make_shared<ImageTextureImpl<T, format, space, TextureAddress::Clamp, TextureAddress::Clamp>>(buffer, size, params);

    throw std::invalid_argument("invalid texture addressing mode");
  }
};

std::shared_ptr<ImageTextureBuffer> loadImageTexture(const std::string& filename);

} // namespace prt
