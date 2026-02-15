// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <fstream>
#include "sys/sysinfo.h"
#include "sys/logging.h"
#include "sys/filesystem.h"
#include "sys/tasking.h"
#include "math/math.h"
#include "sampling/shape_sampler.h"
#include "image/image.h"
#include "image/color.h"
#include "image/pixel.h"
#ifdef PRT_OIDN_SUPPORT
#include "image/oidn_denoiser.h"
#endif
#include "material/material.h"
#include "rkscene/json.hpp"
#include "render_window.h"

namespace prt {

using json = nlohmann::json;

inline json toJSON(const Vec2f& v) { return {v.x, v.y}; }
inline json toJSON(const Vec3f& v) { return {v.x, v.y, v.z}; }
inline json toJSON(const Vec4f& v) { return {v.x, v.y, v.z, v.w}; }

inline json toJSON(const Mat4f& m)
{
  json j = json::array();
  for (int row = 0; row < 4; ++row)
    j.push_back(toJSON(Vec4f(m[0][row], m[1][row], m[2][row], m[3][row])));
  return j;
}

bool saveJson(const std::string& filename, const json& json)
{
  Log() << "Saving JSON: " << filename;
  std::ofstream file(filename);
  if (!file)
    return false;
  file << json.dump(2);
  return file.good();
}

void saveJsonSafe(const std::string& filename, const json& json)
{
  for (int i = 0; i < 3; ++i)
  {
    if (saveJson(filename, json))
      return;
  }
}

RenderWindow::RenderWindow(int width, int height, DisplayMode mode, const std::shared_ptr<Device>& device, const Props& props)
  : Window(width, height, mode)
{
  this->device = device;
  this->props = props;

  // Initialize RNG
  seed = props.get<uint64_t>("seed", 0, 16);
  rng.reset(seed);
  Log() << "Seed: " << std::hex << std::setfill('0') << std::setw(16) << seed;

  // Initialize output
  batchCount = props.exists("batch") ? props.get("batch", std::numeric_limits<int>::max()-1) : 0;
  isBatchMode = batchCount > 0;

  outputPrefix = props.get("benchmark", "");
  isBenchmarkMode = !outputPrefix.empty();

  if (props.exists("output"))
  {
    outputPrefix = props.get("output");
    auto pos = outputPrefix.find_last_of("/\\");
    if (pos == std::string::npos)
      outputId = outputPrefix;
    else
      outputId = outputPrefix.substr(pos + 1);
  }

  if (!outputPrefix.empty() && isBatchMode)
  {
    if (!isDirectory(outputPrefix))
    {
      if (!makeDirectory(outputPrefix))
        LogError() << "Could not create directory: " << outputPrefix;
    }

    outputPrefix += "/";
  }

  isOutputEnabled = props.exists("output") || !outputPrefix.empty();

  runTimeThreshold = posInf;
  std::string runTimeStr = props.get("runtime", props.get("t", ""));
  if (!runTimeStr.empty())
  {
    runTimeThreshold = parseTime(runTimeStr);
    Log() << "Run time limit: " << int64_t(runTimeThreshold) << " s";
  }

  autoShotThreshold = props.get("autoshot", 0.0);
  isCheckpointEnabled = oriIsCheckpointEnabled = props.exists("checkpoint"); // save a screenshot at every power-of-two spp
  minSpp = props.get("minSpp", 0);
  maxSpp = oriMaxSpp = props.get("spp", 0);
  adaptiveMaxSpp = INT_MAX;
  multiresMaxSpp = props.get("multiresSpp", 0);
  multiresMinSpp = props.get("multiresMinSpp", 0);
  adaptiveMultiresMaxSpp = INT_MAX;
  minVarSpp = max(props.get("minVarSpp", 4), 4);
  maxNoise = props.get("maxNoise", 0.f);
  multiresMaxNoise = props.get("multiresMaxNoise", 0.f);
  prevNoise = 0.f;
  jitterStart = props.get("jitter", 0);
  jitter = jitterStart > 0;
  warmupSpp = props.get("warmup", maxSpp / 6);

  presetIndex = 0;
  if (multiresMaxSpp > 0 && batchCount > 0)
  {
    if (jitter)
      throw std::invalid_argument("cannot set jitter in multires batch mode");
    if (props.exists("sampler"))
      throw std::invalid_argument("cannot set sampler in multires batch mode");
    if (!isPowerOfTwo(multiresMaxSpp))
      throw std::invalid_argument("multiresSpp must be a power of two");
    initPresets(false);
  }
  else
  {
    presetCountPerRes = 1;
    presetCount = 1;
  }

  sharpness = -1.f;

  sequenceFrameCount = max(props.get("sequence", 1), 1);
  sequenceFrameIndex = 0;
  isSequencePlaying = false;

  // Init renderer
  oriImageSize = Vec2i(width, height);
  initRenderer(width, height);

  // Setup view
  const float autoSceneScale = reduceMin(device->getSceneBounds().getSize()) * 0.1f;
  sceneScale = props.get("sceneScale", autoSceneScale);
  nearClip = props.get("nearClip", 0.01f * sceneScale); // 1cm

  viewFilename = props.get("viewFile", "default.view");
  defaultViewId = props.get("view", 0);

  printCamera = false;

  poiFilename = props.get("poiFile", "default.poi");
  isPoiMode = false;
  mousePosX = -1;
  mousePosY = -1;

  lightDir = props.get("distantLightDir", props.get("sunSkyLightDir", Vec3f(0.f, 1.f, 0.f)));

  const int isMutateEnabled = props.get("mutate", 1);
  isMutateViewEnabled = props.get("mutateView", isMutateEnabled);
  isMutateLightEnabled = props.get("mutateLight", isMutateEnabled);
  isMutateLightColorEnabled = props.get("mutateLightColor", isMutateEnabled);
  isMutateMtlEnabled = props.get("mutateMtl", isMutateEnabled);
  isMutateFilterEnabled = props.get("mutateFilter",
    int(isMutateEnabled && !props.exists("filter") && !props.exists("filterWidth") && !jitter && presetCount <= 1));
  isMutateSamplerEnabled = props.get("mutateSampler", isMutateEnabled);
  isMutateRenderEnabled = props.get("mutateRender", isMutateEnabled);
  maxViewDist = props.get("maxViewDist", float(posMax));

  // Setup tone mapping
  toneMapperNames.pushBack("none", "filmic", "aces", "reinhard", "log", "linear");
  std::string toneMapperName = props.get("tonemap", "none");
  tone.typeId = 0;
  for (int i = 0; i < toneMapperNames.getSize(); ++i)
  {
    if (toneMapperNames[i] == toneMapperName)
    {
      tone.typeId = i;
      break;
    }
  }

  tone.ev = props.get("ev", 0.f);
  tone.burn = props.get("burn", 0.1f);

  prevTone.typeId = -1;
  prevTone.ev = qnan;
  prevTone.burn = qnan;

  isAutoExposureMode = props.exists("evAuto");
  evMin = props.get("evMin", float(negInf));
  evMax = props.get("evMax", float(posInf));
  evCompMin = props.get("evCompMin", float(-1.f));
  evCompMax = props.get("evCompMax", float(1.f));

  if (props.exists("autoMaxRadiance"))
  {
    maxRadiance = props.get("maxRadiance", 10.f);
    //autoMaxRadiance = maxRadiance * pow(2.f, tone.ev);
    autoMaxRadiance = 128.f;
  }
  else
  {
    maxRadiance = props.get("maxRadiance", float(posMax));
    autoMaxRadiance = 0.f;
  }

  // Setup denoiser
  denoiserNames.pushBack("none");
#ifdef PRT_OIDN_SUPPORT
  denoiserNames.pushBack("oidn");
#endif
  std::string denoiserName = props.get("denoiser", "none");
  denoiserId = 0;
  for (int i = 0; i < denoiserNames.getSize(); ++i)
  {
    if (denoiserNames[i] == denoiserName)
    {
      denoiserId = i;
      break;
    }
  }

  std::string denoiserModeName = props.get("denoiserMode", "hdr");
  if (denoiserModeName == "ldr")
    denoiserMode = DenoiserMode::Ldr;
  else if (denoiserModeName == "hdr")
    denoiserMode = DenoiserMode::Hdr;
  else
    throw std::runtime_error("invalid denoiserMode");

  denoisers.alloc(denoiserNames.getSize());
  initDenoiser();
  denoiserSpp = props.get("denoiserSpp", 0);

  isTextEnabled = !props.exists("no-overlay");
  isFrameStatsPrinted = false;
  currentFps = 0;
  currentMray = 0;
  displayTimeSum = 0;
  displayTimeCount = 0;

  deviceInfo = device->getInfo();
  isDemoMode = props.exists("demo");
  this->buildStats = buildStats;

  maxDepth = props.get("maxDepth", 100);
  isSceneChanged = false;

  isIrradianceMode = props.exists("irradiance");
}

void RenderWindow::initRenderer(int width, int height)
{
  // Set window size
  setSize(width, height);

  // Init framebuffer
  bool aovAll = props.exists("aovAll") || props.exists("outAux");
  bool aovDefault = props.exists("aov") || props.exists("aovDefault") || aovAll;
  Props frameProps = props;

  if (aovDefault)
  {
    frameProps.set("aovHdrUnclamp");
    frameProps.set("aovDepth");
    frameProps.set("aovHWDepth");
    frameProps.set("aovAlpha");

    if (!jitter)
    {
      //frameProps.set("aovLdr");
      frameProps.set("aovAlbedo");
      // frameProps.set("aovDiffuseAlbedo");
      // frameProps.set("aovSpecularAlbedo");
      frameProps.set("aovNormal");
      frameProps.set("aovRoughness");

      if (maxSpp >= minVarSpp)
      {
        frameProps.set("aovHdrVar");
        frameProps.set("aovHdrUnclampVar");
        frameProps.set("aovAlbedoVar");
        frameProps.set("aovNormalVar");
        //frameProps.set("aovDepthVar");
      }
    }
    else
    {
      frameProps.set("aovAlbedo1");
      frameProps.set("aovDiffuseAlbedo1");
      frameProps.set("aovSpecularAlbedo1");
      frameProps.set("aovNormal1");
      frameProps.set("aovRoughness1");
    }

    if (props.exists("irradiance"))
      frameProps.set("aovSh");

    if (sequenceFrameCount > 1 || aovAll)
    {
      frameProps.set("aovMotionBack");
      frameProps.set("aovMotionFore");
    }
  }

  imageSize = Vec2i(width, height);
  device->initFrame(imageSize, frameProps);

  aovId = 0;
  aovNames.clear();
  aovNames.pushBack("color");

  hdrVarBuffer.free();
  hdrUnclampBuffer.free();
  hdrUnclampVarBuffer.free();
  ldrBuffer.free();
  albedoBuffer.free();
  albedoVarBuffer.free();
  albedo1Buffer.free();
  diffuseAlbedoBuffer.free();
  diffuseAlbedo1Buffer.free();
  specularAlbedoBuffer.free();
  specularAlbedo1Buffer.free();
  normalBuffer.free();
  normalVarBuffer.free();
  normal1Buffer.free();
  depthBuffer.free();
  depthVarBuffer.free();
  hwDepthBuffer.free();
  roughnessBuffer.free();
  roughness1Buffer.free();
  alphaBuffer.free();
  for (int i = 0; i < 4; ++i)
    shBuffer[i].free();
  motionBackBuffer.free();
  motionForeBuffer.free();

  if (frameProps.exists("aovHdrVar"))
  {
    hdrVarBuffer.alloc(imageSize);
    //hdrHalfBuffer.alloc(imageSize);
  }

  if (frameProps.exists("aovHdrUnclamp"))
  {
    hdrUnclampBuffer.alloc(imageSize);
    if (frameProps.exists("aovHdrUnclampVar"))
    {
      hdrUnclampVarBuffer.alloc(imageSize);
      //hdrUnclampHalfBuffer.alloc(imageSize);
    }
  }

  if (frameProps.exists("aovLdr"))
    ldrBuffer.alloc(imageSize);

  if (frameProps.exists("aovAlbedo"))
  {
    albedoBuffer.alloc(imageSize);
    if (frameProps.exists("aovAlbedoVar"))
    {
      albedoVarBuffer.alloc(imageSize);
      //albedoHalfBuffer.alloc(imageSize);
    }
    aovNames.pushBack("albedo");
  }

  if (frameProps.exists("aovAlbedo1"))
    albedo1Buffer.alloc(imageSize);

  if (frameProps.exists("aovDiffuseAlbedo"))
    diffuseAlbedoBuffer.alloc(imageSize);

  if (frameProps.exists("aovDiffuseAlbedo1"))
  {
    diffuseAlbedo1Buffer.alloc(imageSize);
    aovNames.pushBack("diffuseAlbedo1");
  }

  if (frameProps.exists("aovSpecularAlbedo"))
    specularAlbedoBuffer.alloc(imageSize);

  if (frameProps.exists("aovSpecularAlbedo1"))
  {
    specularAlbedo1Buffer.alloc(imageSize);
    aovNames.pushBack("specularAlbedo1");
  }

  if (frameProps.exists("aovNormal"))
  {
    normalBuffer.alloc(imageSize);
    if (frameProps.exists("aovNormalVar"))
    {
      normalVarBuffer.alloc(imageSize);
      //normalHalfBuffer.alloc(imageSize);
    }
    aovNames.pushBack("normal");
  }

  if (frameProps.exists("aovNormal1"))
    normal1Buffer.alloc(imageSize);

  if (frameProps.exists("aovDepth"))
  {
    depthBuffer.alloc(imageSize);
    if (frameProps.exists("aovDepthVar"))
    {
      depthVarBuffer.alloc(imageSize);
      //depthHalfBuffer.alloc(imageSize);
    }
  }

  if (frameProps.exists("aovHWDepth"))
  {
    hwDepthBuffer.alloc(imageSize);
    aovNames.pushBack("hwDepth");
  }

  if (frameProps.exists("aovRoughness"))
  {
    roughnessBuffer.alloc(imageSize);
    aovNames.pushBack("roughness");
  }

  if (frameProps.exists("aovRoughness1"))
    roughness1Buffer.alloc(imageSize);

  if (frameProps.exists("aovAlpha"))
  {
    alphaBuffer.alloc(imageSize);
    aovNames.pushBack("alpha");
  }

  if (frameProps.exists("aovSh"))
  {
    for (int i = 0; i < 4; ++i)
      shBuffer[i].alloc(imageSize);
    aovNames.pushBack("sh0");
    aovNames.pushBack("sh1x");
    aovNames.pushBack("sh1y");
    aovNames.pushBack("sh1z");
  }

  if (frameProps.exists("aovMotionBack"))
  {
    motionBackBuffer.alloc(imageSize);
    //aovNames.pushBack("motionBack");
  }

  if (frameProps.exists("aovMotionFore"))
  {
    motionForeBuffer.alloc(imageSize);
    //aovNames.pushBack("motionFore");
  }

  // Init filter
  if (props.exists("filter"))
    device->initFilter(props);

  // Init renderer
  std::string rendererId = props.get("renderer", "pathStream");
  Props rendererProps = props;
  rendererProps.set("type", rendererId);
  rendererProps.set("imageSize", imageSize);
  Props buildStats;
  device->initRenderer(rendererProps, buildStats);

  // Force rendering frame from scratch
  oldView = {};

  // Force tone mapping initialization
  prevTone.typeId = -1;
}

void RenderWindow::initDenoiser()
{
  if (denoiserId != 0 && !denoisers[denoiserId].ldr && !denoisers[denoiserId].hdr)
  {
  #if PRT_OIDN_SUPPORT
    if (denoiserNames[denoiserId] == "oidn")
    {
      if (!isIrradianceMode)
        denoisers[denoiserId].ldr = std::make_shared<OIDNDenoiser>();
      denoisers[denoiserId].hdr = std::make_shared<OIDNDenoiser>();
    }
  #endif

    if (denoiseBuffer.getSize() != imageSize)
      denoiseBuffer.alloc(imageSize);

    Denoiser::Params params;
    params.type    = isIrradianceMode ? "RTLightmap" : "RT";
    params.albedo  = (float*)albedoBuffer.getData();
    //params.normal  = (float*)((denoiserNames[denoiserId] == "optix") ? cameraNormalBuffer.getData() : normalBuffer.getData());
    params.normal  = (float*)normalBuffer.getData();
    params.output  = (float*)denoiseBuffer.getData();
    params.width   = imageSize.x;
    params.height  = imageSize.y;
    params.verbose = 1;

    if (denoisers[denoiserId].ldr)
    {
      if (ldrBuffer.getSize() != imageSize)
        ldrBuffer.alloc(imageSize);
      params.color = (float*)ldrBuffer.getData();
      params.hdr   = false;
      denoisers[denoiserId].ldr->init(params);
    }

    if (denoisers[denoiserId].hdr)
    {
      if (hdrBuffer.getSize() != imageSize)
        hdrBuffer.alloc(imageSize);
      params.color = (float*)hdrBuffer.getData();
      params.hdr   = true;
      denoisers[denoiserId].hdr->init(params);
    }
  }

  denoiser = (denoiserMode == DenoiserMode::Ldr) ? denoisers[denoiserId].ldr : denoisers[denoiserId].hdr;
}

void RenderWindow::onInit()
{
  // Reset all views
  viewId = -1;
  resetView();

  for (int i = 0; i < ViewSet::size; ++i)
    viewSet.views[i] = view;

  // Initialize the sequence keyframes
  for (int i = 0; i < 2; ++i)
  {
    sequenceKeyframes[i].view = view;
    sequenceKeyframes[i].lightDir = lightDir;
  }

  // Try to load the view set
  loadViewSet(viewFilename, viewSet);

  // Set the active view to the default
  setActiveView(defaultViewId);
  copiedView = view;
  copiedLightDir = lightDir;

  // Compute camera movement parameters
  viewPosStep = 0.2f * sceneScale;
  viewPosSpeed = 1.f;
  viewPosSpeedMul = 1.05f;
  viewAngleStep = 1.5f / (float)imageSize.y;
  viewPosDelta = zero;
  viewRollStep = degToRad(2.f);
  viewRollDelta = 0.f;
  viewFovStep = degToRad(0.5f);
  viewFovDelta = 0.f;
  viewRadiusStep = viewPosStep * 0.005f;
  viewRadiusDelta = 0.f;

  isMouseActive = false;
  isCameraRotateMode = true;

  // Try to load the POI set
  loadPoiSet(poiFilename, poiSet);

  // Init the frame stats
  frameCount = 0;
  spp = 0;

  // Benchmark mode
  if (isBenchmarkMode)
  {
    Log() << "Benchmark mode";
    setInputEnabled(false);
  }

  // Batch mode
  if (isBatchMode)
  {
    Log() << "Batch rendering mode";
    setInputEnabled(false);

    batchCount++;
    isAutoExposureMode = false;
    spp = maxSpp;
    presetIndex = presetCount - 1;
    sequenceFrameIndex = sequenceFrameCount - 1;
    isSequencePlaying = sequenceFrameCount > 1;
  }

  timer.reset();
  statsTimer.reset();
  screenshotTimer.reset();
}

void RenderWindow::onDestroy()
{
  Props avgStats = buildStats;
  statsRecorder.getAverage(avgStats);
  Log() << "Average: " << avgStats;

  if (isBenchmarkMode)
  {
    saveAvgStats(avgStats, outputPrefix + ".txt");
    saveFullStats(outputPrefix + ".csv");
    saveScreenshot(outputPrefix);
  }
  else
  {
    if (isOutputEnabled && !isBatchMode && !(isCheckpointEnabled && isPowerOfTwo(spp)))
    {
      if (isCheckpointEnabled || outputPrefix.empty())
        saveAutoScreenshot();
      else
        saveScreenshot(outputPrefix);
    }
  }
}

void RenderWindow::onRender()
{
  // Batch rendering
  if (batchCount > 0)
  {
    if (isAutoExposureMode && (spp >= 16) && (spp < 256) && (abs(evPrev - tone.ev) < 0.0001f))
    {
      Log() << "Final rendering";
      isAutoExposureMode = false;
      if (autoMaxRadiance > 0.f)
        setAutoMaxRadiance();
      tone.ev += rng.get1f(evCompMin, evCompMax); // apply random EV compensation
      Log() << "Final EV: " << tone.ev;
      isSceneChanged = true; // reset rendering the frame
    }
    else if (!isAutoExposureMode && (spp == maxSpp))
    {
      if (!nextPreset())
      {
        // Current frame is complete
        if (!isSequencePlaying || ++sequenceFrameIndex == sequenceFrameCount)
        {
          // Batch is complete
          batchCount--;
          adaptiveMaxSpp = INT_MAX;
          adaptiveMultiresMaxSpp = INT_MAX;

          // Create a special file indicating completion
          if (!doneFilename.empty())
          {
            std::ofstream f(doneFilename);
            Log() << "Done";
          }
        }

        if (batchCount > 0)
        {
          // Switch to the first rendering preset
          presetIndex = -1;
          nextPreset();

          if (!isSequencePlaying || sequenceFrameIndex == sequenceFrameCount)
          {
            // Begin the next batch
            Log() << "Setting exposure";
            isAutoExposureMode = true;
            evPrev = posInf;
            if (autoMaxRadiance > 0.f)
              resetMaxRadiance();

            mutate();
          }

          if (isSequencePlaying)
            setSequenceFrame(sequenceFrameIndex);
        }
        else
        {
          quit();
          return;
        }
      }
    }
    else
    {
      evPrev = tone.ev;
    }
  }
  else
  {
    // Interactive rendering
    if (isSequencePlaying)
    {
      sequenceFrameIndex = (sequenceFrameIndex + 1) % sequenceFrameCount;
      setSequenceFrame(sequenceFrameIndex);
    }
  }

  if (isSceneChanged)
  {
    if (props.exists("sunSkyLightDir"))
      props.set("sunSkyLightDir", lightDir);
    if (props.exists("distantLightDir"))
      props.set("distantLightDir", lightDir);
    device->initLights(props);
  }

  // Setup the camera
  view.roll += viewRollDelta * viewRollStep;
  Basis3f cameraBasis = Basis3f(one).rotateV(view.yaw).rotateU(view.pitch).rotateN(view.roll);
  view.pos += cameraBasis.toGlobal(viewPosDelta * viewPosStep * viewPosSpeed);
  view.fovY = clamp(view.fovY + viewFovDelta * viewFovStep, viewFovStep, degToRad(100.f));
  view.radius = max(view.radius + viewRadiusDelta * viewRadiusStep * viewPosSpeed, 0.f);

  // Update the camera if necessary
  bool isCameraUpdated = false;
  bool updateCamera = memcmp(&view, &oldView, sizeof(View)) != 0;
  if (updateCamera || printCamera)
  {
    Props camera = makeCamera();

    if (printCamera)
    {
      Vec3f lookAt = view.pos + cameraBasis.toGlobal(Vec3f(0.f, 0.f, -view.focus));
      Vec3f up = cameraBasis.toGlobal(Vec3f(0.f, 1.f, 0.f));
      Log() << "Camera: " << std::setprecision(6) << camera << " lookAt=" << lookAt << " up=" << up;
      printCamera = false;
    }

    if (updateCamera)
    {
      Props prevCamera = prevView ? makeCamera(prevView) : camera;
      Props nextCamera = (nextView && isSequencePlaying) ? makeCamera(nextView) : camera;

      device->initCamera(camera, prevCamera, nextCamera);
      oldView = view;
      if (!isSequencePlaying)
        prevView = view;
      isCameraUpdated = true;
    }
  }

  // Clear the frame if necessary
  if (isCameraUpdated || isSceneChanged /*|| maxSpp == 1*/)
  {
    device->clearFrame();
    device->initSampler(rng.get1ui());

    if (jitter)
    {
      const int jitterIndex = jitterStart + (isSequencePlaying ? sequenceFrameIndex : frameCount);
      //Log() << "Jitter sequence index: " << jitterIndex;
      device->setJitter({halton(2, jitterIndex), halton(3, jitterIndex)});
    }
    else
    {
      device->disableJitter();
    }

    spp = 0;
    prevNoise = 0.f;
    displayTimeSum = 0;
    displayTimeCount = 0;
    isSceneChanged = false;
  }

  // Render the frame
  bool isRendering = (maxSpp == 0) || (spp < maxSpp) || isCameraUpdated || (isBatchMode && isAutoExposureMode);

  if (!isRendering && (getDisplayMode() == DisplayMode::Offscreen || isBenchmarkMode))
    quit();

  if (isRendering)
  {
    // Init the stats
    frameStats.clear();
    frameStats.set("renderMs", 0.0);
    frameStats.set("fps", 0.0);

    isFrameStatsPrinted = false;
  }

  // Render the frame
  Surface surface;
  beginFrame(surface);

  if (isRendering)
    device->render(frameStats);

  // Update tone mapping if necessary
  if (memcmp(&tone, &prevTone, sizeof(Tone)) != 0)
  {
    Props toneMapperProps;
    toneMapperProps.set("type", toneMapperNames[tone.typeId]);
    toneMapperProps.set("exposure", exp2(tone.ev));
    toneMapperProps.set("burn", tone.burn);

    device->initToneMapper(toneMapperProps);
    prevTone = tone;
  }

  // Update the frame
  updateSurface(surface);

  if (isTextEnabled)
  {
    getText() << deviceInfo;
    if (isRendering)
    {
      getText() << std::endl << std::fixed << std::setprecision(1) << currentFps << " fps" << std::endl << currentMray << " Mray/s";
    }
  }

  endFrame();

  if (isRendering)
  {
    // Complete the stats
    ++frameCount;
    int samplesPerFrame = frameStats.get("spp", 1);
    spp += samplesPerFrame;

    frameStats.set("renderMs", getRenderTime() * 1000.0);
    frameStats.set("fps", 1.0 / getDisplayTime());

    const int displayTimeSkip = 4; // skip first few frames
    if (displayTimeCount >= displayTimeSkip)
      displayTimeSum += getDisplayTime();
    displayTimeCount++;

    // Record the stats
    if (spp > warmupSpp || !isBenchmarkMode)
      statsRecorder.add(frameStats);

    // Print the stats if necessary
    frameStats.set("spp", spp);
    if (statsTimer.query() >= 2.0)
    {
      statsTimer.reset();
      printFrameStats();
    }

    // Compute ETA
    if (maxSpp > 0 && displayTimeSum > 30 && displayTimeCount >= 32)
    {
      double displayTimeAvg = displayTimeSum / (displayTimeCount-displayTimeSkip);
      double framesRemain = double(maxSpp - spp) / samplesPerFrame;
      double eta = framesRemain * displayTimeAvg;
      if (eta*1.01 > (runTimeThreshold - getElapsedTime()))
      {
        LogError() << "Run time limit will be exceeded";
        quit();
      }
    }
  }
  else
  {
    printFrameStats();
  }

  // Check noise level
  if (isRendering && !(isBatchMode && isAutoExposureMode) &&
      (maxNoise > 0.f || multiresMaxNoise > 0.f) && spp >= minVarSpp && isPowerOfTwo(spp) &&
      (!isSequencePlaying || sequenceFrameIndex == 0) && presetIndex == 0)
  {
    const float noise = device->getFrameError();
    Log() << "Noise: " << noise;

    if (spp >= minSpp && noise <= maxNoise && noise <= prevNoise)
    {
      Log() << "Noise is below threshold, stopping rendering";
      maxSpp = adaptiveMaxSpp = spp;
    }

    if (spp >= multiresMinSpp && spp < adaptiveMultiresMaxSpp && noise <= multiresMaxNoise && noise <= prevNoise)
    {
      Log() << "Noise is below threshold for multires";
      adaptiveMultiresMaxSpp = spp;
    }

    prevNoise = noise;
  }

  // Save a screenshot if necessary
  if (isRendering && isOutputEnabled && !isBenchmarkMode && !(isBatchMode && isAutoExposureMode) &&
      ((isCheckpointEnabled && isPowerOfTwo(spp)) || (isBatchMode && (spp == maxSpp))))
  {
    std::stringstream sm;
    if (!outputPrefix.empty())
      sm << outputPrefix;
    if (!outputId.empty())
    {
      if (!outputPrefix.empty() && outputPrefix.back() != '/' && outputPrefix.back() != '\\')
        sm << "_";
      sm << outputId;
      if (isBatchMode && oriIsCheckpointEnabled)
      {
        const std::string dir = sm.str();
        if (!isDirectory(dir))
        {
          if (!makeDirectory(dir))
            LogError() << "Could not create directory: " << dir;
        }
        sm << "/" << outputId;
      }
    }

    const std::string baseFilename = sm.str();
    if (oriIsCheckpointEnabled)
    {
      const int sppDigits = oriMaxSpp > 0 ? toString(oriMaxSpp).size() : 6;
      sm << "_" << std::setfill('0') << std::setw(sppDigits) << spp << "spp";
    }
    sm << outputSuffix;

    const std::string filename = sm.str();
    saveScreenshot(filename, isSequencePlaying);

    if (isOutputEnabled)
      doneFilename = baseFilename + ".done";
  }

  if (autoShotThreshold > 0)
  {
    if (screenshotTimer.query() >= autoShotThreshold)
      saveAutoScreenshot();
  }

  if (getElapsedTime() >= runTimeThreshold)
  {
    LogWarn() << "Run time limit exceeded";
    quit();
  }
}

Props RenderWindow::makeCamera(View view)
{
  Basis3f basis = Basis3f(one).rotateV(view.yaw).rotateU(view.pitch).rotateN(view.roll);

  Props camera;
  if (view.radius == 0.f)
    camera.set("type", "pinhole");
  else
    camera.set("type", "thinlens");

  camera.set("position", view.pos);
  camera.set("basis", basis);
  camera.set("fov", view.fovY);
  camera.set("width", imageSize.x);
  camera.set("height", imageSize.y);
  camera.set("lensRadius", view.radius);
  camera.set("focalDistance", view.focus);
  camera.set("nearClip", nearClip);
  //camera.set("farClip", farClip);

  return camera;
}

Props RenderWindow::makeCamera()
{
  return makeCamera(view);
}

void RenderWindow::updateFrame(bool color)
{
  if (color && hdrBuffer.getSize() != imageSize)
    hdrBuffer.alloc(imageSize);

  FrameAovs frame;
  frame.hdr = hdrBuffer.getData();
  frame.hdrVar = hdrVarBuffer.getData();
  frame.hdrHalf = hdrHalfBuffer.getData();
  frame.ldr = ldrBuffer.getData();
  frame.hdrUnclamp = hdrUnclampBuffer.getData();
  frame.hdrUnclampVar = hdrUnclampVarBuffer.getData();
  frame.hdrUnclampHalf = hdrUnclampHalfBuffer.getData();
  frame.albedo = albedoBuffer.getData();
  frame.albedoVar = albedoVarBuffer.getData();
  frame.albedoHalf = albedoHalfBuffer.getData();
  frame.albedo1 = albedo1Buffer.getData();
  frame.diffuseAlbedo = diffuseAlbedoBuffer.getData();
  frame.diffuseAlbedo1 = diffuseAlbedo1Buffer.getData();
  frame.specularAlbedo = specularAlbedoBuffer.getData();
  frame.specularAlbedo1 = specularAlbedo1Buffer.getData();
  frame.normal = normalBuffer.getData();
  frame.normalVar = normalVarBuffer.getData();
  frame.normalHalf = normalHalfBuffer.getData();
  frame.normal1 = normal1Buffer.getData();
  frame.depth = depthBuffer.getData();
  frame.depthVar = depthVarBuffer.getData();
  frame.depthHalf = depthHalfBuffer.getData();
  // frame.cameraNormal = cameraNormalBuffer.getData();
  frame.hwDepth = hwDepthBuffer.getData();
  frame.roughness = roughnessBuffer.getData();
  frame.roughness1 = roughness1Buffer.getData();
  frame.alpha = alphaBuffer.getData();
  for (int i = 0; i < 4; ++i)
    frame.sh[i] = shBuffer[i].getData();
  frame.motionBack = motionBackBuffer.getData();
  frame.motionFore = motionForeBuffer.getData();
  device->blitFrame(frame);

#if 0
  for (int i = 0; i < imageSize.x*imageSize.y; ++i)
  {
    if (colorBuffer.getData())
      if (!all(isfinite(colorBuffer.getData()[i])))
        LogWarn() << "NaN/Inf in color buffer";

    if (srgbBuffer.getData())
      if (!all(isfinite(srgbBuffer.getData()[i])))
        LogWarn() << "NaN/Inf in srgb buffer";

    if (albedoBuffer.getData())
      if (!all(isfinite(albedoBuffer.getData()[i])))
        LogWarn() << "NaN/Inf in albedo buffer";

    if (normalBuffer.getData())
      if (!all(isfinite(normalBuffer.getData()[i])))
        LogWarn() << "NaN/Inf in normal buffer";
  }
#endif
}

void RenderWindow::updateSurface(Surface& surface)
{
  if (aovNames[aovId] != "color" || denoiser || isAutoExposureMode)
    updateFrame(isAutoExposureMode);

  if (isAutoExposureMode)
  {
    autoExpose();
    displayTimeSum = 0;
    displayTimeCount = 0;
  }

  if (!surface.data)
    return;

  if (aovNames[aovId] == "color")
  {
    if (!denoiser || (spp < denoiserSpp && (maxSpp == 0 || spp < maxSpp)))
    {
      device->blitFrame(surface);
    }
    else
    {
      denoiser->execute();

      if (denoiserMode == DenoiserMode::Ldr)
        device->blitFrameLdr(surface, denoiseBuffer.getData());
      else
        device->blitFrameHdr(surface, denoiseBuffer.getData());
    }
  }
  else if (aovNames[aovId] == "albedo")
    device->blitFrameLdr(surface, albedoBuffer.getData());
  else if (aovNames[aovId] == "diffuseAlbedo1")
    device->blitFrameLdr(surface, diffuseAlbedo1Buffer.getData());
  else if (aovNames[aovId] == "specularAlbedo1")
    device->blitFrameLdr(surface, specularAlbedo1Buffer.getData());
  else if (aovNames[aovId] == "normal")
    device->blitFrameLdr(surface, normalBuffer.getData(), 1.f, 0.5f);
  else if (aovNames[aovId] == "hwDepth")
    device->blitFrameLdr(surface, hwDepthBuffer.getData());
  else if (aovNames[aovId] == "roughness")
    device->blitFrameLdr(surface, roughnessBuffer.getData());
  else if (aovNames[aovId] == "alpha")
    device->blitFrameLdr(surface, alphaBuffer.getData());
  else if (aovNames[aovId] == "sh0")
    device->blitFrameHdr(surface, shBuffer[0].getData());
  else if (aovNames[aovId] == "sh1x")
    device->blitFrameLdr(surface, shBuffer[1].getData(), 1.f, 0.5f);
  else if (aovNames[aovId] == "sh1y")
    device->blitFrameLdr(surface, shBuffer[2].getData(), 1.f, 0.5f);
  else if (aovNames[aovId] == "sh1z")
    device->blitFrameLdr(surface, shBuffer[3].getData(), 1.f, 0.5f);

  if (isPoiMode)
    renderPois(surface);
}

std::pair<int, int> RenderWindow::renderPois(Surface& surface, int bucketCountX, int bucketCountY)
{
  const Vec3f color = Vec3f(1.f, 0.f, 0.f);
  const Vec3f colorT = Vec3f(1.f, 0.5f, 0.f);
  const int size = 2;

  const int pixel = encodeBgr8(color);
  const int pixelT = encodeBgr8(colorT);

  int visiblePoiCount = 0;
  std::vector<int> visiblePoiBuckets(bucketCountX*bucketCountY, 0);

  // Get the camera
  const Mat4f worldToRaster = device->getWorldToRaster();

  // Iterate over the POIs
  for (int i = 0; i < poiSet.getSize(); ++i)
  {
    // Transform the POI to raster space
    const Poi& poi = poiSet[i];
    const Vec4f p4D = xfmPoint(worldToRaster, poi.p);
    const float pDepth = p4D.w;
    const Vec3f p = projPoint(p4D);

    const int xi = int(p.x);
    const int yi = int(p.y);

    // Viewport clipping, depth test, backface culling
    if (xi >= 0 && xi < imageSize.x && yi >= 0 && yi < imageSize.y &&
      pDepth > 0.f && dot(poi.N, view.pos - poi.p) >= 0.f)
    {
      // Occlusion test (ignore transparent surfaces)
      const float depth = pDepth*0.99f;
      Array<Props> hits;
      device->queryPixel(xi, yi, hits);

      if (hits.isEmpty() || hits.getBack().get("depth", float(posInf)) > depth)
      {
        // Render the POI
        if (surface.data)
        {
          // Is the point behind a transparent surface?
          const bool T = (hits.getSize() > 1) && hits[0].get("depth", float(posInf)) <= depth;

          for (int u = -size; u <= size; ++u)
          {
            for (int v = -size; v <= size; ++v)
            {
              const int xi2 = clamp(xi + u, 0, imageSize.x-1);
              const int yi2 = clamp(yi + v, 0, imageSize.y-1);
              surface.getRow(yi2)[xi2] = T ? pixelT : pixel;
            }
          }
        }

        const int bx = clamp(int(p.x / imageSize.x * bucketCountX), 0, bucketCountX-1);
        const int by = clamp(int(p.y / imageSize.y * bucketCountY), 0, bucketCountY-1);
        visiblePoiBuckets[bx + by * bucketCountX]++;
        visiblePoiCount++;
      }
    }
  }

  int visiblePoiBucketCount = 0;
  for (int i = 0; i < bucketCountX*bucketCountY; ++i)
    visiblePoiBucketCount += min(visiblePoiBuckets[i], 1);
  return std::make_pair(visiblePoiCount, visiblePoiBucketCount);
}

std::pair<int, int> RenderWindow::getVisiblePoiCount(int bucketCountX, int bucketCountY)
{
  // Dummy surface (don't render anything)
  Surface surface;
  surface.data = nullptr;
  surface.width = 0;
  surface.height = 0;
  surface.pitch = 0;

  return renderPois(surface, bucketCountX, bucketCountY);
}

void RenderWindow::autoExpose()
{
  const float minLogLum  = -13.f;
  const float maxLogLum  = 4.f;
  const float minPercent = 80.f;
  const float maxPercent = 98.5f;

  const int blockSize = 8;
  const int histogramSize = 1024;

  // Compute the histogram
  float histogram[histogramSize];
  for (int i = 0; i < histogramSize; ++i)
    histogram[i] = 0.f;

  for (int Y = 0; Y < imageSize.y; Y += blockSize)
  {
    for (int X = 0; X < imageSize.x; X += blockSize)
    {
      // Compute the average brightness in the current block
      float lumSum = 0.f;
      float weightSum = 0.f;
      for (int y = Y; y < Y + blockSize && y < imageSize.y; ++y)
      {
        for (int x = X; x < X + blockSize && x < imageSize.x; ++x)
        {
          lumSum += luminance(hdrBuffer[y][x]);
          weightSum += 1.f;
        }
      }
      const float lum = lumSum / weightSum;

      const float logLum = clamp(log2f(lum), minLogLum, maxLogLum);
      const int bucket = clamp(int((logLum - minLogLum) / (maxLogLum - minLogLum) * histogramSize), 0, histogramSize-1);
      histogram[bucket] += 1.f;
    }
  }

  /*
  // Save the histogram
  FILE* f = fopen("histogram.csv", "wt");
  for (int i = 0; i < histogramSize; ++i)
  {
    fprintf(f, "%d,%f\n", i, histogram[i]);
    Log() << i << ": " << histogram[i];
  }
  fclose(f);
  */

  // Compute the average luminance
  float histogramSum = 0.f;
  for (int i = 0; i < histogramSize; ++i)
    histogramSum += histogram[i];

  const float minHistogramSum = histogramSum * minPercent / 100.f;
  const float maxHistogramSum = histogramSum * maxPercent / 100.f;
  histogramSum = 0.f;

  float lumSum = 0.f;
  float weightSum = 0.f;

  for (int i = 0; i < histogramSize; ++i)
  {
    const float a = max(histogramSum, minHistogramSum);
    histogramSum += histogram[i];
    const float b = min(histogramSum, maxHistogramSum);
    const float weight = max(b - a, 0.f);

    const float bucketLum = exp2(minLogLum + ((float(i) + 0.5f) / histogramSize) * (maxLogLum - minLogLum));
    lumSum += bucketLum * weight;
    weightSum += weight;
  }

  const float avgLum = lumSum / weightSum;
  const float exposureScale = rcp(max(avgLum, 0.0001f));
  tone.ev = clamp(log2f(exposureScale), evMin, evMax);
  Log() << "Auto EV: " << tone.ev;
}

void RenderWindow::resetMaxRadiance()
{
  props.set("maxRadiance", maxRadiance);
  device->updateRenderer(props);
}

void RenderWindow::setAutoMaxRadiance()
{
  props.set("maxRadiance", min(autoMaxRadiance / pow(2.f, tone.ev), maxRadiance));
  device->updateRenderer(props);
}

void RenderWindow::printFrameStats()
{
  if (!isFrameStatsPrinted)
    Log() << frameStats;

  isFrameStatsPrinted = true;

  currentFps = frameStats.get("fps", 0.0);
  currentMray = frameStats.get("mray", 0.0);
}

void RenderWindow::saveAvgStats(const Props& stats, const std::string& filename)
{
  Log() << "Saving stats: " << filename;
  FILE* file = fopen(filename.c_str(), "wt");
  for (auto& i : stats)
    fprintf(file, "stats_%s=%s\n", i.first.c_str(), i.second.get<std::string>().c_str());
  fclose(file);
}

void RenderWindow::saveFullStats(const std::string& filename)
{
  Log() << "Saving stats: " << filename;
  statsRecorder.saveCsv(filename);
}

void RenderWindow::saveScreenshot(const std::string& basename, bool withSequenceFrameIndex)
{
  std::stringstream suffixStream;
  if (withSequenceFrameIndex)
  {
    const int frameDigits = toString(sequenceFrameCount-1).size();
    suffixStream << "." << std::setfill('0') << std::setw(frameDigits) << sequenceFrameIndex;
  }

  const std::string suffix = suffixStream.str();

  printFrameStats();

  // LDR
  Image4uc image(imageSize);
  Surface surface;
  surface.width = imageSize.x;
  surface.height = imageSize.y;
  surface.pitch = imageSize.x * 4;
  surface.data = image.getData();
  updateSurface(surface);

  if (isBenchmarkMode)
    saveImageSafe(basename + suffix + ".ppm", image);
  else if (!isBatchMode)
    saveImageSafe(basename + suffix, image);

#ifdef PRT_IMAGE_SUPPORT
  // HDR + AOVs
  if (!isBenchmarkMode)
  {
    updateFrame();

    // Scale HDR values by the exposure value if not rendering training images
    const float hdrScale = isBatchMode ? 1.f : exp2(tone.ev);
    const float hdrVarScale = sqr(hdrScale);
    Log() << "HDR scale: " << hdrScale;

    saveImageSafe(basename + ".hdr" + suffix, hdrBuffer, hdrScale);
    if (hdrVarBuffer && spp >= minVarSpp)
      saveImageSafe(basename + ".hdrvar" + suffix, hdrVarBuffer, hdrVarScale);
    if (hdrHalfBuffer && spp >= minVarSpp)
      saveImageSafe(basename + ".hdrh" + suffix, hdrHalfBuffer, hdrScale);

    if (hdrUnclampBuffer)
      saveImageSafe(basename + ".hdruc" + suffix, hdrUnclampBuffer, hdrScale);
    if (hdrUnclampVarBuffer && spp >= minVarSpp)
      saveImageSafe(basename + ".hdrucvar" + suffix, hdrUnclampVarBuffer, hdrVarScale);
    if (hdrUnclampHalfBuffer && spp >= minVarSpp)
      saveImageSafe(basename + ".hdruch" + suffix, hdrUnclampHalfBuffer, hdrScale);

    if (ldrBuffer)
      saveImageSafe(basename + ".ldr" + suffix, ldrBuffer);

    if (albedoBuffer)
      saveImageSafe(basename + ".alb" + suffix, albedoBuffer);
    if (albedoVarBuffer && spp >= minVarSpp)
      saveImageSafe(basename + ".albvar" + suffix, albedoVarBuffer);
    if (albedoHalfBuffer && spp >= minVarSpp)
      saveImageSafe(basename + ".albh" + suffix, albedoHalfBuffer);
    if (albedo1Buffer)
      saveImageSafe(basename + (albedoBuffer ? ".alb1" : ".alb") + suffix, albedo1Buffer);

    if (diffuseAlbedoBuffer)
      saveImageSafe(basename + ".dalb" + suffix, diffuseAlbedoBuffer);
    if (diffuseAlbedo1Buffer)
      saveImageSafe(basename + (diffuseAlbedoBuffer ? ".dalb1" : ".dalb") + suffix, diffuseAlbedo1Buffer);

    if (specularAlbedoBuffer)
      saveImageSafe(basename + ".salb" + suffix, specularAlbedoBuffer);
    if (specularAlbedo1Buffer)
      saveImageSafe(basename + (specularAlbedoBuffer ? ".salb1" : ".salb") + suffix, specularAlbedo1Buffer);

    if (normalBuffer)
      saveImageSafe(basename + ".nrm" + suffix, normalBuffer);
    if (normalVarBuffer && spp >= minVarSpp)
      saveImageSafe(basename + ".nrmvar" + suffix, normalVarBuffer);
    if (normalHalfBuffer && spp >= minVarSpp)
      saveImageSafe(basename + ".nrmh" + suffix, normalHalfBuffer);
    if (normal1Buffer)
      saveImageSafe(basename + (normalBuffer ? ".nrm1" : ".nrm") + suffix, normal1Buffer);

    if (depthBuffer)
      saveImageSafe(basename + ".z" + suffix, depthBuffer, 1.f, 32);
    if (depthVarBuffer && spp >= minVarSpp)
      saveImageSafe(basename + ".zvar" + suffix, depthVarBuffer, 1.f, 32);
    if (depthHalfBuffer && spp >= minVarSpp)
      saveImageSafe(basename + ".zh" + suffix, depthHalfBuffer, 1.f, 32);
    // if (cameraNormalBuffer)
    //     saveImageSafe(basename + ".cnrm" + suffix, cameraNormalBuffer);

    if (hwDepthBuffer)
      saveImageSafe(basename + ".hwz" + suffix, hwDepthBuffer, 1.f, 32);

    if (roughnessBuffer)
      saveImageSafe(basename + ".rgh" + suffix, roughnessBuffer);
    if (roughness1Buffer)
      saveImageSafe(basename + (roughnessBuffer ? ".rgh1" : ".rgh") + suffix, roughness1Buffer);

    if (alphaBuffer)
      saveImageSafe(basename + ".a" + suffix, alphaBuffer);

    if (shBuffer[0])
    {
      saveImageSafe(basename + ".sh0"  + suffix, shBuffer[0]);
      saveImageSafe(basename + ".sh1x" + suffix, shBuffer[1]);
      saveImageSafe(basename + ".sh1y" + suffix, shBuffer[2]);
      saveImageSafe(basename + ".sh1z" + suffix, shBuffer[3]);
    }

    if (motionBackBuffer)
      saveImageSafe(basename + ".mvb" + suffix, motionBackBuffer);
    if (motionForeBuffer)
      saveImageSafe(basename + ".mvf" + suffix, motionForeBuffer);

    // Save image metadata
    json jsonImage;

    if (device->hasJitter())
    {
      jsonImage["jitter"] = toJSON(device->getJitter());
      jsonImage["jitterOffset"] = toJSON(0.5f - device->getJitter());
    }
    else
    {
      jsonImage["pixelFilter"] = device->getFilterType();
      jsonImage["pixelFilterWidth"] = device->getFilterWidth();
      if (sharpness >= 0.f)
        jsonImage["sharpness"] = sharpness;
    }

    jsonImage["sampler"] = device->getSamplerType();

    jsonImage["worldToViewMatrix"] = toJSON(device->getWorldToViewD3D());
    jsonImage["viewToClipMatrix"]  = toJSON(device->getViewToClipD3D());

    json jsonCamera;
    Props camera = makeCamera();
    Basis3f cameraBasis = camera.get<Basis3f>("basis");
    jsonCamera["position"] = toJSON(camera.get<Vec3f>("position"));
    jsonCamera["direction"] = toJSON(-cameraBasis.N);
    jsonCamera["up"] = toJSON(cameraBasis.V);
    jsonCamera["right"] = toJSON(cameraBasis.U);
    jsonCamera["fovY"] = camera.get<float>("fov");
    jsonCamera["lensRadius"] = camera.get<float>("lensRadius");
    jsonCamera["focalDistance"] = camera.get<float>("focalDistance");
    jsonCamera["nearClip"] = camera.get<float>("nearClip");

    jsonImage["camera"] = jsonCamera;
    jsonImage["sceneScale"] = sceneScale;
    jsonImage["exposure"] = exp2(tone.ev) / hdrScale;
    jsonImage["maxPathDepth"] = props.get("maxDepth", maxDepth);

    if (!jsonImage.empty())
      saveJsonSafe(basename + suffix + ".json", jsonImage);
  }
#endif

  displayTimeSum = 0;
  displayTimeCount = 0;
  screenshotTimer.reset();
}

void RenderWindow::saveAutoScreenshot()
{
  int index = 0;

  // Try to open the screenshot index file
  FILE* indexFile = fopen("screenshot_index", "rt");
  if (indexFile != 0)
  {
    if (fscanf(indexFile, "%d", &index) != 1)
      LogWarn() << "Could not read screenshot index";
    fclose(indexFile);
  }

  // Save the screenshot with the next index
  std::stringstream filenameBase;
  filenameBase << "screenshot_" << std::setfill('0') << std::setw(4) << index;
  saveScreenshot(filenameBase.str());

  // Save the index file
  ++index;
  indexFile = fopen("screenshot_index", "wt");
  fprintf(indexFile, "%d", index);
  fclose(indexFile);

  displayTimeSum = 0;
  displayTimeCount = 0;
  screenshotTimer.reset();
}

void RenderWindow::resetView()
{
  if (viewId >= 0)
    Log() << "Reset view: " << viewId;

  if (viewId > 0)
  {
    // Reset from view 0
    view = viewSet.views[0];
    return;
  }

  // Setup the camera
  Box3f sceneBox = device->getSceneBounds();
  Vec3f sceneCenter = sceneBox.getCenter();
  view.pos.x = sceneCenter.x;
  view.pos.y = sceneCenter.y;
  view.pos.z = 1.7f * reduceMax(sceneBox.getSize()) + sceneCenter.z;

  view.yaw   = 0.f;
  view.pitch = 0.f;
  view.roll  = 0.f;

  view.fovY = degToRad(45.f);

  view.radius = 0.f;
  view.focus = length(sceneCenter - view.pos);
}

void RenderWindow::setActiveView(int id)
{
  viewId = id;

  sequenceKeyframes[0].view = viewSet.views[id];
  sequenceKeyframes[1].view = viewSet.views2[id];

  if (!std::isnan(viewSet.lightDirs[id].x))
    sequenceKeyframes[0].lightDir = viewSet.lightDirs[id];
  if (!std::isnan(viewSet.lightDirs2[id].x))
    sequenceKeyframes[1].lightDir = viewSet.lightDirs2[id];

  Log() << "Active view: " << viewId;
  setSequenceFrame(sequenceFrameIndex);
}

void RenderWindow::saveView()
{
  if (sequenceFrameIndex == 0 || sequenceFrameIndex == sequenceFrameCount-1)
  {
    if (sequenceFrameIndex == 0)
    {
      sequenceKeyframes[0].view = view;
      viewSet.views[viewId] = view;

      sequenceKeyframes[0].lightDir = lightDir;
      viewSet.lightDirs[viewId] = lightDir;
    }
    else
    {
      sequenceKeyframes[1].view = view;
      viewSet.views2[viewId] = view;

      sequenceKeyframes[1].lightDir = lightDir;
      viewSet.lightDirs2[viewId] = lightDir;
    }

    saveViewSet(viewFilename, viewSet);

    Log() << "Saved view: " << viewId;
  }
  else
  {
    LogWarn() << "Cannot save view in the middle of a sequence";
  }
}

bool RenderWindow::isViewSalient(float minCoverage, float minPoiCoverage)
{
  // Compute coverage
  const int bucketSize = 8;
  int hitCount = 0;
  int testCount = 0;
  for (int y = bucketSize/2; y < imageSize.y; y += bucketSize)
  {
    for (int x = bucketSize/2; x < imageSize.x; x += bucketSize)
    {
      Props hit = device->queryPixel(x, y);
      if (!hit.isEmpty())
        hitCount++;
      testCount++;
    }
  }

  if (testCount == 0)
    return false;

  float coverage = (float)hitCount / testCount;
  //Log() << "Coverage: " << coverage;

  // Reject view if coverage is below threshold
  if (coverage < minCoverage)
    return false;

  // Reject view if too few POIs are visible
  const int poiBucketCountXY = 3;
  const int poiBucketCount = poiBucketCountXY * poiBucketCountXY;
  int visiblePoiCount, visiblePoiBucketCount;
  std::tie(visiblePoiCount, visiblePoiBucketCount) = getVisiblePoiCount(poiBucketCountXY, poiBucketCountXY);
  const float poiCoverage = float(visiblePoiBucketCount) / float(poiBucketCount);
  //Log() << "POIs visible: " << poiCoverage << ", " << visiblePoiCount << " points, " << visiblePoiBucketCount << " buckets";
  if ((poiSet.getSize() > 50 && poiCoverage < minPoiCoverage) || visiblePoiCount == 0)
    return false; // reject

  return true;
}

void RenderWindow::mutate()
{
  sequenceFrameIndex = 0;

  if (!isMutateViewEnabled && !isMutateLightEnabled && !isMutateFilterEnabled &&
      !isMutateSamplerEnabled && !isMutateRenderEnabled)
    return;

  const Box3f bounds = device->getSceneBounds();
  const Vec3f center = bounds.getCenter();
  const float radius = length(bounds.getSize()/2.f);

  // Generate ID
  static int mutationIndex = 0;
  const uint64_t id = mutationIndex == 0 ? seed : rng.get1ull();
  mutationIndex++;
  char idString[17];
  sprintf(idString, "%016lx", id);
  outputId = idString;
  Log() << "Mutation: " << outputId;

  if (!poiSet.isEmpty())
  {
    const Vec3f oldPos = view.pos;
    float eps = 0.f;

    Poi poi = zero;
    Vec3f poiDir = zero;
    float poiDist = 0.f;
    bool poiVisible = false;

    const float minPoiDist = 4.f * nearClip;
    const float smallStep = 0.1f;
    const float largeStep = 0.99f;
    const float largeStepProb = 0.1f;
    const int maxStepCount = 20000;
    const int maxSoftRetryCount = 3;
    const int maxRetryCount = 20;
    const float minWalkDist = 0.01f;
    const float acceptProb = 0.5f;

    const float maxViewAngle = degToRad(45.f);
    const float minCosViewAngle = cos(maxViewAngle);

    const int maxFocusTestCount = 1000;
    const int maxLightTestCount = 1000;

    int stepCount = INT_MAX;
    int retryCount = -1;

    Ray ray;
    Props hit;

    // Mutate the view
    if (isMutateViewEnabled)
    {
      Log() << "Mutating view";

      // Initialize the keyframes
      if (sequenceFrameCount > 1)
      {
        sequenceKeyframes[0].view = view;
        sequenceKeyframes[1].view = view;
      }

      // Compute the bounding box of the POIs
      Box3f poiBounds = empty;
      for (int i = 0; i < poiSet.getSize(); ++i)
        poiBounds.grow(poiSet[i].p);

      float moveDist = 0.f;
      Vec3f moveDir  = zero; // in camera space
      float rotYaw   = 0.f;
      float rotPitch = 0.f;
      float rotRoll  = 0.f;

      for (; ;)
      {
        // Random walk, find a visible POI
        do
        {
          if (stepCount > maxStepCount)
          {
            retryCount++;
            if (retryCount > maxRetryCount)
            {
              LogWarn() << "View mutation failed";
              break;
            }
            else if (retryCount > 0)
            {
              //Log() << "View mutation failed, retrying " << retryCount << "/" << maxRetryCount;
            }

            // Retry from the beginning
            view.pos = oldPos;
            stepCount = 0;

            // Pick a random POI
            int poiId = min(int(rng.get1f() * poiSet.getSize()), poiSet.getSize()-1);
            poi = poiSet[poiId];

            if (retryCount == 0 || retryCount > maxSoftRetryCount)
            {
              // Pick a random camera FOV
              view.fovY = degToRad((rng.get1f() < 0.3f) ? rng.get1f(20.f, 45.f) : rng.get1f(45.f, 70.f));
              // Adjust FOV to have constant angular resolution
              view.fovY = 2.f * atan(tan(view.fovY / 2.f) * (float(imageSize.y) / 576.f));
              //Log() << "FOV: " << radToDeg(view.fovY) << " degrees";

              // Pick a random camera movement and rotation
              if (sequenceFrameCount > 1 && rng.get1f() < 0.85f)
              {
                moveDist = abs(rng.getTruncNorm1f(0.f, 0.03f, 0.f, 0.3f)) * sceneScale * sequenceFrameCount;

                const float moveType = rng.get1f();
                if (moveType < 0.1f)
                  moveDir = zero;
                else if (moveType < 0.2f)
                  moveDir = Vec3f(0.f, 0.f, 1.f);
                else if (moveType < 0.25f)
                  moveDir = Vec3f(0.f, 0.f, -1.f);
                else
                  moveDir = uniformSampleSphere(rng.get2f());

                if (rng.get1f() < 0.8f)
                {
                  const float rotScale = degToRad(1.f) * (view.fovY / degToRad(45.0f)) * sequenceFrameCount;
                  rotYaw   = (rng.get1f() < 0.7f) ? rng.getTruncNorm1f(0.f, 0.15f, -5.f, 5.f) * rotScale : 0.f;
                  rotPitch = (rng.get1f() < 0.7f) ? rng.getTruncNorm1f(0.f, 0.15f, -5.f, 5.f) * rotScale : 0.f;
                  rotRoll  = (rng.get1f() < 0.3f) ? rng.getTruncNorm1f(0.f, 0.10f, -5.f, 5.f) * rotScale : 0.f;
                }
              }
              else
              {
                moveDist = 0.f;
                moveDir  = zero;
                rotYaw   = 0.f;
                rotPitch = 0.f;
                rotRoll  = 0.f;
              }
            }
          }

          do
          {
            // Shoot a ray in a random direction
            ray.init(view.pos, uniformSampleSphere(rng.get2f()));
            hit = device->queryRay(ray);
            eps = hit.get("eps", eps);

            // Move forward in the selected direction
            float step = (rng.get1f() < largeStepProb) ? largeStep : smallStep;
            step *= hit.get("dist", radius/8.f*rng.get1f());
            Vec3f newPos = ray.org + ray.dir * step;
            if (length(newPos-center) < radius*2.f || length(newPos-center) < length(view.pos-center))
              view.pos = newPos;

            stepCount++;
            if (stepCount > maxStepCount)
              break;
          } while (length(oldPos - view.pos) < (minWalkDist * radius) || view.pos.y < poiBounds.low.y);

          // Check whether the POI is visible at an acceptable angle
          if (rng.get1f() < acceptProb)
          {
            poiDir = poi.p - view.pos;
            poiDist = length(poiDir);
            poiDir = normalize(poiDir);
            if (poiDist > minPoiDist && (poi.N == Vec3f(zero) || dot(poi.N, -poiDir) > minCosViewAngle))
            {
              ray.init(view.pos, poiDir, eps, poiDist-eps);
              poiVisible = !device->queryOcclusionRay(ray);
            }
          }
        } while (!poiVisible);

        //Log() << "Mutate step count = " << stepCount;

        // Pick a random position on the ray segment
        float maxPoiDist = min(poiDist, maxViewDist, radius * 0.5f); // make sure we're not too far
        float stepSize = lerp(minPoiDist, maxPoiDist, rng.get1f());
        view.pos = ray.org + ray.dir * (poiDist - stepSize);

        // Point the camera roughly in the direction of the POI
        float viewDirPdf;
        const Vec3f viewDir = makeFrame(poiDir) * uniformSampleCone(viewDirPdf, rng.get2f(), view.fovY / 8.f);
        view.yaw   = atan2(viewDir.x, -viewDir.z);
        view.pitch = acosSafe(viewDir.y) - float(pi)/2.f;
        view.roll  = rng.get1f() * (2.f*float(pi));

        // Initialize the camera for ray queries
        device->initCamera(makeCamera());

        // Check whether the view is salient
        const float minCoverage      = 0.9f;
        const float minPoiCoverage   = 0.75f;
        const float minCoverage1     = (sequenceFrameCount > 1) ? 0.8f  : minCoverage;
        const float minPoiCoverage1  = (sequenceFrameCount > 1) ? 0.51f : minPoiCoverage;
        if (retryCount < maxRetryCount && !isViewSalient(minCoverage1, minPoiCoverage1))
          continue; // reject view

        // Pick a random aperture radius
        Vec2f focusPoint(0.5f);
        const float dofProb = 0.25f;
        if ((rng.get1f() > dofProb) || isIrradianceMode)
        {
          // Pinhole camera, no DOF
          view.radius = 0.f;
          view.focus  = 0.f;
        }
        else
        {
          // Pick a random point on the image to focus on
          int focusTestCount = 0;
          for (; focusTestCount < maxFocusTestCount; ++focusTestCount)
          {
            focusPoint = rng.getTruncNorm2f(0.5f, 0.5f/3.f, 0.f, 1.f);

            Array<Props> hits;
            device->queryPixel(focusPoint, hits);
            if (!hits.isEmpty())
            {
              // Pick a random depth value from the hits
              view.focus = hits[rng.get1i(0, hits.getSize()-1)].get("depth", 0.f);
              break;
            }
          }

          if (focusTestCount == maxFocusTestCount)
            continue; // focus failed, reject view

          view.radius = view.focus * 0.01f * rng.get1f();
        }

        // Generate another view for the last keyframe of the sequence
        if (sequenceFrameCount > 1)
        {
          // Save the first keyframe
          View view1 = view;

          // Move the camera in a random direction
          ray.init(view.pos, makeFrame(viewDir) * moveDir);
          hit = device->queryRay(ray);
          if (hit.get("dist", moveDist) < moveDist)
            continue; // collision, reject view
          view.pos += ray.dir * moveDist;

          // Add random rotation
          view.yaw   += rotYaw;
          view.pitch += rotPitch;
          view.roll  += rotRoll;

          // Initialize the camera for ray queries
          device->initCamera(makeCamera());

          // Check whether the view is salient
          if (retryCount < maxRetryCount && !isViewSalient(minCoverage, minPoiCoverage))
            continue; // reject view

          if (view.radius > 0.f)
          {
            // Use the current focus point or pick a new one
            int focusTestCount = 0;
            for (; focusTestCount < maxFocusTestCount; ++focusTestCount)
            {
              if (focusTestCount > 0 || rng.get1f() < 0.5f)
                focusPoint = rng.getTruncNorm2f(0.5f, 0.5f/3.f, 0.f, 1.f);

              Array<Props> hits;
              device->queryPixel(focusPoint, hits);
              if (!hits.isEmpty())
              {
                // Pick a random depth value from the hits
                view.focus = hits[rng.get1i(0, hits.getSize()-1)].get("depth", 0.f);
                break;
              }
            }

            if (focusTestCount == maxFocusTestCount)
              continue; // focus failed, reject view

            if (view.focus < view1.focus)
            {
              // Decrease the aperture radius to avoid too much blur
              view1.radius *= view.focus / view1.focus;
              view.radius   = view1.radius;
            }
          }

          // Save the keyframes
          sequenceKeyframes[0].view = view1;
          sequenceKeyframes[1].view = view;

          // Switch back to the first keyframe
          view = view1;
        }

        // Done
        break;
      }
    }

    // Mutate the lights
    if (isMutateLightEnabled)
    {
      Log() << "Mutating lights";

      // Initialize the keyframes
      if (sequenceFrameCount > 1)
      {
        sequenceKeyframes[0].lightDir = lightDir;
        sequenceKeyframes[1].lightDir = lightDir;
      }

      bool directLight = rng.get1f() < 0.5f; // should we have direct lighting in the image?

      bool lightVisible = false;
      int pointTestCount = 0;

      // Initialize the camera for ray queries
      device->initCamera(makeCamera());

      do
      {
        // Pick a random POI (may not be visible)
        int poiId = min(int(rng.get1f() * poiSet.getSize()), poiSet.getSize()-1);
        Vec3f p = poiSet[poiId].p;

        if (directLight && (pointTestCount < maxLightTestCount/2))
        {
          // Try to pick a random point on the image instead
          int imgTestCount = 0;
          do
          {
            Vec2f pImg = rng.get2f();
            hit = device->queryPixel(pImg);
            imgTestCount++;
          } while (hit.isEmpty() && (imgTestCount < maxLightTestCount));
          p = hit.get("p", p);
        }

        int dirTestCount = 0;
        do
        {
          // Pick a random light direction
          lightDir = cosineSampleHemisphere(rng.get2f());
          swap(lightDir.y, lightDir.z);

          // Check whether the light directly illuminates the selected point
          ray.init(p, lightDir, reduceMax(abs(p)) * 1e-5f);
          lightVisible = !device->queryOcclusionRay(ray);
          dirTestCount++;
        } while (!lightVisible && (dirTestCount < maxLightTestCount));

        pointTestCount++;
      } while (!lightVisible && (pointTestCount < maxLightTestCount));

      if (sequenceFrameCount > 1)
      {
        // Generate another light direction for the last keyframe of the sequence
        Vec3f lightDir2 = lightDir;

        if (rng.get1f() < 0.7f)
        {
          const float theta = degToRad(1.f) * rng.getTruncNorm1f(0.f, 0.15f, -10.f, 10.f) * sequenceFrameCount;
          const float phi   = rng.get1f() * (2.f*float(pi));

          lightDir2 = makeFrame(lightDir) * Vec3f(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta));

          if (rng.get1f() < 0.5f)
            std::swap(lightDir, lightDir2);
        }

        // Save the keyframes
        sequenceKeyframes[0].lightDir = lightDir;
        sequenceKeyframes[1].lightDir = lightDir2;
      }

      props.set("sunSkyLightAlbedo", lerp(0.f, 0.5f, rng.get1f()));
      props.set("sunSkyLightTurbidity", lerp(1.f, 8.f, rng.get1f()));

      if (isMutateLightColorEnabled)
      {
        float radiusScale = pow(2.f, rng.getTruncNorm1f(0.f, 2.5f, -5.f, 5.f));
        props.set("sunSkyLightRadiusScale", radiusScale);

        float sunSaturation = 0.f;
        float skySaturation = 0.f;
        float sunHue = 0.f;
        float skyHue = 0.f;

        if (rng.get1f() < 0.8f)
        {
          // Colored light
          if (rng.get1f() < 0.5f)
          {
            // "Artificial" light
            sunSaturation = rng.get1f();
            skySaturation = rng.get1f();
          }
          else
          {
            // "Natural" light
            // Don't change the saturation
            sunSaturation = -1.f;
            skySaturation = -1.f;
          }

          // Change the hue
          sunHue = rng.get1f();
          skyHue = rng.get1f();
        }

        props.set("sunSkyLightSunSaturation", sunSaturation);
        props.set("sunSkyLightSkySaturation", skySaturation);
        props.set("sunSkyLightSunHue", sunHue);
        props.set("sunSkyLightSkyHue", skyHue);

        float sunScale = (rng.get1f() < 0.5f) ? rng.get1f(0.f, 2.f) : 1.f;
        float skyScale = rng.get1f(0.f, 2.f);
        props.set("sunSkyLightSunScale", sunScale);
        props.set("sunSkyLightSkyScale", skyScale);
      }

      isSceneChanged = true;
    }
  }

