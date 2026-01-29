// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/logging.h"
#include "sys/tasking.h"
#include "math/random.h"
#include "image/texture.h"
#include "image/image_texture.h"
#include "image/color.h"

namespace prt {

class MaterialParam3f
{
public:
  std::shared_ptr<Texture> texture;
  Vec3f scale;

#ifdef PRT_MUTATION_SUPPORT
private:
  float hueOffset = 0.f;
  float saturationScale = 1.f;
  float maxSaturation = negInf;
#endif

public:
  MaterialParam3f() : scale(zero) {}
  MaterialParam3f(const Vec3f& value) : scale(value) {}
  MaterialParam3f(const std::shared_ptr<Texture>& texture, const Vec3f& scale = one) : texture(texture), scale(scale) {}

  prt_inline bool isUniform() const { return !texture; }

  prt_inline Vec3f get() const { return finalize(scale); }

  prt_inline Vec3f get(const ShadingContext& ctx) const
  {
    if (texture) return finalize(texture->get3f(ctx) * scale);
    return finalize(scale);
  }

  prt_inline Vec3vf get(vbool m, const ShadingContextSimd& ctx) const
  {
    if (texture) return finalize(texture->get3f(m, ctx) * Vec3vf(scale));
    return finalize(scale);
  }

  prt_inline operator bool() const { return scale != Vec3f(zero); }
  prt_inline bool operator !() const { return scale == Vec3f(zero); }

  prt_inline bool operator ==(const Vec3f& value) const
  {
    return (isUniform() || scale == Vec3f(zero)) && (scale == value);
  }

  prt_inline bool operator !=(const Vec3f& value) const
  {
    return !(*this == value);
  }

  void mutate(Random& rng)
  {
#ifdef PRT_MUTATION_SUPPORT
    if (maxSaturation < 0.f)
    {
      maxSaturation = 0.f;

      if (texture)
      {
        std::shared_ptr<ImageTexture> imageTexture = std::dynamic_pointer_cast<ImageTexture>(texture);
        if (imageTexture)
        {
          Vec2i size = imageTexture->getSize();

          maxSaturation =
            tbb::parallel_reduce(
              tbb::blocked_range2d<int>(0, size.y, 0, size.x),
              0.f,
              [&](const tbb::blocked_range2d<int>& r, float partialMax) -> float
              {
                for (int y = r.rows().begin(); y != r.rows().end(); ++y)
                {
                  for (int x = r.cols().begin(); x != r.cols().end(); ++x)
                  {
                    Vec3f rgb = clamp(imageTexture->get3f(Vec2i(x,y)) * scale, 0.f, 1.f);
                    Vec3f hsv = rgbToHsv(rgb);
                    partialMax = max(partialMax, hsv.y);
                  }
                }
                return partialMax;
              },
              [](float a, float b) -> float { return max(a, b); }
             );
        }
      }
      else
      {
        Vec3f rgb = clamp(scale, 0.f, 1.f);
        Vec3f hsv = rgbToHsv(rgb);
        maxSaturation = hsv.y;
      }
    }

    if (texture)
    {
      hueOffset = rng.get1f();
      float saturationTarget = max(rng.get1f(-0.01f, 1.2f), 0.f); // allow some slight over- and undersaturation
      saturationScale = (maxSaturation > 0.f) ? min(saturationTarget / maxSaturation, 10.f) : 1.f;
    }
    else if (maxSaturation > 0.f)
    {
      // If there's no texture, just generate a random RGB color
      scale = rng.get3f();

      // Rarely quantize the color to 0/1
      if (rng.get1f() < 0.2f)
      {
        scale.x = scale.x < 0.5f ? 0.f : 1.f;
        scale.y = scale.y < 0.5f ? 0.f : 1.f;
        scale.z = scale.z < 0.5f ? 0.f : 1.f;
      }
    }
#endif
  }

