// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/common.h"
#include "math/vec2.h"
#include "pixel.h"

namespace prt {

// Low-level image I/O
class ImageIo
{
public:
  struct Desc
  {
    Vec2i size;
    PixelFormat format;
  };

  struct HandleSt;
  typedef HandleSt* Handle;

  static Handle open(const std::string& filename);
  static Desc getDesc(Handle h);
  static void getData(Handle h, void* data);
  static void close(Handle h);

  static bool save(const std::string& filename, const Desc& desc, const void* data);
};

} // namespace prt
