// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/props.h"
#include "sys/blob.h"
#include "sys/filesystem.h"
#include "core/intersector_stream.h"
#include "accel/embree_intersector_stream.h"

#include "scene.h"

namespace prt {

class IntersectorFactoryStream
{
public:
  template <int streamSize>
  static std::shared_ptr<IntersectorStream<streamSize>> make(const std::shared_ptr<const Scene>& scene, const Props& props, Props& stats)
  {
    const std::string defaultIsectType = "packet";
    std::string isectType = props.get("isect", defaultIsectType);

    Log() << "Intersector: " << isectType;

    bool filter = scene->hasTransparentMaterials();

    if (isectType == "single")
      return std::make_shared<EmbreeIntersectorSingleStream<streamSize>>(scene->geometry, filter, props, stats);
    if (isectType == "packet")
      return std::make_shared<EmbreeIntersectorPacketStream<streamSize>>(scene->geometry, filter, props, stats);

    throw std::invalid_argument("invalid intersector type");
  }
};

} // namespace prt