  template <class T>
  prt_inline Vec3<T> finalize(const Vec3<T>& in) const
  {
#ifdef PRT_MUTATION_SUPPORT
    Vec3<T> c = in;

    if (texture)
    {
      c = rgbToHsv(c);
      c.x += hueOffset;
      c.y = min(c.y * saturationScale, T(1.f));
      c = hsvToRgb(c);
      //c = c * (reduceMax(in) / reduceMax(c));
    }

    return clamp(c, T(0.f), T(1.f));
#else
    return in;
#endif
  }
};

class MaterialParam1f
{
public:
  std::shared_ptr<Texture> texture;
  float scale;

#ifdef PRT_MUTATION_SUPPORT
private:
  float rndScale = 1.f;
  float maxValue = negInf;
#endif

public:
  MaterialParam1f() : scale(zero) {}
  MaterialParam1f(float value) : scale(value) {}
  MaterialParam1f(const std::shared_ptr<Texture>& texture, float scale = one) : texture(texture), scale(scale) {}

  prt_inline bool isUniform() const { return !texture; }
  prt_inline float get() const { return scale; }

  prt_inline float get(const ShadingContext& ctx) const
  {
    if (texture) return finalize(texture->get1f(ctx) * scale);
    return finalize(scale);
  }

  prt_inline vfloat get(vbool m, const ShadingContextSimd& ctx) const
  {
    if (texture) return finalize(texture->get1f(m, ctx) * scale);
    return finalize(scale);
  }

  prt_inline operator bool() const { return scale != 0.f; }
  prt_inline bool operator !() const { return scale == 0.f; }

  prt_inline bool operator ==(float value) const
  {
    return (isUniform() || scale == 0.f) && (scale == value);
  }

  prt_inline bool operator !=(float value) const
  {
    return !(*this == value);
  }

  float mutate(Random& rng, float a, float b)
  {
#ifdef PRT_MUTATION_SUPPORT
    if (maxValue < 0.f)
    {
      maxValue = 0.f;

      if (texture)
      {
        std::shared_ptr<ImageTexture> imageTexture = std::dynamic_pointer_cast<ImageTexture>(texture);
        if (imageTexture)
        {
          Vec2i size = imageTexture->getSize();

          maxValue =
            tbb::parallel_reduce(
              tbb::blocked_range2d<int>(0, size.y, 0, size.x),
              0.f,
              [&](const tbb::blocked_range2d<int>& r, float partialMax) -> float
              {
                for (int y = r.rows().begin(); y != r.rows().end(); ++y)
                {
                  for (int x = r.cols().begin(); x != r.cols().end(); ++x)
                  {
                    float value = average(imageTexture->get3f(Vec2i(x,y))) * scale;
                    partialMax = max(partialMax, value);
                  }
                }
                return partialMax;
              },
              [](float a, float b) -> float { return max(a, b); }
             );
        }
      }
      else
      {
        maxValue = scale;
      }
    }

    float valueTarget = rng.get1f(a, max(b, maxValue));
    if (texture)
    {
      rndScale = (maxValue > 0.f) ? min(valueTarget / maxValue, 100.f) : 1.f;
    }
    else
    {
      rndScale = 1.f;
      scale = valueTarget;
    }
    return valueTarget;
#endif
    return 0.f;
  }

  template <class T>
  prt_inline T finalize(const T& in) const
  {
#ifdef PRT_MUTATION_SUPPORT
    return in * rndScale;
#else
    return in;
#endif
  }
};

// Tangent space normal
class MaterialParamNormal
{
public:
  std::shared_ptr<Texture> texture;
  Vec2f scale;

  MaterialParamNormal() : scale(zero) {}
  MaterialParamNormal(const std::shared_ptr<Texture>& texture, Vec2f scale = 1.f) : texture(texture), scale(scale) {}

  prt_inline operator bool() const { return bool(texture); }
  prt_inline bool operator !() const { return !texture; }

  prt_inline bool isUniform() const { return !texture; }
  prt_inline Vec3f get() const { return Vec3f(0.f, 0.f, 1.f); }

  prt_inline Vec3f get(const ShadingContext& ctx) const
  {
    if (texture)
    {
      Vec3f N = texture->get3f(ctx);
      return normalize(Vec3f(N.x*scale.x, N.y*scale.y, N.z));
    }
    return Vec3f(0.f, 0.f, 1.f);
  }

  prt_inline Vec3vf get(vbool m, const ShadingContextSimd& ctx) const
  {
    if (texture)
    {
      Vec3vf N = texture->get3f(m, ctx);
      return normalize(Vec3vf(N.x*scale.x, N.y*scale.y, N.z));
    }
    return Vec3vf(0.f, 0.f, 1.f);
  }

  void mutate(Random& rng)
  {
    if (texture)
      texture->mutate(rng);
  }
};

} // namespace prt
