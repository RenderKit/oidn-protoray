// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <algorithm>
#include <iostream>
#include "common.h"

namespace prt {

// Statically allocated array for POD types only!
template <class T, int capacity>
class StaticArray
{
private:
  T items[capacity];
  int size;

public:
  prt_inline StaticArray() : size(0) {}

  prt_inline StaticArray(int n)
  {
    assert(n >= 0 && n <= capacity);
    size = n;
  }

  prt_inline T& operator [](size_t i)
  {
    assert(i < (size_t)size);
    return items[i];
  }

  prt_inline const T& operator [](size_t i) const
  {
    assert(i < (size_t)size);
    return items[i];
  }

  prt_inline T* getData()
  {
    return items;
  }

  prt_inline const T* getData() const
  {
    return items;
  }

  prt_inline int getSize() const
  {
    return size;
  }

  prt_inline bool isEmpty() const
  {
    return size == 0;
  }

  void resize(int n)
  {
    assert(n >= 0 && n <= capacity);
    size = n;
  }

  void clear()
  {
    size = 0;
  }

  prt_inline int pushBack()
  {
    assert(size < capacity);
    return size++;
  }

  prt_inline int pushBack(const T& item)
  {
    assert(size < capacity);
    items[size] = item;
    return size++;
  }

  template <class... Items>
  int pushBack(const T& item, const Items&... moreItems)
  {
    pushBack(item);
    return pushBack(moreItems...);
  }

  // Return -1 if the item is not found
  int indexOf(const T& item)
  {
    for (int i = 0; i < size; ++i)
    {
      if (items[i] == item)
        return i;
    }

    return -1;
  }

  // Slow!
  void insert(int pos, const T& value)
  {
    assert(size < capacity);
    assert(pos >= 0 && pos <= size);

    memmove(items + pos + 1, items + pos, (size - pos) * sizeof(T));
    items[pos] = value;
    ++size;
  }

  // Slow!
  void remove(int pos)
  {
    assert(pos >= 0 && pos < size);

    memmove(items + pos, items + pos + 1, (size - pos - 1) * sizeof(T));
    --size;
  }

  void sort()
  {
    std::sort(items, items + size);
  }

  template <class Predicate>
  void sort(Predicate predicate)
  {
    std::sort(items, items + size, predicate);
  }

  prt_inline T& getFront()
  {
    assert(size > 0);
    return items[0];
  }

  prt_inline const T& getFront() const
  {
    assert(size > 0);
    return items[0];
  }

  prt_inline T& getBack()
  {
    assert(size > 0);
    return items[size - 1];
  }

  prt_inline const T& getBack() const
  {
    assert(size > 0);
    return items[size - 1];
  }

  prt_inline T* begin()
  {
    return items;
  }

  prt_inline const T* begin() const
  {
    return items;
  }

  prt_inline T* end()
  {
    return items + size;
  }

  prt_inline const T* end() const
  {
    return items + size;
  }
};

template <class T, int capacity>
inline std::ostream& operator <<(std::ostream& osm, const StaticArray<T, capacity>& array)
{
  osm << "{" << std::endl;
  for (int i = 0; i < array.getSize(); ++i)
  {
    osm << "    " << array[i] << std::endl;
  }
  osm << "}";
  return osm;
}

} // namespace prt
