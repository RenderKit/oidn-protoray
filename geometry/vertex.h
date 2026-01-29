// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "math/vec2.h"
#include "math/vec3.h"

namespace prt {

struct FatVertex
{
  Vec3f pos;
  Vec3f normal;
  Vec2f tex;
};

prt_inline bool operator ==(const FatVertex& a, const FatVertex& b)
{
  return (a.pos == b.pos) && (a.normal == b.normal) && (a.tex == b.tex);
}

prt_inline bool operator !=(const FatVertex& a, const FatVertex& b)
{
  return (a.pos != b.pos) || (a.normal != b.normal) || (a.tex != b.tex);
}

inline uint32_t hash(const Vec3f& v)
{
  // FNV-1a hash
  const uint32_t prime = 16777619;
  uint32_t res = 2166136261;

  // Position
  res = (res ^ bitwise_cast<uint32_t>(v[0])) * prime;
  res = (res ^ bitwise_cast<uint32_t>(v[1])) * prime;
  res = (res ^ bitwise_cast<uint32_t>(v[2])) * prime;

  return res;
}

inline uint32_t hash(const FatVertex& v)
{
  // FNV-1a hash
  const uint32_t prime = 16777619;
  uint32_t res = 2166136261;

  // Position
  res = (res ^ bitwise_cast<uint32_t>(v.pos[0])) * prime;
  res = (res ^ bitwise_cast<uint32_t>(v.pos[1])) * prime;
  res = (res ^ bitwise_cast<uint32_t>(v.pos[2])) * prime;

  // Normal
  res = (res ^ bitwise_cast<uint32_t>(v.normal[0])) * prime;
  res = (res ^ bitwise_cast<uint32_t>(v.normal[1])) * prime;
  res = (res ^ bitwise_cast<uint32_t>(v.normal[2])) * prime;

  // Texcoord
  res = (res ^ bitwise_cast<uint32_t>(v.tex[0])) * prime;
  res = (res ^ bitwise_cast<uint32_t>(v.tex[1])) * prime;

  return res;
}

} // namespace prt