  // Mutate the materials
  if (isMutateMtlEnabled || (isMutateLightEnabled && isMutateLightColorEnabled))
  {
    Log() << "Mutating materials";
  #if !defined(PRT_MUTATION_SUPPORT)
    LogError() << "Material mutation support not enabled";
  #endif
    device->mutateMaterials(rng, isMutateMtlEnabled, isMutateLightEnabled && isMutateLightColorEnabled);
    isSceneChanged = true;
  }

  // Mutate the filter
  if (isMutateFilterEnabled)
  {
    Log() << "Mutating filter";

    if (rng.get1f() <= 0.9f)
    {
      // Blackman-Harris 3.0-3.6
      // 3.0: (Blender Cycles, EEVEE), ~ Tent 2.0 (Chaos Corona)
      // 3.3: ~ Truncated Gaussian 2.0 (PRMan, Arnold) [Burley, 2007, "Filtering in PRMan"]
      // 3.5: ~ Gaussian 3.0 (PBRT)
      const float minWidth = 3.0f;
      const float maxWidth = 3.6f;
      const float alphaSteps = 6;
      float alpha = float(rng.get1i(0, alphaSteps)) / float(alphaSteps);
      float width = lerp(minWidth, maxWidth, alpha);

      props.set("filter", "blackmanHarris");
      props.set("filterWidth", width);
      sharpness = 1.f - alpha;
    }
    else
    {
      // Truncated Gaussian 2.0 (PRMan, Arnold)
      props.set("filter", "gauss");
      props.set("filterWidth", 2.f);
      props.set("filterSigma", 0.5f);
      props.set("filterBias", 0);
      sharpness = 0.5f;
    }

    device->initFilter(props);
  }

