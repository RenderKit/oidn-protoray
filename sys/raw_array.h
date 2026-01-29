// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "memory.h"

namespace prt {

// Raw array with uninitialized items
template <class T>
class RawArray : Uncopyable
{
  static_assert(std::is_trivially_destructible<T>::value, "data type must be POD");

private:
  T* items;

public:
  prt_inline RawArray() : items(0) {}

  RawArray(int n)
  {
    assert(n >= 0);
    items = (T*)alignedAlloc(n * sizeof(T));
  }

  prt_inline ~RawArray()
  {
    alignedFree(items);
  }

  prt_inline T& operator [](size_t i)
  {
    return items[i];
  }

  prt_inline const T& operator [](size_t i) const
  {
    return items[i];
  }

  // Reallocates the array deleting its previous contents
  void alloc(int n)
  {
    assert(n >= 0);
    alignedFree(items);
    items = (T*)alignedAlloc(n * sizeof(T));
  }

  void free()
  {
    alignedFree(items);
    items = 0;
  }

  prt_inline T* getData()
  {
    return items;
  }

  prt_inline const T* getData() const
  {
    return items;
  }
};

} // namespace prt
