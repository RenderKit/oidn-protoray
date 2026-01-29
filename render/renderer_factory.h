// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "renderer.h"

namespace prt {

class RendererFactory
{
public:
  static std::string getSamplerType(const std::string& type, const Props& props);
};

} // namespace prt
