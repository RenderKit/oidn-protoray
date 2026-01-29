// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "sampling/random_sampler_simd.h"
#include "sampling/sobol_sampler_simd.h"
#include "sampling/sobol_burley_sampler_simd.h"
#include "intersector_factory_stream.h"
#include "primary_renderer_stream.h"
#include "diffuse_renderer_stream.h"
#include "diffuse2_renderer_stream.h"
#include "ao_renderer_stream.h"
#include "path_renderer_stream.h"
#include "renderer_factory_stream.h"

namespace prt {

std::shared_ptr<Renderer> RendererFactoryStream::make(const std::string& type, const std::shared_ptr<const Scene>& scene, const std::shared_ptr<Intersector>& intersector, const Props& props, Props& stats)
{
  std::string samplerType = getSamplerType(type, props);
  Log() << "Sampler: " << samplerType;

  if (samplerType == "sobol")
    return make<SobolSamplerSimd>(type, scene, intersector, props, stats);
  if (samplerType == "sobolBurley")
    return make<SobolBurleySamplerSimd<false>>(type, scene, intersector, props, stats);
  if (samplerType == "sobolBurleyBlueNoise")
    return make<SobolBurleySamplerSimd<true>>(type, scene, intersector, props, stats);
#if 1
  if (samplerType == "random")
    return make<RandomSamplerSimd>(type, scene, intersector, props, stats);
#endif

  throw std::invalid_argument("invalid sampler type");
}

template <class Sampler>
std::shared_ptr<Renderer> RendererFactoryStream::make(const std::string& type, const std::shared_ptr<const Scene>& scene, const std::shared_ptr<Intersector>& intersector, const Props& props, Props& stats)
{
  return make<Sampler, 1024, 32>(type, scene, intersector, props, stats);
}

template <class Sampler, int streamSize, int streamTileX>
std::shared_ptr<Renderer> RendererFactoryStream::make(const std::string& type, const std::shared_ptr<const Scene>& scene, const std::shared_ptr<Intersector>& cachedIntersector, const Props& props, Props& stats)
{
  static const int streamTileY = (streamSize > 64) ? (streamSize / streamTileX) : (64 / streamTileX);

  // Stream SOP
  Log() << "SOP stream size: " << streamSize;

  std::shared_ptr<IntersectorStream<streamSize>> intersector = std::dynamic_pointer_cast<IntersectorStream<streamSize>>(cachedIntersector);
  if (!intersector)
    intersector = IntersectorFactoryStream::make<streamSize>(scene, props, stats);

  if (type == "primaryStream")
    return std::make_shared<PrimaryRendererStream<ShadingContextSimd, Sampler, true, streamSize, streamTileX, streamTileY>>(scene, intersector, props);
  if (type == "primaryStreamFast")
    return std::make_shared<PrimaryRendererStream<SimpleShadingContextSimd, Sampler, false, streamSize, streamTileX, streamTileY>>(scene, intersector, props);
  if (type == "diffuseStream")
    return std::make_shared<DiffuseRendererStream<ShadingContextSimd, Sampler, true, streamSize, streamTileX, streamTileY>>(scene, intersector, props);
  if (type == "diffuseStreamFast")
    return std::make_shared<DiffuseRendererStream<SimpleShadingContextSimd, Sampler, false, streamSize, streamTileX, streamTileY>>(scene, intersector, props);
  if (type == "diffuse2Stream")
    return std::make_shared<Diffuse2RendererStream<ShadingContextSimd, Sampler, true, streamSize, streamTileX, streamTileY>>(scene, intersector, props);
  if (type == "diffuse2StreamFast")
    return std::make_shared<Diffuse2RendererStream<SimpleShadingContextSimd, Sampler, false, streamSize, streamTileX, streamTileY>>(scene, intersector, props);
  if (type == "aoStream")
    return std::make_shared<AoRendererStream<ShadingContextSimd, Sampler, true, streamSize, streamTileX, streamTileY>>(scene, intersector, props);
  if (type == "aoStreamFast")
    return std::make_shared<AoRendererStream<SimpleShadingContextSimd, Sampler, false, streamSize, streamTileX, streamTileY>>(scene, intersector, props);
  if (type == "pathStream")
  {
    return std::make_shared<PathRendererStream<Sampler, streamSize, streamTileX, streamTileY>>(scene, intersector, props);
  }

  throw std::invalid_argument("invalid renderer type");
}

} // namespace prt

