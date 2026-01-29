// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "sys/logging.h"
#include "renderer_factory.h"

namespace prt {

std::string RendererFactory::getSamplerType(const std::string& type, const Props& props)
{
  std::string samplerType = props.get("sampler", "default");

  if (samplerType == "default")
    samplerType = "sobolBurley";

  return samplerType;
}

} // namespace prt
