// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/memory.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"

namespace prt {

enum class PixelFormat
{
  Bgrx8,
  Bgra8,
  Rg8,
  R8,
  Rgb16f,
  Rg16f,
  R16f,
  Rgb32f,
  Rg32f,
  R32f,
};

enum class ColorSpace
{
  Srgb,
  Linear,
  SignedLinear
};

extern const float decodeSrgb8Table[256]; // 2.2 gamma

prt_inline int encodeBgr8(const Vec3f& c)
{
  Vec3i ci = clamp(toInt(pow(c, 1.f/2.2f) * 255.f), 0, 255);
  return ci.z | (ci.y << 8) | (ci.x << 16);
}

prt_inline vint encodeBgr8(const Vec3vf& c)
{
  Vec3vi ci = clamp(toInt(pow(c, vfloat(1.f/2.2f)) * vfloat(255.f)), vint(0), vint(255));
  return ci.z | (ci.y << 8) | (ci.x << 16);
}

// For linearly encoded data (e.g., bump maps)
prt_inline int encodeBgr8Linear(const Vec3f& c)
{
  Vec3i ci = clamp(toInt(c * 255.f), 0, 255);
  return ci.z | (ci.y << 8) | (ci.x << 16);
}

prt_inline Vec3f decodeBgr8(const Vec4uc& c)
{
  return Vec3f(decodeSrgb8Table[c.z], decodeSrgb8Table[c.y], decodeSrgb8Table[c.x]);
}

prt_inline Vec3f decodeBgr8(int c)
{
  uint32_t b = (uint32_t)c & 0xff;
  uint32_t g = ((uint32_t)c >> 8) & 0xff;
  uint32_t r = ((uint32_t)c >> 16) & 0xff;

  return Vec3f(decodeSrgb8Table[r], decodeSrgb8Table[g], decodeSrgb8Table[b]);
}

prt_inline Vec3vf decodeBgr8(vint c)
{
  vint b = c & 0xff;
  vint g = shr(c, 8) & 0xff;
  vint r = shr(c, 16) & 0xff;

  return Vec3vf(gather(decodeSrgb8Table, r),
                gather(decodeSrgb8Table, g),
                gather(decodeSrgb8Table, b));
}

prt_inline Vec4f decodeBgra8(int c)
{
  uint32_t b = (uint32_t)c & 0xff;
  uint32_t g = ((uint32_t)c >> 8) & 0xff;
  uint32_t r = ((uint32_t)c >> 16) & 0xff;
  uint32_t a = (uint32_t)c >> 24;

  return Vec4f(decodeSrgb8Table[r],
               decodeSrgb8Table[g],
               decodeSrgb8Table[b],
               (float)a * (1.f/255.f));
}

prt_inline Vec4vf decodeBgra8(vint c)
{
  vint b = c & 0xff;
  vint g = shr(c, 8) & 0xff;
  vint r = shr(c, 16) & 0xff;
  vint a = shr(c, 24);

  return Vec4vf(gather(decodeSrgb8Table, r),
                gather(decodeSrgb8Table, g),
                gather(decodeSrgb8Table, b),
                toFloat(a) * vfloat(1.f/255.f));
}

prt_inline float decodeR8(int c)
{
  uint32_t r = (uint32_t)c & 0xff;
  return decodeSrgb8Table[r];
}

prt_inline vfloat decodeR8(vint c)
{
  vint r = c & 0xff;
  return gather(decodeSrgb8Table, r);
}

// For linearly encoded data (e.g., bump maps)
prt_inline Vec3f decodeBgr8Linear(int c)
{
  uint32_t b = (uint32_t)c & 0xff;
  uint32_t g = ((uint32_t)c >> 8) & 0xff;
  uint32_t r = ((uint32_t)c >> 16) & 0xff;

  return Vec3f((float)r, (float)g, (float)b) * (1.f/255.f);
}

prt_inline Vec3vf decodeBgr8Linear(vint c)
{
  vint b = c & 0xff;
  vint g = shr(c, 8) & 0xff;
  vint r = shr(c, 16) & 0xff;

  return toFloat(Vec3vi(r, g, b)) * vfloat(1.f/255.f);
}

prt_inline Vec4f decodeBgra8Linear(int c)
{
  uint32_t b = (uint32_t)c & 0xff;
  uint32_t g = ((uint32_t)c >> 8) & 0xff;
  uint32_t r = ((uint32_t)c >> 16) & 0xff;
  uint32_t a = (uint32_t)c >> 24;

  return Vec4f((float)r, (float)g, (float)b, (float)a) * (1.f/255.f);
}

prt_inline Vec4vf decodeBgra8Linear(vint c)
{
  vint b = c & 0xff;
  vint g = shr(c, 8) & 0xff;
  vint r = shr(c, 16) & 0xff;
  vint a = shr(c, 24);

  return toFloat(Vec4vi(r, g, b, a)) * vfloat(1.f/255.f);
}

prt_inline float decodeR8Linear(int c)
{
  uint32_t r = (uint32_t)c & 0xff;
  return float(r) * (1.f/255.f);
}

prt_inline vfloat decodeR8Linear(vint c)
{
  vint r = c & 0xff;
  return toFloat(r) * vfloat(1.f/255.f);
}

// For normal maps
prt_inline Vec3f decodeBgr8SignedLinear(int c)
{
  uint32_t b = (uint32_t)c & 0xff;
  uint32_t g = ((uint32_t)c >> 8) & 0xff;
  uint32_t r = ((uint32_t)c >> 16) & 0xff;

  return Vec3f((float)r, (float)g, (float)b) * (2.f/255.f) - 1.f;
}

prt_inline Vec3vf decodeBgr8SignedLinear(vint c)
{
  vint b = c & 0xff;
  vint g = shr(c, 8) & 0xff;
  vint r = shr(c, 16) & 0xff;

  return toFloat(Vec3vi(r, g, b)) * vfloat(2.f/255.f) - vfloat(1.f);
}

prt_inline Vec4f decodeBgra8SignedLinear(int c)
{
  uint32_t b = (uint32_t)c & 0xff;
  uint32_t g = ((uint32_t)c >> 8) & 0xff;
  uint32_t r = ((uint32_t)c >> 16) & 0xff;
  uint32_t a = (uint32_t)c >> 24;

  return Vec4f(Vec3f((float)r, (float)g, (float)b) * (2.f/255.f) - 1.f,
                     (float)a * (1.f/255.f));
}

prt_inline Vec4vf decodeBgra8SignedLinear(vint c)
{
  vint b = c & 0xff;
  vint g = shr(c, 8) & 0xff;
  vint r = shr(c, 16) & 0xff;
  vint a = shr(c, 24);

  return Vec4vf(toFloat(Vec3vi(r, g, b)) * vfloat(2.f/255.f) - vfloat(1.f),
                toFloat(a) * vfloat(1.f/255.f));
}

prt_inline float decodeR8SignedLinear(int c)
{
  uint32_t r = (uint32_t)c & 0xff;
  return float(r) * (2.f/255.f) - 1.f;
}

prt_inline vfloat decodeR8SignedLinear(vint c)
{
  vint r = c & 0xff;
  return toFloat(r) * vfloat(2.f/255.f) - vfloat(1.f);
}

// TODO: gamma
prt_inline int encodeBgr8Fast(const Vec3f& c)
{
  Vec3i ci = clamp(toInt(c * 255.f), 0, 255);
  return ci.z | (ci.y << 8) | (ci.x << 16);
}

// TODO: gamma
prt_inline vint encodeBgr8Fast(const Vec3vf& c)
{
  Vec3vi ci = clamp(toInt(c * vfloat(255.f)), vint(0), vint(255));
  return ci.z | (ci.y << 8) | (ci.x << 16);
}

// DecodePixel class
// -----------------

template <PixelFormat format, ColorSpace space = ColorSpace::Srgb>
struct DecodePixel;


template <>
struct DecodePixel<PixelFormat::Bgrx8, ColorSpace::Srgb>
{
  prt_inline static Vec3f  get(int  c) { return decodeBgr8(c); }
  prt_inline static Vec3vf get(vint c) { return decodeBgr8(c); }
};

template <>
struct DecodePixel<PixelFormat::Bgrx8, ColorSpace::Linear>
{
  prt_inline static Vec3f  get(int  c) { return decodeBgr8Linear(c); }
  prt_inline static Vec3vf get(vint c) { return decodeBgr8Linear(c); }
};

template <>
struct DecodePixel<PixelFormat::Bgrx8, ColorSpace::SignedLinear>
{
  prt_inline static Vec3f  get(int  c) { return decodeBgr8SignedLinear(c); }
  prt_inline static Vec3vf get(vint c) { return decodeBgr8SignedLinear(c); }
};

template <>
struct DecodePixel<PixelFormat::Bgra8, ColorSpace::Srgb>
{
  prt_inline static Vec4f  get(int  c) { return decodeBgra8(c); }
  prt_inline static Vec4vf get(vint c) { return decodeBgra8(c); }
};

template <>
struct DecodePixel<PixelFormat::Bgra8, ColorSpace::Linear>
{
  prt_inline static Vec4f  get(int  c) { return decodeBgra8Linear(c); }
  prt_inline static Vec4vf get(vint c) { return decodeBgra8Linear(c); }
};

template <>
struct DecodePixel<PixelFormat::Bgra8, ColorSpace::SignedLinear>
{
  prt_inline static Vec4f  get(int  c) { return decodeBgra8SignedLinear(c); }
  prt_inline static Vec4vf get(vint c) { return decodeBgra8SignedLinear(c); }
};

template <>
struct DecodePixel<PixelFormat::R8, ColorSpace::Srgb>
{
  prt_inline static float  get(int  c) { return decodeR8(c); }
  prt_inline static vfloat get(vint c) { return decodeR8(c); }
};

template <>
struct DecodePixel<PixelFormat::R8, ColorSpace::Linear>
{
  prt_inline static float  get(int  c) { return decodeR8Linear(c); }
  prt_inline static vfloat get(vint c) { return decodeR8Linear(c); }
};

template <>
struct DecodePixel<PixelFormat::R8, ColorSpace::SignedLinear>
{
  prt_inline static float  get(int  c) { return decodeR8SignedLinear(c); }
  prt_inline static vfloat get(vint c) { return decodeR8SignedLinear(c); }
};

// Color space is ignored for float formats
template <ColorSpace space>
struct DecodePixel<PixelFormat::Rgb32f, space>
{
  prt_inline static Vec3f  get(const Vec3f&  c) { return c; }
  prt_inline static Vec3vf get(const Vec3vf& c) { return c; }
};

} // namespace prt
