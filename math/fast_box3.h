// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "box3.h"

namespace prt {

struct FastBox3f
{
  vfloat4 low;
  vfloat4 high;

  prt_inline FastBox3f() {}
  prt_inline FastBox3f(Empty) : low(posMax), high(negMax) {}
  prt_inline FastBox3f(const FastBox3f& box) : low(box.low), high(box.high) {}
  prt_inline FastBox3f(const vfloat4& low, const vfloat4& high) : low(low), high(high) {}
  prt_inline explicit FastBox3f(const Box3f& box) : low(packVec3f(box.low)), high(packVec3f(box.high)) {}

  prt_inline FastBox3f& operator =(const FastBox3f& box)
  {
    low = box.low;
    high = box.high;
    return *this;
  }

  prt_inline vfloat4& operator [](size_t i)
  {
    assert(i < 2);
    return (&low)[i];
  }

  prt_inline const vfloat4& operator [](size_t i) const
  {
    assert(i < 2);
    return (&low)[i];
  }

  prt_inline vfloat4 getSize() const
  {
    return high - low;
  }

  prt_inline vfloat4 getCenter() const
  {
    return (low + high) * 0.5f;
  }

  prt_inline float getArea() const
  {
    return 2.0f * getHalfArea();
  }

  prt_inline float getHalfArea() const
  {
    vfloat4 s = getSize();
    vfloat4 a = s * permute4<1,2,0,3>(s);
    return toScalar(a + broadcast<1>(a) + broadcast<2>(a));
  }

  prt_inline float getVolume() const
  {
    vfloat4 s = getSize();
    vfloat4 v = s * permute4<1,2,0,3>(s) * permute4<2,0,1,3>(s);
    return toScalar(v);
  }

  prt_inline Box3f unpack() const
  {
    return Box3f(unpackVec3f(low), unpackVec3f(high));
  }

  prt_inline void grow(const vfloat4& vec)
  {
    low = min(low, vec);
    high = max(high, vec);
  }

  prt_inline void grow(const FastBox3f& box)
  {
    low = min(low, box.low);
    high = max(high, box.high);
  }

  prt_inline void intersect(const FastBox3f& box)
  {
    low = max(low, box.low);
    high = min(high, box.high);
  }
};

} // namespace prt
