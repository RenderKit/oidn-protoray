// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "light.h"

namespace prt {

// Sun and sky environment lights
// [Hosek and Wilkie 2012, "An Analytic Model for Full Spectral Sky-Dome Radiance"]
// [Hosek and Wilkie 2013, "Adding a Solar Radiance Function to the Hosek Skylight Model"]
class SunSkyLight
{
public:
  SunSkyLight(const Props& props, float sceneRadius);

  std::shared_ptr<EnvironmentLight> getSunLight() const { return sunLight; }
  std::shared_ptr<EnvironmentLight> getSkyLight() const { return skyLight; }

private:
  std::shared_ptr<EnvironmentLight> sunLight;
  std::shared_ptr<EnvironmentLight> skyLight;
};

} // namespace prt
