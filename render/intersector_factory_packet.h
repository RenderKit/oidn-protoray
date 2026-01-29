// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/props.h"
#include "core/intersector_packet.h"
#include "scene.h"

namespace prt {

class IntersectorFactoryPacket
{
public:
  static std::shared_ptr<IntersectorPacket> make(const std::shared_ptr<const Scene>& scene, const Props& props, Props& stats);
};

} // namespace prt
