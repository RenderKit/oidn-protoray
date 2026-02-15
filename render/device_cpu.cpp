// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "sys/sysinfo.h"
#include "camera/pinhole_camera.h"
#include "camera/thin_lens_camera.h"
#include "image/linear_tone_mapper.h"
#include "image/reinhard_tone_mapper.h"
#include "image/aces_tone_mapper.h"
#include "image/filmic_tone_mapper.h"
#include "image/log_tone_mapper.h"
#include "renderer_factory_single.h"
#include "renderer_factory_packet.h"
#include "renderer_factory_stream.h"
#include "bsdf/albedo_tables.h"
#include "device_cpu.h"

namespace prt {

DeviceCpu::DeviceCpu()
{
  precomputeAlbedoTables();
}

std::string DeviceCpu::getInfo()
{
  CpuInfo cpuInfo;
  getCpuInfo(cpuInfo);
  return cpuInfo.brand;
}

void DeviceCpu::initRenderer(const Props& props, Props& stats)
{
  std::string type = props.get("type");

  if (type.find("Stream") != std::string::npos)
    renderer = RendererFactoryStream::make(type, scene, intersector, props, stats);
  else if (type.find("Packet") != std::string::npos)
    renderer = RendererFactoryPacket::make(type, scene, intersector, props, stats);
  else
    renderer = RendererFactorySingle::make(type, scene, intersector, props, stats);

  intersector = renderer->getIntersector();

  samplerType = RendererFactory::getSamplerType(type, props);
}

void DeviceCpu::initSampler(unsigned int seed)
{
  renderer->initSampler(seed);
}

void DeviceCpu::render(Props& stats)
{
  frameBuffer->begin();
  renderer->render(camera.get(), prevCamera.get(), nextCamera.get(),
           frameBuffer.get(), stats);
  frameBuffer->finish();
}

Props DeviceCpu::queryRay(const Ray& ray)
{
  return renderer->queryRay(ray);
}

bool DeviceCpu::queryOcclusionRay(const Ray& ray)
{
  return renderer->queryOcclusionRay(ray);
}

Props DeviceCpu::queryPixel(const Vec2f& point)
{
  return renderer->queryPixel(camera.get(), point);
}

void DeviceCpu::queryPixel(const Vec2f& point, Array<Props>& hits)
{
  renderer->queryPixel(camera.get(), point, hits);
}

Props DeviceCpu::queryPixel(int x, int y)
{
  return renderer->queryPixel(camera.get(), x, y);
}

void DeviceCpu::queryPixel(int x, int y, Array<Props>& hits)
{
  renderer->queryPixel(camera.get(), x, y, hits);
}

void DeviceCpu::updateRenderer(const Props& props)
{
  renderer->update(props);
}

void DeviceCpu::initScene(const std::string& path, const Props& props)
{
  intersector.reset();
  scene = std::make_shared<Scene>(path, props);
}

Box3f DeviceCpu::getSceneBounds()
{
  return scene->getBounds();
}

void DeviceCpu::initLights(const Props& props)
{
  scene->initLights(props);
}

void DeviceCpu::mutateMaterials(Random& rng, bool mutateRegular, bool mutateEmissive)
{
  scene->mutateMaterials(rng, mutateRegular, mutateEmissive);
}

std::shared_ptr<Camera> DeviceCpu::makeCamera(const Props& props)
{
  std::string type = props.get("type");
  float lensRadius = props.get("lensRadius", 0.f);

  if (type == "pinhole" || lensRadius == 0.f)
    return std::make_shared<PinholeCamera>(props);
  else if (type == "thinlens")
    return std::make_shared<ThinLensCamera>(props);
  else
    throw std::invalid_argument("invalid camera type");
}

void DeviceCpu::initCamera(const Props& props)
{
  camera = makeCamera(props);
  prevCamera = nextCamera = camera;
}

void DeviceCpu::initCamera(const Props& props, const Props& prevProps, const Props& nextProps)
{
  camera = makeCamera(props);
  prevCamera = makeCamera(prevProps);
  nextCamera = makeCamera(nextProps);
}

Mat4f DeviceCpu::getWorldToViewD3D()
{
  return camera->worldToViewD3D;
}

Mat4f DeviceCpu::getViewToClipD3D()
{
  return camera->viewToClipD3D;
}

Mat4f DeviceCpu::getWorldToRaster()
{
  return camera->worldToRaster;
}

bool DeviceCpu::hasJitter()
{
  return frameBuffer->hasJitter();
}

Vec2f DeviceCpu::getJitter()
{
  return frameBuffer->getJitter();
}

void DeviceCpu::setJitter(const Vec2f& jitter)
{
  frameBuffer->setJitter(jitter);
}

void DeviceCpu::disableJitter()
{
  frameBuffer->disableJitter();
}

void DeviceCpu::initFrame(const Vec2i& size, const Props& props)
{
  const int varFlags = AccumFlag::SampleVar /*| AccumFlag::HalfBuffer*/;

  frameBuffer = std::make_shared<FrameBuffer>(size, (props.exists("aovHdrVar") ? varFlags : 0) | AccumFlag::CheckpointBuffer);

  if (props.exists("aovHdrUnclamp"))
    frameBuffer->getColorUnclamp().alloc(size, props.exists("aovHdrUnclampVar") ? varFlags : 0);

  if (props.exists("aovAlbedo"))
    frameBuffer->getAlbedo().alloc(size, props.exists("aovAlbedoVar") ? varFlags : 0);
  if (props.exists("aovAlbedo1"))
    frameBuffer->getAlbedo1().alloc(size);

  if (props.exists("aovDiffuseAlbedo"))
    frameBuffer->getDiffuseAlbedo().alloc(size);
  if (props.exists("aovDiffuseAlbedo1"))
    frameBuffer->getDiffuseAlbedo1().alloc(size);

  if (props.exists("aovSpecularAlbedo"))
    frameBuffer->getSpecularAlbedo().alloc(size);
  if (props.exists("aovSpecularAlbedo1"))
    frameBuffer->getSpecularAlbedo1().alloc(size);

  if (props.exists("aovNormal"))
    frameBuffer->getNormal().alloc(size, props.exists("aovNormalVar") ? varFlags : 0);
  if (props.exists("aovNormal1"))
    frameBuffer->getNormal1().alloc(size);

  if (props.exists("aovDepth"))
    frameBuffer->getDepth().alloc(size, props.exists("aovDepthVar") ? varFlags : 0);

  if (props.exists("aovHWDepth"))
    frameBuffer->getHWDepth().alloc(size);

  if (props.exists("aovRoughness"))
    frameBuffer->getRoughness().alloc(size);
  if (props.exists("aovRoughness1"))
    frameBuffer->getRoughness1().alloc(size);

  if (props.exists("aovAlpha"))
    frameBuffer->getAlpha().alloc(size);

  if (props.exists("aovSh"))
  {
    for (int i = 0; i < 4; ++i)
      frameBuffer->getSh(i).alloc(size);
  }

  if (props.exists("aovMotionBack"))
    frameBuffer->getMotionBack().alloc(size);

  if (props.exists("aovMotionFore"))
    frameBuffer->getMotionFore().alloc(size);
}

void DeviceCpu::initFilter(const Props& props)
{
  if (!frameBuffer) return;

  std::shared_ptr<PixelFilter> filter;
  std::string type = props.get("filter");
  filterType = type;
  Log() << "Filter: " << type;

  if (type == "box")
    return;
  else if (type == "tent" || type == "triangle")
  {
    float width = props.get("filterWidth", 2.f);
    filter = std::make_shared<TriangleFilter>(width);
  }
  else if (type == "gauss" || type == "gaussian")
  {
    float width = props.get("filterWidth", 3.f);
    float sigma = props.get("filterSigma", width / 6.f);
    bool bias = props.get("filterBias", 1);
    filter = std::make_shared<GaussianFilter>(width, sigma, bias);
    Log() << "Filter sigma: " << sigma;
    Log() << "Filter bias: " << bias;
  }
  else if (type == "blackmanHarris" || type == "blackmanharris")
  {
    float width = props.get("filterWidth", 3.f);
    filter = std::make_shared<BlackmanHarrisFilter>(width);
  }
  else
    throw std::invalid_argument("invalid pixel filter type");

  filter->init();
  Log() << "Filter width: " << filter->getWidth();

  frameBuffer->setFilter(filter);

  std::string mode = props.get("filterMode", "sample");
  Log() << "Filter mode: " << mode;

  if (mode == "sample")
    frameBuffer->setFilterMode(PixelFilterMode::Sample);
  else if (mode == "splat")
    frameBuffer->setFilterMode(PixelFilterMode::Splat);
  else
    throw std::invalid_argument("invalid pixel filter mode");
}

void DeviceCpu::initToneMapper(const Props& props)
{
  if (!frameBuffer) return;

  std::shared_ptr<ToneMapper> toneMapper;
  std::string type = props.get("type");

  if (type == "linear")
    toneMapper = std::make_shared<LinearToneMapper>(props);
  else if (type == "reinhard")
    toneMapper = std::make_shared<ReinhardToneMapper>(props);
  else if (type == "aces")
    toneMapper = std::make_shared<AcesToneMapper>(props);
  else if (type == "filmic")
    toneMapper = std::make_shared<FilmicToneMapper>(props);
  else if (type == "log")
    toneMapper = std::make_shared<LogToneMapper>(props);
  else if (type != "none")
    throw std::invalid_argument("invalid tone mapper type");

  frameBuffer->setToneMapper(toneMapper);
}

void DeviceCpu::clearFrame()
{
  frameBuffer->clear();
}

void DeviceCpu::blitFrame(Surface& dest)
{
  frameBuffer->getColor().blitLdr(dest, frameBuffer->getToneMapper());
}

void DeviceCpu::blitFrame(FrameAovs& dest)
{
  if (dest.hdr)
    frameBuffer->getColor().blit(dest.hdr, dest.hdrVar, dest.hdrHalf);

  if (dest.ldr)
    frameBuffer->getColor().blitLdr(dest.ldr, frameBuffer->getToneMapper());

  if (dest.hdrUnclamp)
    frameBuffer->getColorUnclamp().blit(dest.hdrUnclamp, dest.hdrUnclampVar, dest.hdrUnclampHalf);

  if (dest.albedo)
    frameBuffer->getAlbedo().blit(dest.albedo, dest.albedoVar, dest.albedoHalf);
  if (dest.albedo1)
    frameBuffer->getAlbedo1().blit(dest.albedo1);

  if (dest.diffuseAlbedo)
    frameBuffer->getDiffuseAlbedo().blit(dest.diffuseAlbedo);
  if (dest.diffuseAlbedo1)
    frameBuffer->getDiffuseAlbedo1().blit(dest.diffuseAlbedo1);

  if (dest.specularAlbedo)
    frameBuffer->getSpecularAlbedo().blit(dest.specularAlbedo);
  if (dest.specularAlbedo1)
    frameBuffer->getSpecularAlbedo1().blit(dest.specularAlbedo1);

  if (dest.normal)
    frameBuffer->getNormal().blit(dest.normal, dest.normalVar, dest.normalHalf);
  if (dest.normal1)
    frameBuffer->getNormal1().blit(dest.normal1);

  // if (dest.cameraNormal)
  //     frameBuffer->getCameraNormal().blit(dest.cameraNormal, dest.cameraNormalVar, dest.cameraNormalHalf);

  if (dest.depth)
    frameBuffer->getDepth().blit(dest.depth, dest.depthVar, dest.depthHalf);

  if (dest.hwDepth)
    frameBuffer->getHWDepth().blit(dest.hwDepth, dest.hwDepthVar, dest.hwDepthHalf);

  if (dest.roughness)
    frameBuffer->getRoughness().blit(dest.roughness);
  if (dest.roughness1)
    frameBuffer->getRoughness1().blit(dest.roughness1);

  if (dest.alpha)
    frameBuffer->getAlpha().blit(dest.alpha);

  if (dest.sh[0])
  {
    frameBuffer->getSh(0).blit(dest.sh[0]);
    for (int i = 1; i < 4; ++i)
      frameBuffer->getSh(i).blitRel(dest.sh[i], dest.sh[0]);
  }

  if (dest.motionBack)
    frameBuffer->getMotionBack().blit(dest.motionBack);

  if (dest.motionFore)
    frameBuffer->getMotionFore().blit(dest.motionFore);
}

void DeviceCpu::blitFrameLdr(Surface& dest, const float* src, float offset, float scale)
{
  Vec2i size(dest.width, dest.height);

  tbb::parallel_for(tbb::blocked_range<int>(0, size.y), [&](const tbb::blocked_range<int>& r)
  {
    for (int y = r.begin(); y != r.end(); ++y)
    {
      int i = y * size.x;
      int* outRow = dest.getRow(y);
      for (int x = 0; x < size.x; ++x)
        outRow[x] = encodeBgr8((src[i+x] + offset) * scale);
    }
  });
}

void DeviceCpu::blitFrameLdr(Surface& dest, const Vec3f* src, float offset, float scale)
{
  Vec2i size(dest.width, dest.height);

  tbb::parallel_for(tbb::blocked_range<int>(0, size.y), [&](const tbb::blocked_range<int>& r)
  {
    for (int y = r.begin(); y != r.end(); ++y)
    {
      int i = y * size.x;
      int* outRow = dest.getRow(y);
      for (int x = 0; x < size.x; ++x)
        outRow[x] = encodeBgr8((src[i+x] + offset) * scale);
    }
  });
}

void DeviceCpu::blitFrameHdr(Surface& dest, const Vec3f* src)
{
  ToneMapper* toneMapper = frameBuffer->getToneMapper();
  Vec2i size(dest.width, dest.height);

  tbb::parallel_for(tbb::blocked_range<int>(0, size.y), [&](const tbb::blocked_range<int>& r)
  {
    for (int y = r.begin(); y != r.end(); ++y)
    {
      int i = y * size.x;
      int* outRow = dest.getRow(y);
      for (int x = 0; x < size.x; ++x)
      {
        Vec3f c = src[i+x];
        if (toneMapper)
          c = toneMapper->get(c);
        outRow[x] = encodeBgr8(c);
      }
    }
  });
}

float DeviceCpu::getFrameError()
{
  return frameBuffer->getColor().getError(frameBuffer->getToneMapper());
}

} // namespace prt


