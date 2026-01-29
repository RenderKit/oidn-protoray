// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "renderer_factory.h"

namespace prt {

class RendererFactoryStream : public RendererFactory
{
public:
  static std::shared_ptr<Renderer> make(const std::string& type, const std::shared_ptr<const Scene>& scene, const std::shared_ptr<Intersector>& intersector, const Props& props, Props& stats);

private:
  template <class Sampler>
  static std::shared_ptr<Renderer> make(const std::string& type, const std::shared_ptr<const Scene>& scene, const std::shared_ptr<Intersector>& intersector, const Props& props, Props& stats);

  template <class Sampler, int streamSize, int streamTileX>
  static std::shared_ptr<Renderer> make(const std::string& type, const std::shared_ptr<const Scene>& scene, const std::shared_ptr<Intersector>& intersector, const Props& props, Props& stats);
};

} // namespace prt