  // Mutate the sampler
  if (isMutateSamplerEnabled && presetCount > 1 && presetIndex == 0)
  {
    Log() << "Mutating sampler";

    const bool isBlueNoise = rng.get1f() < 0.1f;
    initPresets(isBlueNoise);
  }

  // Mutate the rendering settings
  if (isMutateRenderEnabled)
  {
    props.set("maxDepth", (rng.get1f() < 0.98f) ? maxDepth : 1); // rarely render only direct lighting
    device->updateRenderer(props);

    // Start the jitter sequence from a random index
    jitterStart = rng.get1i(1, 1024);
  }
}

void RenderWindow::queryPixel(int x, int y)
{
  Array<Props> hits;
  device->queryPixel(x, y, hits);
  if (hits.isEmpty())
    return;

  for (int i = 0; i < hits.getSize(); ++i)
    Log() << "Pixel " << i << ": " << hits[i];

  const Props& result = hits[0];

  // Refocus
  if (view.radius > 0.f)
    view.focus = result.get("depth", view.focus);

  if (isPoiMode)
  {
    // Add point to POIs
    Poi poi;
    poi.p = result.get<Vec3f>("p");
    poi.N = normalize(result.get<Vec3f>("Ng"));
    poi.dist = result.get<float>("dist");
    poiSet.pushBack(poi);
    Log() << "POI: p=" << poi.p << " N=" << poi.N << " dist=" << poi.dist;
    savePoi(poiFilename, poi);
  }
}

