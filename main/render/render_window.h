// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/array.h"
#include "sys/props.h"
#include "sys/timer.h"
#include "math/random.h"
#include "image/image.h"
#include "image/denoiser.h"
#include "render/device.h"
#include "window.h"
#include "view.h"
#include "stats_recorder.h"

namespace prt {

class RenderWindow : public Window
{
private:
  std::shared_ptr<Device> device;
  Vec2i imageSize = zero;
  Vec2i oriImageSize = zero;

  // AOVs
  Image3f hdrBuffer;
  Image3f hdrVarBuffer;
  Image3f hdrHalfBuffer;
  Image3f hdrUnclampBuffer;
  Image3f hdrUnclampVarBuffer;
  Image3f hdrUnclampHalfBuffer;
  Image3f ldrBuffer;
  Image3f albedoBuffer;
  Image3f albedoVarBuffer;
  Image3f albedoHalfBuffer;
  Image3f albedo1Buffer;
  Image3f diffuseAlbedoBuffer;
  Image3f diffuseAlbedo1Buffer;
  Image3f specularAlbedoBuffer;
  Image3f specularAlbedo1Buffer;
  Image3f normalBuffer;
  Image3f normalVarBuffer;
  Image3f normalHalfBuffer;
  Image3f normal1Buffer;
  Image1f depthBuffer;
  Image1f depthVarBuffer;
  Image1f depthHalfBuffer;
  // Image3f cameraNormalBuffer;
  Image1f hwDepthBuffer;
  Image1f roughnessBuffer;
  Image1f roughness1Buffer;
  Image1f alphaBuffer;
  Image3f shBuffer[4];
  Image2f motionBackBuffer;
  Image2f motionForeBuffer;
  Image3f denoiseBuffer;

  int aovId;
  Array<std::string> aovNames;

  float sceneScale; // 1 meter in scene units
  float nearClip;
  View view, oldView;
  View prevView, nextView;
  View copiedView;
  Vec3f copiedLightDir;
  int viewId;
  int defaultViewId;
  Vec3f viewPosDelta;
  float viewPosStep;
  float viewPosSpeed;
  float viewPosSpeedMul;
  float viewAngleStep;
  float viewRollDelta;
  float viewRollStep;
  float viewFovDelta;
  float viewFovStep;
  float viewRadiusDelta;
  float viewRadiusStep;
  bool isMouseActive;
  bool isCameraRotateMode;
  ViewSet viewSet;
  std::string viewFilename;
  Array<Poi> poiSet;
  std::string poiFilename;
  bool isPoiMode;
  int mousePosX, mousePosY;

  Vec3f lightDir;

  // Sequence
  struct Keyframe
  {
    View view;

    Vec3f lightDir {qnan};
  };

  Keyframe sequenceKeyframes[2];
  int sequenceFrameIndex;
  int sequenceFrameCount;
  bool isSequencePlaying;

  uint64_t seed;
  Random rng;

  bool isMutateViewEnabled;
  bool isMutateLightEnabled;
  bool isMutateLightColorEnabled;
  bool isMutateMtlEnabled;
  bool isMutateFilterEnabled;
  bool isMutateSamplerEnabled;
  bool isMutateRenderEnabled;
  float maxViewDist;

  // Tone mapping
  struct Tone
  {
    int typeId;
    float ev; // exposure value
    float burn;
  };

  Tone tone;
  Tone prevTone;
  Array<std::string> toneMapperNames;

  bool isAutoExposureMode;
  float evMin, evMax;
  float evCompMin, evCompMax;
  float evPrev;

  float maxRadiance;
  float autoMaxRadiance;

  // Denoising
  enum class DenoiserMode
  {
    Ldr,
    Hdr,
  };

  struct DenoiserSet
  {
    std::shared_ptr<Denoiser> ldr;
    std::shared_ptr<Denoiser> hdr;
  };

  int denoiserId;
  DenoiserMode denoiserMode;
  std::shared_ptr<Denoiser> denoiser;
  Array<DenoiserSet> denoisers;
  Array<std::string> denoiserNames;
  int denoiserSpp; // activate denoiser after this SPP

  Timer timer;
  double runTimeThreshold;  // sec
  Timer statsTimer;
  Timer screenshotTimer;
  double autoShotThreshold; // sec
  bool isCheckpointEnabled;
  bool oriIsCheckpointEnabled;
  int frameCount;
  int spp;
  int minSpp;
  int maxSpp;
  int oriMaxSpp;
  int adaptiveMaxSpp;
  int multiresMaxSpp;
  int multiresMinSpp;
  int adaptiveMultiresMaxSpp;
  int minVarSpp; // minimum SPP for variance
  float maxNoise; // maximum noise error threshold
  float multiresMaxNoise; // maximum noise error threshold for multires
  float prevNoise; // previous noise value
  bool jitter;
  int jitterStart; // jitter sequence start index, 0 if no jitter
  int presetCount;
  int presetCountPerRes; // number of presets per resolution
  int presetIndex;
  float sharpness;

  Props frameStats;
  double currentFps;
  double currentMray;
  bool isFrameStatsPrinted;
  double displayTimeSum;
  int displayTimeCount;
  bool isTextEnabled;
  bool isBenchmarkMode;
  int warmupSpp;
  bool isBatchMode;
  int batchCount;
  bool isOutputEnabled;
  std::string outputPrefix;
  std::string outputSuffix;
  std::string outputId;
  std::string doneFilename;
  StatsRecorder statsRecorder;
  std::string deviceInfo;
  bool isDemoMode;
  bool printCamera;
  Props buildStats;

  Props props;
  int maxDepth;
  bool isSceneChanged;

  bool isIrradianceMode;

public:
  RenderWindow(int width, int height, DisplayMode mode, const std::shared_ptr<Device>& device, const Props& props);

private:
  void onInit();
  void onDestroy();
  void onRender();

  void initRenderer(int width, int height);

  Props makeCamera(View view);
  Props makeCamera();

  void updateFrame(bool color = true);
  void updateSurface(Surface& surface);
  std::pair<int, int> renderPois(Surface& surface, int bucketCountX = 3, int bucketCountY = 3);
  std::pair<int, int> getVisiblePoiCount(int bucketCountX, int bucketCountY);
  void autoExpose();

  void resetMaxRadiance();
  void setAutoMaxRadiance();

  void resetView();
  void setActiveView(int id);
  void saveView();
  bool isViewSalient(float minCoverage, float minPoiCoverage);

  void mutate();
  void queryPixel(int x, int y);
  void queryPixelGrid();

  void initPresets(bool isBlueNoise);
  bool nextPreset();

  Keyframe getSequenceFrame(int i);
  void setSequenceFrame(int i);

  void onKeyDown(int key);
  void onKeyUp(int key);
  void onMouseButtonDown(int button, int x, int y);
  void onMouseButtonUp(int button);
  void onMouseMotion(int x, int y, int dx, int dy);

  void initDenoiser();
  void transformNormals(const Vec2i& size, Vec3f* normal);

  void printFrameStats();
  void saveAvgStats(const Props& frameStats, const std::string& filename);
  void saveFullStats(const std::string& filename);
  void saveScreenshot(const std::string& basename, bool withSequenceFrameIndex = false);
  void saveAutoScreenshot();
};

} // namespace prt
