// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/props.h"
#include "core/intersector_single.h"
#include "scene.h"

namespace prt {

class IntersectorFactorySingle
{
public:
  static std::shared_ptr<IntersectorSingle> make(const std::shared_ptr<const Scene>& scene, const Props& props, Props& stats);
};

} // namespace prt