void RenderWindow::queryPixelGrid()
{
  if (!isPoiMode)
    return;

  const int gridSize = imageSize.x / 16;

  for (int x = gridSize; x < imageSize.x; x += gridSize)
  {
    for (int y = gridSize; y < imageSize.y; y += gridSize)
    {
      Array<Props> hits;
      device->queryPixel(x, y, hits);
      if (hits.isEmpty())
        continue;

      const Props& result = hits[0];

      // Add point to POIs
      Poi poi;
      poi.p = result.get<Vec3f>("p");
      poi.N = normalize(result.get<Vec3f>("Ng"));
      poi.dist = result.get<float>("dist");
      poiSet.pushBack(poi);
      savePoi(poiFilename, poi);
    }
  }
}

void RenderWindow::initPresets(bool isBlueNoise)
{
  if (isBlueNoise)
  {
    const int bnSamples = int(log2(multiresMaxSpp)) + 1; // checkpointing doesn't work with blue noise sampling
    presetCountPerRes = 1 + bnSamples; // random, blue noise 1spp, 2spp, ...
  }
  else
  {
    presetCountPerRes = 1; // random only
  }

  presetCount = presetCountPerRes * 5; // full res AA, full, 1/1.5, 1/2, 1/3
}

bool RenderWindow::nextPreset()
{
  if (presetCount < 2)
    return ++presetIndex < presetCount;

  while (++presetIndex < presetCount)
  {
    const int presetRes = presetIndex / presetCountPerRes; // full AA, full, 1/1.5, 1/2, 1/3
    const int presetSmp = presetIndex % presetCountPerRes; // random, blue noise 1spp, 2spp, ...

    Vec2i presetImageSize = oriImageSize;
    switch (presetRes)
    {
    case 0:
      presetImageSize = oriImageSize;
      outputSuffix = "";
      break;

    case 1:
      presetImageSize = oriImageSize;
      outputSuffix = "-10x";
      break;

    case 2:
      presetImageSize = oriImageSize * 2 / 3;
      outputSuffix = "-15x";
      break;

    case 3:
      presetImageSize = oriImageSize / 2;
      outputSuffix = "-20x";
      break;

    case 4:
      presetImageSize = oriImageSize / 3;
      outputSuffix = "-30x";
      break;
    }

    if (presetSmp == 0)
    {
      // Random sampling
      props.set("sampler", "sobolBurley");
      maxSpp = (presetRes == 0) ? min(oriMaxSpp, adaptiveMaxSpp) : min(multiresMaxSpp, adaptiveMultiresMaxSpp);
      isCheckpointEnabled = oriIsCheckpointEnabled;
    }
    else
    {
      // Blue-noise sampling
      const int presetMaxSpp = 1 << (presetSmp - 1);
      if (presetMaxSpp > adaptiveMultiresMaxSpp)
        continue; // skip preset
      props.set("sampler", "sobolBurleyBlueNoise");
      maxSpp = presetMaxSpp;
      isCheckpointEnabled = false; // we're doing checkpoints with separate presets
      //outputSuffix += "-bn";
    }

    props.set("spp", maxSpp);

    jitter = (sequenceFrameCount > 1) && (presetRes > 0);

    initRenderer(presetImageSize.x, presetImageSize.y);
    return true; // preset applied
  }

  return false; // no more presets
}

