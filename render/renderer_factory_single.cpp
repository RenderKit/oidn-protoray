// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "sampling/random_sampler.h"
#include "sampling/sobol_sampler.h"
#include "sampling/sobol_burley_sampler.h"
#include "intersector_factory_single.h"
#include "renderer_single.h"
#include "primary_integrator_single.h"
#include "po_integrator_single.h"
#include "ao_integrator_single.h"
#include "ao_hit_integrator_single.h"
#include "diffuse_integrator_single.h"
#include "diffuse2_integrator_single.h"
#include "path_integrator_single.h"
#include "debug_integrator_single.h"
#include "renderer_factory_single.h"

namespace prt {

std::shared_ptr<Renderer> RendererFactorySingle::make(const std::string& type, const std::shared_ptr<const Scene>& scene, const std::shared_ptr<Intersector>& intersector, const Props& props, Props& stats)
{
  std::string samplerType = getSamplerType(type, props);
  Log() << "Sampler: " << samplerType;

  if (samplerType == "sobol")
    return make<SobolSampler>(type, scene, intersector, props, stats);
  if (samplerType == "sobolBurley")
    return make<SobolBurleySampler<false>>(type, scene, intersector, props, stats);
  if (samplerType == "sobolBurleyBlueNoise")
    return make<SobolBurleySampler<true>>(type, scene, intersector, props, stats);
  if (samplerType == "random")
    return make<RandomSampler>(type, scene, intersector, props, stats);

  throw std::invalid_argument("invalid sampler type");
}

template <class Sampler>
std::shared_ptr<Renderer> RendererFactorySingle::make(const std::string& type, const std::shared_ptr<const Scene>& scene, const std::shared_ptr<Intersector>& cachedIntersector, const Props& props, Props& stats)
{
  std::shared_ptr<IntersectorSingle> intersector = std::dynamic_pointer_cast<IntersectorSingle>(cachedIntersector);
  if (!intersector)
    intersector = IntersectorFactorySingle::make(scene, props, stats);

  if (type == "primary")
    return std::make_shared<RendererSingle<PrimaryIntegratorSingle<ShadingContext, Sampler>, Sampler>>(scene, intersector, props);
  if (type == "primaryFast")
    return std::make_shared<RendererSingle<PrimaryIntegratorSingle<SimpleShadingContext, Sampler>, Sampler, false>>(scene, intersector, props);
  if (type == "po")
    return std::make_shared<RendererSingle<PoIntegratorSingle<Sampler>, Sampler>>(scene, intersector, props);
  if (type == "ao")
    return std::make_shared<RendererSingle<AoIntegratorSingle<Sampler>, Sampler>>(scene, intersector, props);
  if (type == "aoHit")
    return std::make_shared<RendererSingle<AoHitIntegratorSingle<Sampler>, Sampler>>(scene, intersector, props);
  if (type == "diffuse")
    return std::make_shared<RendererSingle<DiffuseIntegratorSingle<ShadingContext, Sampler>, Sampler>>(scene, intersector, props);
  if (type == "diffuseFast")
    return std::make_shared<RendererSingle<DiffuseIntegratorSingle<SimpleShadingContext, Sampler>, Sampler, false>>(scene, intersector, props);
  if (type == "diffuse2")
    return std::make_shared<RendererSingle<Diffuse2IntegratorSingle<ShadingContext, Sampler>, Sampler>>(scene, intersector, props);
  if (type == "diffuse2Fast")
    return std::make_shared<RendererSingle<Diffuse2IntegratorSingle<SimpleShadingContext, Sampler>, Sampler, false>>(scene, intersector, props);
  if (type == "path")
    return std::make_shared<RendererSingle<PathIntegratorSingle<Sampler>, Sampler>>(scene, intersector, props);
  if (type == "debug")
    return std::make_shared<RendererSingle<DebugIntegratorSingle<Sampler>, Sampler>>(scene, intersector, props);

  throw std::invalid_argument("invalid renderer type");
}

} // namespace prt
