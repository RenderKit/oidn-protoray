// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/constants.h"

namespace prt {

template <class T>
struct Box1
{
  T low;
  T high;

  prt_inline Box1() {}
  prt_inline Box1(Empty) : low(posMax), high(negMax) {}
  prt_inline Box1(const Box1<T>& box) : low(box.low), high(box.high) {}
  prt_inline Box1(const T& low, const T& high) : low(low), high(high) {}

  prt_inline Box1<T>& operator =(const Box1<T>& box)
  {
    low = box.low;
    high = box.high;
    return *this;
  }

  prt_inline T& operator [](size_t i)
  {
    assert(i < 2);
    return (&low)[i];
  }

  prt_inline const T& operator [](size_t i) const
  {
    assert(i < 2);
    return (&low)[i];
  }

  prt_inline T getSize() const
  {
    return high - low;
  }

  prt_inline T getCenter() const
  {
    return (low + high) * 0.5f;
  }

  prt_inline T getArea() const
  {
    return high - low;
  }

  prt_inline T getVolume() const
  {
    return high - low;
  }

  prt_inline void grow(const T& x)
  {
    low = min(low, x);
    high = max(high, x);
  }

  prt_inline void grow(const Box1<T>& box)
  {
    grow(box.low);
    grow(box.high);
  }

  prt_inline void intersect(const Box1<T>& box)
  {
    low = max(low, box.low);
    high = min(high, box.high);
  }

  prt_inline bool contains(const Box1& box) const
  {
    return box.low >= low && box.high <= high;
  }

  prt_inline bool isValid() const
  {
    return low <= high;
  }
};

typedef Box1<float> Box1f;
typedef Box1<int> Box1i;

} // namespace prt