RenderWindow::Keyframe RenderWindow::getSequenceFrame(int i)
{
  if (sequenceFrameCount <= 1)
    return sequenceKeyframes[0];

  Keyframe f;

  const float t = float(i) / float(sequenceFrameCount - 1);

  const Keyframe& key0 = sequenceKeyframes[0];
  const Keyframe& key1 = sequenceKeyframes[1];

  // Interpolate the view
  View& v = f.view;
  v.pos    = lerp(key0.view.pos,    key1.view.pos,    t);
  v.yaw    = lerp(key0.view.yaw,    key1.view.yaw,    t);
  v.pitch  = lerp(key0.view.pitch,  key1.view.pitch,  t);
  v.roll   = lerp(key0.view.roll,   key1.view.roll,   t);
  v.fovY   = lerp(key0.view.fovY,   key1.view.fovY,   t);
  v.focus  = lerp(key0.view.focus,  key1.view.focus,  t);
  v.radius = lerp(key0.view.radius, key1.view.radius, t);

  // Interpolate the light
  f.lightDir = slerp(key0.lightDir, key1.lightDir, t);

  return f;
}

void RenderWindow::setSequenceFrame(int i)
{
  if (sequenceFrameCount > 1)
    Log() << "Sequence frame: " << i;

  Keyframe frame = getSequenceFrame(i);

  // Update the view
  view = frame.view;

  if (isSequencePlaying && sequenceFrameCount > 1)
  {
    oldView  = {}; // force rendering frame from scratch
    prevView = getSequenceFrame(max(i-1, 0)).view;
    nextView = getSequenceFrame(min(i+1, sequenceFrameCount-1)).view;
  }

  // Update the light
  if (frame.lightDir != lightDir)
  {
    lightDir = frame.lightDir;
    isSceneChanged = true;
  }
}

