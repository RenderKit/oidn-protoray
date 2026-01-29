// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <type_traits>
#include <algorithm>
#include <iostream>
#include "memory.h"
#include "stream.h"

namespace prt {

// Aligned dynamic array
template <class T, class Idx = int>
class Array
{
private:
  T* items;
  Idx size;
  Idx capacity;

public:
  prt_inline Array() : items(0), size(0), capacity(0) {}

  Array(Idx n)
  {
    assert(n >= 0);

    items = (T*)alignedAlloc(n * sizeof(T));

    for (Idx i = 0; i < n; ++i)
      new (items + i) T;

    size = n;
    capacity = n;
  }

  Array(const Array& other)
  {
    copyFrom(other);
  }

  prt_inline ~Array()
  {
    cleanup();
  }

  Array& operator =(const Array& other)
  {
    if (&other == this)
      return *this;

    cleanup();
    copyFrom(other);
    return *this;
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

  // Reallocates the array deleting its previous contents
  void alloc(Idx n)
  {
    assert(n >= 0);

    cleanup();

    items = (T*)alignedAlloc(n * sizeof(T));

    for (Idx i = 0; i < n; ++i)
      new (items + i) T;

    size = n;
    capacity = n;
  }

  void free()
  {
    cleanup();

    items = 0;
    size = 0;
    capacity = 0;
  }

  prt_inline Idx getSize() const
  {
    return size;
  }

  prt_inline bool isEmpty() const
  {
    return size == 0;
  }

  // Resizes the array (does not compact!)
  void resize(Idx n)
  {
    if (size == n)
      return;

    if (n < size)
    {
      // Trim
      for (Idx i = n; i < size; ++i)
        items[i].~T();

      size = n;
    }
    else
    {
      // Extend
      reserve(n);

      for (Idx i = size; i < n; ++i)
        new (items + i) T;

      size = n;
    }
  }

  // Reallocates the array
  void reserve(Idx n)
  {
    assert(n >= size);

    if (n == capacity)
      return;

    capacity = n;

    // Reallocate and move data
    T* oldItems = items;
    items = (T*)alignedAlloc(capacity * sizeof(T));

    if (std::is_trivially_copy_constructible<T>::value)
    {
      memcpy(items, oldItems, size * sizeof(T));
    }
    else
    {
      // Slow!
      for (Idx i = 0; i < size; ++i)
      {
        new (items + i) T(std::move(oldItems[i]));
        oldItems[i].~T();
      }
    }

    alignedFree(oldItems);
  }

  void compact()
  {
    reserve(size);
  }

  void clear()
  {
    for (Idx i = 0; i < size; ++i)
      items[i].~T();

    size = 0;
  }

  prt_inline Idx pushBack()
  {
    if (size == capacity)
      expand();

    new (items + size) T;
    return size++;
  }

  prt_inline Idx pushBack(const T& item)
  {
    if (size == capacity)
      expand();

    new (items + size) T(item);
    return size++;
  }

  template <class... Items>
  Idx pushBack(const T& item, const Items&... moreItems)
  {
    pushBack(item);
    return pushBack(moreItems...);
  }

  // Return -1 if the item is not found
  Idx indexOf(const T& item)
  {
    for (Idx i = 0; i < size; ++i)
    {
      if (items[i] == item)
        return i;
    }

    return -1;
  }

  // Slow!
  void insert(Idx pos, const T& value)
  {
    assert(pos >= 0 && pos <= size);

    if (size == capacity)
      expand();

    memmove(items + pos + 1, items + pos, (size - pos) * sizeof(T));
    new (&items[pos]) T(value);
    ++size;
  }

  // Slow!
  void remove(Idx pos)
  {
    assert(pos >= 0 && pos < size);

    items[pos].~T();
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

  prt_inline T* getData()
  {
    return items;
  }

  prt_inline const T* getData() const
  {
    return items;
  }

private:
  void cleanup()
  {
    for (Idx i = 0; i < size; ++i)
      items[i].~T();

    alignedFree(items);
  }

  void copyFrom(const Array& other)
  {
    size = other.size;
    capacity = other.size;

    if (size > 0)
    {
      items = (T*)alignedAlloc(size * sizeof(T));

      if (std::is_trivially_copy_constructible<T>::value)
      {
        memcpy(items, other.items, size * sizeof(T));
      }
      else
      {
        // Slow!
        for (Idx i = 0; i < size; ++i)
          new (items + i) T(other.items[i]);
      }
    }
    else
    {
      items = 0;
    }
  }

  void expand()
  {
    const Idx minCapacity = 32;
    Idx newCapacity = max(capacity * 2, minCapacity);
    reserve(newCapacity);
  }
};

template <class T, class Idx>
inline Stream& operator <<(Stream& osm, const Array<T, Idx>& array)
{
  osm << array.getSize();

  if (std::is_trivially_copy_constructible<T>::value)
  {
    osm.write(array.getData(), array.getSize() * sizeof(T));
  }
  else
  {
    for (Idx i = 0; i < array.getSize(); ++i)
      osm << array[i];
  }

  return osm;
}

template <class T, class Idx>
inline Stream& operator >>(Stream& ism, Array<T, Idx>& array)
{
  Idx size;
  ism >> size;
  if (size < 0)
    throw std::runtime_error("invalid array size in stream");
  array.alloc(size);

  if (std::is_trivially_copy_constructible<T>::value)
  {
    ism.readFull(array.getData(), size * sizeof(T));
  }
  else
  {
    for (Idx i = 0; i < size; ++i)
      ism >> array[i];
  }

  return ism;
}

template <class T, class Idx>
inline std::ostream& operator <<(std::ostream& osm, const Array<T, Idx>& array)
{
  osm << "{" << std::endl;
  for (Idx i = 0; i < array.getSize(); ++i)
  {
    osm << "    " << array[i] << std::endl;
  }
  osm << "}";
  return osm;
}

} // namespace prt
