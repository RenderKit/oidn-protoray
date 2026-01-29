// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "sys/blob.h"
#include "sys/filesystem.h"
#include "accel/embree_intersector_packet.h"
#include "intersector_factory_packet.h"

namespace prt {

std::shared_ptr<IntersectorPacket> IntersectorFactoryPacket::make(const std::shared_ptr<const Scene>& scene, const Props& props, Props& stats)
{
  const std::string defaultIsectType = "packet";
  std::string isectType = props.get("isect", defaultIsectType);

  Log() << "Intersector: " << isectType;

  bool filter = scene->hasTransparentMaterials();

  if (isectType == "single")
    return std::make_shared<EmbreeIntersectorSinglePacket>(scene->geometry, filter, props, stats);
  if (isectType == "packet")
    return std::make_shared<EmbreeIntersectorPacket>(scene->geometry, filter, props, stats);
  if (isectType == "packet8")
    return std::make_shared<EmbreeIntersectorPacket8>(scene->geometry, filter, props, stats);

  throw std::invalid_argument("invalid intersector type");
}

} // namespace prt