void RenderWindow::onKeyDown(int key)
{
  switch (key)
  {
  case Key::Up:
  case 'w':
    viewPosDelta.z = -1.f;
    break;

  case Key::Down:
  case 's':
    viewPosDelta.z = 1.f;
    break;

  case 'a':
    viewPosDelta.x = -1.f;
    break;

  case 'd':
    viewPosDelta.x = 1.f;
    break;

  case Key::Left:
    viewRollDelta = -1.f;
    break;

  case Key::Right:
    viewRollDelta = 1.f;
    break;

  case Key::NumpadMinus:
  case 'q':
    viewFovDelta = 1.f;
    break;

  case Key::NumpadPlus:
  case 'e':
    viewFovDelta = -1.f;
    break;

  case '-':
    viewRadiusDelta = -1.f;
    break;

  case '=':
    viewRadiusDelta = 1.f;
    break;

  case 'p':
    view.radius = 0.f;
    Log() << "Pinhole lens";
    break;

  case 'z':
    isCameraRotateMode = false;
    break;

  case ' ':
    if (isPoiMode)
      queryPixelGrid();
    else
      saveAutoScreenshot();
    break;

  case Key::Escape:
    quit();
    break;

  case Key::Home:
    if (sequenceFrameCount > 1)
    {
      sequenceFrameIndex = 0;
      setSequenceFrame(sequenceFrameIndex);
    }
    break;

  case Key::End:
    if (sequenceFrameCount > 1)
    {
      sequenceFrameIndex = sequenceFrameCount - 1;
      setSequenceFrame(sequenceFrameIndex);
    }
    break;

  case Key::PageUp:
    if (sequenceFrameCount > 1)
    {
      sequenceFrameIndex = (sequenceFrameIndex - 1 + sequenceFrameCount) % sequenceFrameCount;
      setSequenceFrame(sequenceFrameIndex);
    }
    break;

  case Key::PageDown:
    if (sequenceFrameCount > 1)
    {
      sequenceFrameIndex = (sequenceFrameIndex + 1) % sequenceFrameCount;
      setSequenceFrame(sequenceFrameIndex);
    }
    break;

  default:
    // Change active view
    if (key >= '0' && key <= '9')
      setActiveView(key - '0');
    break;
  }

  if (isDemoMode)
    return;

  // In non-demo mode only
  switch (key)
  {
  // Reset view
  case Key::Backspace:
    resetView();
    break;

  // Save view
  case Key::Return:
    saveView();
    break;

  // Copy view
  case 'c':
    copiedView = view;
    copiedLightDir = lightDir;
    Log() << "Copied view";
    break;

  // Paste view
  case 'v':
    view = copiedView;
    if (lightDir != copiedLightDir)
    {
      lightDir = copiedLightDir;
      isSceneChanged = true;
    }
    Log() << "Pasted view";
    break;

  // Print camera
  case 'i':
    printCamera = true;
    break;

  // Loop sequence
  case 'l':
    if (sequenceFrameCount > 1)
      isSequencePlaying = !isSequencePlaying;
    break;

  // Mutate scene
  case 'm':
    mutate();
    break;

  // POI mode
  case 'x':
    isPoiMode = !isPoiMode;
    if (isPoiMode)
      Log() << "POI mode: enabled";
    else
      Log() << "POI mode: disabled";
    break;

  // Query pixel
  case '\t':
    if (mousePosX >= 0 && mousePosY >= 0)
      queryPixel(mousePosX, mousePosY);
    break;

  // Tone mapping
  case 't':
    tone.typeId = (tone.typeId+1) % toneMapperNames.getSize();
    Log() << "Tone mapping: " << toneMapperNames[tone.typeId];
    break;

  // Denoising
  case 'n':
    denoiserId = (denoiserId+1) % denoiserNames.getSize();
    initDenoiser();
    Log() << "Denoiser: " << denoiserNames[denoiserId];
    break;

  case 'h':
    denoiserMode = (denoiserMode == DenoiserMode::Ldr) ? DenoiserMode::Hdr : DenoiserMode::Ldr;
    initDenoiser();
    Log() << "Denoiser mode: " << ((denoiserMode == DenoiserMode::Ldr) ? "ldr" : "hdr");
    break;

  case '[':
    tone.ev -= 0.1f;
    Log() << "EV: " << tone.ev;
    break;

  case ']':
    tone.ev += 0.1f;
    Log() << "EV: " << tone.ev;
    break;

  case ',':
    tone.burn = clamp(tone.burn - 0.02f, 0.f, 1.f);
    Log() << "Burn: " << tone.burn;
    break;

  case '.':
    tone.burn = clamp(tone.burn + 0.02f, 0.f, 1.f);
    Log() << "Burn: " << tone.burn;
    break;

  case '\\':
  case '/':
    isAutoExposureMode = !isAutoExposureMode;
    if (isAutoExposureMode)
      Log() << "Auto exposure: enabled";
    else
      Log() << "Auto exposure: disabled";
    break;

  // AOV display
  case 'f':
    aovId = (aovId+1) % aovNames.getSize();
    Log() << "Display AOV: " << aovNames[aovId];
    break;

  // Toggle refresh
  case 'r':
    setRefreshEnabled(!getRefreshEnabled());
    break;

  case 'o':
    isTextEnabled = !isTextEnabled;
    break;
  }
}

