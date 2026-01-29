// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "math/simd.h"
#include "math/vec3.h"
#include "math/random.h"
#include "image/color.h"

namespace prt {

struct Medium
{
  Vec3f color; // transmittance
  float ior;   // refractive index

  prt_inline Medium() {}
  prt_inline Medium(Zero) : color(zero), ior(zero) {}
  prt_inline Medium(One) : color(one), ior(one) {}
  prt_inline Medium(const Vec3f& color, float ior) : color(color), ior(ior) {}
  prt_inline Medium(const Medium& other) : color(other.color), ior(other.ior) {}

  prt_inline bool operator ==(const Medium& other) const
  {
    return color == other.color && ior == other.ior;
  }
};

class Media
{
private:
  static const int maxSize = 2048;

  float color[3][maxSize];
  float ior[maxSize];

  std::shared_ptr<Medium> media[maxSize];
  int size;

public:
  Media() : size(0) {}

  prt_inline const std::shared_ptr<Medium>& get(int id) const
  {
    assert(id >= 0 && id < size);
    return media[id];
  }

  prt_inline int getSize() const { return size; }

  int add(const std::shared_ptr<Medium>& medium)
  {
    if (size == maxSize)
      throw std::runtime_error("too many media");

    const int id = size;
    media[id] = medium;
    update(id);
    size++;
    return id;
  }

  void update(int id)
  {
    const Medium* medium = media[id].get();
    color[0][id] = medium->color[0];
    color[1][id] = medium->color[1];
    color[2][id] = medium->color[2];
    ior[id] = medium->ior;
  }

  void update()
  {
    for (int id = 0; id < size; ++id)
      update(id);
  }

  prt_inline Vec3f getColor(int id) const
  {
    return Vec3f(color[0][id], color[1][id], color[2][id]);
  }

  prt_inline Vec3vf getColor(vbool m, vint id) const
  {
    return gather3(m, color[0], color[1], color[2], id);
  }
};

} // namespace prt
