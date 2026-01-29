// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "vec2.h"

namespace prt {

template <class T>
struct Box2
{
  Vec2<T> low;
  Vec2<T> high;

  prt_inline Box2() {}
  prt_inline Box2(Empty) : low(posMax), high(negMax) {}
  prt_inline Box2(const Box2<T>& box) : low(box.low), high(box.high) {}
  prt_inline Box2(const Vec2<T>& low, const Vec2<T>& high) : low(low), high(high) {}

  prt_inline Box2<T>& operator =(const Box2<T>& box)
  {
    low = box.low;
    high = box.high;
    return *this;
  }

  prt_inline Vec2<T>& operator [](size_t i)
  {
    assert(i < 2);
    return (&low)[i];
  }

  prt_inline const Vec2<T>& operator [](size_t i) const
  {
    assert(i < 2);
    return (&low)[i];
  }

  prt_inline Vec2<T> getSize() const
  {
    return high - low;
  }

  prt_inline Vec2<T> getCenter() const
  {
    return (low + high) * 0.5f;
  }

  prt_inline T getArea() const
  {
    return 2 * getHalfArea();
  }

  prt_inline T getHalfArea() const
  {
    Vec2<T> s = getSize();
    return s.x + s.y;
  }

  prt_inline T getVolume() const
  {
    Vec2<T> s = getSize();
    return s.x * s.y;
  }

  prt_inline void grow(const Vec2<T>& vec)
  {
    low = min(low, vec);
    high = max(high, vec);
  }

  prt_inline void grow(const Box2<T>& box)
  {
    low = min(low, box.low);
    high = max(high, box.high);
  }

  prt_inline void intersect(const Box2<T>& box)
  {
    low = max(low, box.low);
    high = min(high, box.high);
  }

  prt_inline bool contains(const Box2<T>& box) const
  {
    for (int i = 0; i < 2; ++i)
    {
      if (box.low[i] < low[i] || box.high[i] > high[i])
        return false;
    }

    return true;
  }

  prt_inline bool isValid() const
  {
    for (int i = 0; i < 2; ++i)
    {
      if (low[i] > high[i])
        return false;
    }

    return true;
  }
};

typedef Box2<float> Box2f;
typedef Box2<double> Box2d;
typedef Box2<int> Box2i;

} // namespace prt