void RenderWindow::onKeyUp(int key)
{
  switch (key)
  {
  case Key::Up:
  case 'w':
  case Key::Down:
  case 's':
    viewPosDelta.z = 0.f;
    break;

  case 'a':
  case 'd':
    viewPosDelta.x = 0.f;
    break;

  case Key::Left:
  case Key::Right:
    viewRollDelta = 0.f;
    break;

  case Key::NumpadMinus:
  case 'q':
  case Key::NumpadPlus:
  case 'e':
    viewFovDelta = 0.f;
    break;

  case '-':
  case '=':
    viewRadiusDelta = 0.f;
    break;

  case 'z':
    isCameraRotateMode = true;
    break;
  }
}

void RenderWindow::onMouseButtonDown(int button, int x, int y)
{
  switch (button)
  {
  case MouseButton::Left:
    isMouseActive = true;
    break;

  case MouseButton::Right:
    queryPixel(x, y);
    break;

  case MouseButton::WheelUp:
    viewPosSpeed = min(viewPosSpeed*viewPosSpeedMul, 1e10f);
    break;

  case MouseButton::WheelDown:
    viewPosSpeed /= viewPosSpeedMul;
    break;
  }
}

void RenderWindow::onMouseButtonUp(int button)
{
  switch (button)
  {
  case MouseButton::Left:
    isMouseActive = false;
    break;
  }
}

void RenderWindow::onMouseMotion(int x, int y, int dx, int dy)
{
  mousePosX = x;
  mousePosY = y;

  if (!isMouseActive)
    return;

  if (isCameraRotateMode)
  {
    view.yaw   += dx * viewAngleStep;
    view.pitch += dy * viewAngleStep;
    view.pitch = clamp(view.pitch, -float(pi)/2.f, float(pi)/2.f);
  }
  else
  {
    // Rotate environment lights
    float theta = acosSafe(lightDir.y);
    float phi = atan2(lightDir.z, lightDir.x);

    phi += dx * viewAngleStep*3.f;
    theta += dy * viewAngleStep*3.f;
    theta = clamp(theta, 0.f, float(pi)/2.f);

    float x = cos(phi) * sin(theta);
    float y = sin(phi) * sin(theta);
    float z = cos(theta);
    lightDir = normalize(Vec3f(x,z,y));

    Log() << "lightDir=" << lightDir;
    isSceneChanged = true;

    //scene_->environmentLight->offsetAngleX += dx * viewAngleStep_;
    //isSceneChanged_ = true;
  }
}

} // namespace prt
