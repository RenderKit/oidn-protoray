// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "vec3.h"

namespace prt {

template <class T>
struct Box3
{
  Vec3<T> low;
  Vec3<T> high;

  prt_inline Box3() {}
  prt_inline Box3(Empty) : low(posMax), high(negMax) {}
  prt_inline Box3(const Box3<T>& box) : low(box.low), high(box.high) {}
  prt_inline Box3(const Vec3<T>& low, const Vec3<T>& high) : low(low), high(high) {}

  prt_inline Box3<T>& operator =(const Box3<T>& box)
  {
    low = box.low;
    high = box.high;
    return *this;
  }

  prt_inline Vec3<T>& operator [](size_t i)
  {
    assert(i < 2);
    return (&low)[i];
  }

  prt_inline const Vec3<T>& operator [](size_t i) const
  {
    assert(i < 2);
    return (&low)[i];
  }

  prt_inline Vec3<T> getSize() const
  {
    return high - low;
  }

  prt_inline Vec3<T> getCenter() const
  {
    return (low + high) * 0.5f;
  }

  prt_inline T getArea() const
  {
    return 2 * getHalfArea();
  }

  prt_inline T getHalfArea() const
  {
    Vec3<T> s = getSize();
    return s.x * (s.y + s.z) + s.y * s.z;
  }

  prt_inline T getVolume() const
  {
    Vec3<T> s = getSize();
    return s.x * s.y * s.z;
  }

  prt_inline void grow(const Vec3<T>& vec)
  {
    low = min(low, vec);
    high = max(high, vec);
  }

  prt_inline void grow(const Box3<T>& box)
  {
    low = min(low, box.low);
    high = max(high, box.high);
  }

  prt_inline void intersect(const Box3<T>& box)
  {
    low = max(low, box.low);
    high = min(high, box.high);
  }

  prt_inline bool contains(const Vec3<T>& vec) const
  {
    for (int i = 0; i < 3; ++i)
    {
      if (vec[i] < low[i] || vec[i] > high[i])
        return false;
    }

    return true;
  }

  prt_inline bool contains(const Box3<T>& box) const
  {
    for (int i = 0; i < 3; ++i)
    {
      if (box.low[i] < low[i] || box.high[i] > high[i])
        return false;
    }

    return true;
  }

  prt_inline bool overlaps(const Box3<T>& box) const
  {
    for (int i = 0; i < 3; ++i)
    {
      if (high[i] < box.low[i] || low[i] > box.high[i])
        return false;
    }

    return true;
  }

  prt_inline bool isValid() const
  {
    for (int i = 0; i < 3; ++i)
    {
      if (low[i] > high[i])
        return false;
    }

    return true;
  }
};

typedef Box3<float> Box3f;
typedef Box3<double> Box3d;
typedef Box3<int> Box3i;

} // namespace prt
