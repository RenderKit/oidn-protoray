// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "sys/blob.h"
#include "sys/filesystem.h"
#include "accel/embree_intersector_single.h"
#include "intersector_factory_single.h"

namespace prt {

std::shared_ptr<IntersectorSingle> IntersectorFactorySingle::make(const std::shared_ptr<const Scene>& scene, const Props& props, Props& stats)
{
  const std::string defaultIsectType = "single";
  std::string isectType = props.get("isect", defaultIsectType);

  Log() << "Intersector: " << isectType;

  if (isectType != "single")
    throw std::invalid_argument("invalid intersector type");

  bool filter = scene->hasTransparentMaterials();
  return std::make_shared<EmbreeIntersectorSingle>(scene->geometry, filter, props, stats);
}

} // namespace prt
