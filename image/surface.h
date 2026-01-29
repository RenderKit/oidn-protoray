// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/common.h"

namespace prt {

// RGBA8 surface
struct Surface
{
  void* data;
  int width;
  int height;
  int pitch; // in bytes

  prt_inline int* getRow(int y)
  {
    return (int*)((char*)data + y * (ptrdiff_t)pitch);
  }
};

} // namespace prt
