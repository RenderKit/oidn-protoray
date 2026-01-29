// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common.h"

namespace prt {

class Stream
{
public:
  virtual ~Stream() {}

  virtual size_t read(void* dest, size_t count) = 0;
  virtual void write(const void* src, size_t count) = 0;

  void readFull(void* dest, size_t count)
  {
    if (read(dest, count) != count)
      throw std::runtime_error("unexpected end of stream");
  }

  template <class T>
  prt_inline Stream& operator <<(const T& obj)
  {
    static_assert(std::is_trivially_destructible<T>::value, "serialization not implemented");
    write(&obj, sizeof(obj));
    return *this;
  }

  template <class T>
  prt_inline Stream& operator >>(T& obj)
  {
    static_assert(std::is_trivially_destructible<T>::value, "serialization not implemented");
    readFull(&obj, sizeof(obj));
    return *this;
  }
};

} // namespace prt
