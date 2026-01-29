// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "math/vec2.h"
#include "math/vec4.h"
#include "math/simd.h"
#include "pixel.h"
#include "accum_buffer.h"
#include "surface.h"
#include "pixel_filter.h"
#include "tone_mapper.h"

namespace prt {

class FrameBuffer
{
private:
  AccumBuffer3f color;           // color
  AccumBuffer3f colorUnclamp;    // unclamped color
  AccumBuffer3f albedo;          // albedo
  AccumBuffer3f albedo1;         // first-hit albedo
  AccumBuffer3f diffuseAlbedo;   // diffuse albedo
  AccumBuffer3f diffuseAlbedo1;  // first-hit diffuse albedo
  AccumBuffer3f specularAlbedo;  // specular albedo
  AccumBuffer3f specularAlbedo1; // first-hit specular albedo
  AccumBuffer3f normal;          // world-space normal
  AccumBuffer3f normal1;         // first-hit world-space normal
  AccumBuffer1f depth;           // linear depth
  // AccumBuffer3f cameraNormal; // camera-space normal
  AccumBuffer1f hwDepth;         // inverted HW depth
  AccumBuffer1f roughness;       // linear roughness
  AccumBuffer1f roughness1;      // first-hit linear roughness
  AccumBuffer1f alpha;           // alpha opacity
  AccumBuffer3f sh[4];           // L1 SH coefficients
  AccumBuffer2f motionBack;      // backward motion vector
  AccumBuffer2f motionFore;      // forward motion vector
  std::optional<Vec2f> jitter;   // sub-pixel jitter (0-1)
  std::shared_ptr<PixelFilter> filter;
  PixelFilterMode filterMode;
  std::shared_ptr<ToneMapper> toneMapper;
  Vec2f invSize;

public:
  FrameBuffer(const Vec2i& size, int colorFlags = 0);

  void clear();
  void begin();
  void finish();

  prt_inline AccumBuffer3f& getColor()  { return color; }
  prt_inline AccumBuffer3f& getColorUnclamp() { return colorUnclamp; }
  prt_inline AccumBuffer3f& getAlbedo() { return albedo; }
  prt_inline AccumBuffer3f& getAlbedo1() { return albedo1; }
  prt_inline AccumBuffer3f& getDiffuseAlbedo() { return diffuseAlbedo; }
  prt_inline AccumBuffer3f& getDiffuseAlbedo1() { return diffuseAlbedo1; }
  prt_inline AccumBuffer3f& getSpecularAlbedo() { return specularAlbedo; }
  prt_inline AccumBuffer3f& getSpecularAlbedo1() { return specularAlbedo1; }
  prt_inline AccumBuffer3f& getNormal() { return normal; }
  prt_inline AccumBuffer3f& getNormal1() { return normal1; }
  prt_inline AccumBuffer1f& getDepth() { return depth; }
  // prt_inline AccumBuffer3f& getCameraNormal() { return cameraNormal; }
  prt_inline AccumBuffer1f& getHWDepth() { return hwDepth; }
  prt_inline AccumBuffer1f& getRoughness() { return roughness; }
  prt_inline AccumBuffer1f& getRoughness1() { return roughness1; }
  prt_inline AccumBuffer1f& getAlpha() { return alpha; }
  prt_inline AccumBuffer3f& getSh(size_t i) { assert(i < 4); return sh[i]; }
  prt_inline AccumBuffer2f& getMotionBack() { return motionBack; }
  prt_inline AccumBuffer2f& getMotionFore() { return motionFore; }

  prt_inline Vec2i getSize() const { return color.getSize(); }
  prt_inline Vec2f getInvSize() const { return invSize; }

  prt_inline bool hasJitter() const { return jitter.has_value(); }
  prt_inline Vec2f getJitter() const { return *jitter; }
  prt_inline void setJitter(const Vec2f& j) { jitter = j; }
  prt_inline void disableJitter() { jitter.reset(); }

  prt_inline PixelFilter* getFilter() const { return filter.get(); }
  prt_inline void setFilter(const std::shared_ptr<PixelFilter>& f) { filter = f; }

  prt_inline PixelFilterMode getFilterMode() const { return filterMode; }
  prt_inline void setFilterMode(PixelFilterMode fm) { filterMode = fm; }

  prt_inline ToneMapper* getToneMapper() const { return toneMapper.get(); }
  prt_inline void setToneMapper(const std::shared_ptr<ToneMapper>& tm) { toneMapper = tm; }
};

} // namespace prt
