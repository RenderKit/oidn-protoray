// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/common.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/basis3.h"
#include "core/shading_context.h"

namespace prt {

// BSDF type flags
namespace BsdfType
{
  enum : int
  {
    DiffuseReflection    = 1 << 0,
    GlossyReflection     = 1 << 1,
    SpecularReflection   = 1 << 2,

    DiffuseTransmission  = 1 << 3,
    GlossyTransmission   = 1 << 4,
    SpecularTransmission = 1 << 5,
    UnbentTransmission   = 1 << 6, // continuation ray for specular BSDFs
    Transparency         = 1 << 7, // continuation ray for alpha transparency (e.g. cutout)

    Diffuse  = DiffuseReflection  | DiffuseTransmission,
    Glossy   = GlossyReflection   | GlossyTransmission,
    Specular = SpecularReflection | SpecularTransmission,

    Scattering   = Diffuse | Glossy | Specular,
    Continuation = UnbentTransmission | Transparency,

    Smooth = Diffuse | Glossy,
    Delta  = Specular | Continuation,

    Reflection   = DiffuseReflection | GlossyReflection | SpecularReflection,
    Transmission = DiffuseTransmission | GlossyTransmission | SpecularTransmission | Continuation,

    All = Reflection | Transmission
  };
};

struct BsdfSample
{
  Vec2f v; // direction/position
  float c; // component

  prt_inline BsdfSample() {}
  prt_inline BsdfSample(const Vec2f& v, float c) : v(v), c(c) {}
  prt_inline explicit BsdfSample(const Vec3f& s) : v(s.xy()), c(s.z) {}
};

struct BsdfSampleSimd
{
  Vec2vf v; // direction/position
  vfloat c; // component

  prt_inline BsdfSampleSimd() {}
  prt_inline BsdfSampleSimd(const Vec2vf& v, vfloat c) : v(v), c(c) {}
  prt_inline explicit BsdfSampleSimd(const Vec3vf& s) : v(s.xy()), c(s.z) {}
};

class Bsdf
{
protected:
  const Basis3f* frame;
  int flags;

  prt_inline const Basis3f& getFrame() const { return *frame; }
  prt_inline const Vec3f& getN() const { return frame->N; }

public:
  prt_inline Bsdf(const Basis3f* frame = nullptr, int flags = 0) : frame(frame), flags(flags) {}

  prt_inline int getFlags() const { return flags; }

  // Result is multiplied by dot(wi, N)
  virtual Vec3f eval(const ShadingContext& ctx, const Vec3f& wo, const Vec3f& wi, float& pdf) const { pdf = zero; return zero; }

  // Result is multiplied by dot(wi, N)/pdf
  // If the result is zero, the outputs are undefined
  virtual Vec3f sample(const ShadingContext& ctx, const Vec3f& wo, Vec3f& wi, float& pdf, int& type, const BsdfSample& s) const { return zero; }

  virtual Vec3f getTransparency(const ShadingContext& ctx, const Vec3f& wo) const { return zero; }
  virtual float getImportance() const { return 1.f; }
};

class BsdfSimd
{
protected:
  const Basis3vf* frame;
  vint flags;

  prt_inline const Basis3vf& getFrame() const { return *frame; }
  prt_inline const Vec3vf& getN() const { return frame->N; }

public:
  prt_inline BsdfSimd(const Basis3vf* frame = nullptr, vint flags = 0) : frame(frame), flags(flags) {}

  prt_inline vint getFlags() const { return flags; }

  // Result is multiplied by dot(wi, N)
  virtual Vec3vf eval(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo, const Vec3vf& wi, vfloat& pdf) const { pdf = zero; return zero; }

  // Result is multiplied by dot(wi, N)/pdf
  // If the result is zero, the outputs are undefined
  virtual Vec3vf sample(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo, Vec3vf& wi, vfloat& pdf, vint& type, const BsdfSampleSimd& s) const { return zero; }

  virtual Vec3vf getTransparency(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo) const { return zero; }
  virtual vfloat getImportance(vbool m) const { return vfloat(1.f); }

  virtual Vec3vf getAlbedo(vbool m) const = 0;

  virtual Vec3vf getDiffuseAlbedo(vbool m) const { return zero; }

  virtual Vec3vf getSpecularAlbedo(vbool m, const ShadingContextSimd& ctx, const Vec3vf& wo) const
  {
    return zero;
  }

  virtual Vec3vf getNormal(vbool m) const
  {
    if (frame)
      return getN();
    return zero;
  }

  virtual vfloat getRoughness(vbool m) const = 0;
};

} // namespace prt
