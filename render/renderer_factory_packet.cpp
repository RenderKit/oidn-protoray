// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "sampling/random_sampler_simd.h"
#include "sampling/sobol_sampler_simd.h"
#include "sampling/sobol_burley_sampler_simd.h"
#include "intersector_factory_packet.h"
#include "renderer_packet.h"
#include "primary_integrator_packet.h"
#include "po_integrator_packet.h"
#include "ao_integrator_packet.h"
#include "diffuse_integrator_packet.h"
#include "diffuse2_integrator_packet.h"
#include "path_integrator_packet.h"
#include "renderer_factory_packet.h"

namespace prt {

std::shared_ptr<Renderer> RendererFactoryPacket::make(const std::string& type, const std::shared_ptr<const Scene>& scene, const std::shared_ptr<Intersector>& intersector, const Props& props, Props& stats)
{
  std::string samplerType = getSamplerType(type, props);
  Log() << "Sampler: " << samplerType;

  if (samplerType == "sobol")
    return make<SobolSamplerSimd>(type, scene, intersector, props, stats);
  if (samplerType == "sobolBurley")
    return make<SobolBurleySamplerSimd<false>>(type, scene, intersector, props, stats);
  if (samplerType == "sobolBurleyBlueNoise")
    return make<SobolBurleySamplerSimd<true>>(type, scene, intersector, props, stats);
  if (samplerType == "random")
    return make<RandomSamplerSimd>(type, scene, intersector, props, stats);

  throw std::invalid_argument("invalid sampler type");
}

template <class Sampler>
std::shared_ptr<Renderer> RendererFactoryPacket::make(const std::string& type, const std::shared_ptr<const Scene>& scene, const std::shared_ptr<Intersector>& cachedIntersector, const Props& props, Props& stats)
{
  std::shared_ptr<IntersectorPacket> intersector = std::dynamic_pointer_cast<IntersectorPacket>(cachedIntersector);
  if (!intersector)
    intersector = IntersectorFactoryPacket::make(scene, props, stats);

  if (type == "primaryPacket")
    return std::make_shared<RendererPacket<PrimaryIntegratorPacket<ShadingContextSimd, Sampler>, Sampler>>(scene, intersector, props);
  if (type == "primaryPacketFast")
    return std::make_shared<RendererPacket<PrimaryIntegratorPacket<SimpleShadingContextSimd, Sampler>, Sampler, false>>(scene, intersector, props);
  if (type == "poPacket")
    return std::make_shared<RendererPacket<PoIntegratorPacket<Sampler>, Sampler>>(scene, intersector, props);
  if (type == "aoPacket")
    return std::make_shared<RendererPacket<AoIntegratorPacket<ShadingContextSimd, Sampler>, Sampler>>(scene, intersector, props);
  if (type == "aoPacketFast")
    return std::make_shared<RendererPacket<AoIntegratorPacket<SimpleShadingContextSimd, Sampler>, Sampler, false>>(scene, intersector, props);
  if (type == "diffusePacket")
    return std::make_shared<RendererPacket<DiffuseIntegratorPacket<ShadingContextSimd, Sampler>, Sampler>>(scene, intersector, props);
  if (type == "diffusePacketFast")
    return std::make_shared<RendererPacket<DiffuseIntegratorPacket<SimpleShadingContextSimd, Sampler>, Sampler, false>>(scene, intersector, props);
  if (type == "diffuse2Packet")
    return std::make_shared<RendererPacket<Diffuse2IntegratorPacket<ShadingContextSimd, Sampler>, Sampler>>(scene, intersector, props);
  if (type == "diffuse2PacketFast")
    return std::make_shared<RendererPacket<Diffuse2IntegratorPacket<SimpleShadingContextSimd, Sampler>, Sampler, false>>(scene, intersector, props);
  if (type == "pathPacket")
    return std::make_shared<RendererPacket<PathIntegratorPacket<Sampler>, Sampler>>(scene, intersector, props);

  throw std::invalid_argument("invalid renderer type");
}

} // namespace prt
