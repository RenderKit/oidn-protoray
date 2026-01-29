// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "sys/memory.h"
#include "frame_buffer.h"

namespace prt {

FrameBuffer::FrameBuffer(const Vec2i& size, int colorFlags)
  : color(size, colorFlags),
    filterMode(PixelFilterMode::Sample)
{
  invSize = rcp(toFloat(size));
}

void FrameBuffer::clear()
{
  color.clear();
  if (colorUnclamp) colorUnclamp.clear();
  if (albedo) albedo.clear();
  if (albedo1) albedo1.clear();
  if (diffuseAlbedo) diffuseAlbedo.clear();
  if (diffuseAlbedo1) diffuseAlbedo1.clear();
  if (specularAlbedo) specularAlbedo.clear();
  if (specularAlbedo1) specularAlbedo1.clear();
  if (normal) normal.clear();
  if (normal1) normal1.clear();
  if (depth) depth.clear();
  //if (cameraNormal) cameraNormal.clear();
  if (hwDepth) hwDepth.clear();
  if (roughness) roughness.clear();
  if (roughness1) roughness1.clear();
  if (alpha) alpha.clear();
  for (int i = 0; i < 4; ++i)
    if (sh[i]) sh[i].clear();
  if (motionBack) motionBack.clear();
  if (motionFore) motionFore.clear();
}

void FrameBuffer::begin()
{
  color.begin();
  if (colorUnclamp) colorUnclamp.begin();
  if (albedo) albedo.begin();
  if (albedo1) albedo1.begin();
  if (diffuseAlbedo) diffuseAlbedo.begin();
  if (diffuseAlbedo1) diffuseAlbedo1.begin();
  if (specularAlbedo) specularAlbedo.begin();
  if (specularAlbedo1) specularAlbedo1.begin();
  if (normal) normal.begin();
  if (normal1) normal1.begin();
  if (depth) depth.begin();
  //if (cameraNormal) cameraNormal.begin();
  if (hwDepth) hwDepth.begin();
  if (roughness) roughness.begin();
  if (roughness1) roughness1.begin();
  if (alpha) alpha.begin();
  for (int i = 0; i < 4; ++i)
    if (sh[i]) sh[i].begin();
  if (motionBack) motionBack.begin();
  if (motionFore) motionFore.begin();
}

void FrameBuffer::finish()
{
  color.finish();
  if (colorUnclamp) colorUnclamp.finish();
  if (albedo) albedo.finish();
  if (albedo1) albedo1.finish();
  if (diffuseAlbedo) diffuseAlbedo.finish();
  if (diffuseAlbedo1) diffuseAlbedo1.finish();
  if (specularAlbedo) specularAlbedo.finish();
  if (specularAlbedo1) specularAlbedo1.finish();
  if (normal) normal.finish();
  if (normal1) normal1.finish();
  if (depth) depth.finish();
  // if (cameraNormal) cameraNormal.finish();
  if (hwDepth) hwDepth.finish();
  if (roughness) roughness.finish();
  if (roughness1) roughness1.finish();
  if (alpha) alpha.finish();
  for (int i = 0; i < 4; ++i)
    if (sh[i]) sh[i].finish();
  if (motionBack) motionBack.finish();
  if (motionFore) motionFore.finish();
}

} // namespace prt
