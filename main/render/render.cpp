// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <iostream>
#include "sys/string.h"
#include "sys/logging.h"
#include "sys/memory.h"
#include "sys/sysinfo.h"
#include "sys/filesystem.h"
#include "sys/option.h"
#include "sys/tasking.h"

#include "render/device_cpu.h"

#include "main/main.h"
#include "render_window.h"

namespace prt {

namespace {

std::string inputPath;
std::string sceneFilename;
std::string deviceId = "cpu";
#ifdef PRT_GUI_SUPPORT
DisplayMode displayMode = DisplayMode::Window;
#else
DisplayMode displayMode = DisplayMode::Offscreen;
#endif
Vec2i imageSize(1024, 576);
int viewId = 0;
bool isBenchmarkMode = false;
bool isOutputEnabled = false;
std::string outputPrefix;
Props props;
LogLevel logLevel = LogLevel::Info;

bool applyOptions(const Array<Option>& opts)
{
  for (const Option& opt : opts)
  {
    if (opt.name.empty())
    {
      sceneFilename = inputPath + std::string(opt.value);
    }
    else if (opt.name == "c" || opt.name == "config")
    {
      inputPath = getFilenamePrefix(opt.value);
      Array<Option> opts2;
      parseOptions(opt.value, opts2);
      if (!applyOptions(opts2))
        return false;
    }
    else if (opt.name == "dev" || opt.name == "device" || opt.name == "e" || opt.name == "engine")
    {
      deviceId = opt.value;
    }
    else if (opt.name == "r")
    {
      props.set("renderer", opt.value);
    }
    else if (opt.name == "i" || opt.name == "isect")
    {
      props.set("isect", opt.value);
    }
    else if (opt.name == "s" || opt.name == "size")
    {
      imageSize = opt.value.get<Vec2i>();
    }
    else if (opt.name == "dw")
    {
      displayMode = DisplayMode::Window;
    }
    else if (opt.name == "do")
    {
      displayMode = DisplayMode::Offscreen;
    }
    else if (opt.name == "df")
    {
      displayMode = DisplayMode::Fullscreen;
    }
    else if (opt.name == "benchmark" || opt.name == "bench")
    {
      isBenchmarkMode = true;
      outputPrefix = opt.value;
      if (outputPrefix.empty())
        outputPrefix = "benchmark";
      isOutputEnabled = true;
      props.set(opt.name, opt.value);
    }
    else if (opt.name == "o" || opt.name == "out" || opt.name == "output")
    {
      outputPrefix = opt.value;
      isOutputEnabled = true;
    }
    else if (opt.name == "view")
    {
      viewId = opt.value.get<int>();

      if (viewId < 0 || viewId >= ViewSet::size)
      {
        LogError() << "Invalid view";
        return false;
      }
    }
    else if (opt.name == "verbose")
    {
      logLevel = LogLevel(opt.value.get<int>());
    }
    else if (opt.name == "temp")
    {
      setTempPath(opt.value);
    }
    else if (opt.name == "maxThreads")
    {
      Tasking::setMaxThreadCount(opt.value.get<int>());
    }
    else
    {
      props.set(opt.name, opt.value);
    }
  }

  return true;
}

} // namespace

int mainRender(int argc, char* argv[])
{
  // Parse the options
  if (argc < 2)
  {
    std::cout << "Usage: protoray render [options]" << std::endl;
    return 0;
  }

  Array<Option> opts;
  parseOptions(argc, argv, opts);
  if (!applyOptions(opts))
    return 1;

  if (sceneFilename.empty())
  {
    LogError() << "Scene not specified";
    return 1;
  }

  std::string sceneBase = getFilenameBase(sceneFilename);
  std::string sceneName = sceneBase.substr(sceneBase.find_last_of("/\\") + 1);

  // Initialize the RNG seed
  uint64_t seed = 0;
  if (props.exists("seed"))
  {
    seed = props.get<uint64_t>("seed", 16);
  }
  else
  {
    seed = getRandomSeed64();
    props.set("seed", seed);
  }

  // Initialize logging to file
  std::string logPath = props.get("log", "");

  if (isBenchmarkMode && logPath.empty())
  {
    logPath = outputPrefix + ".log";
  }
  else
  {
    bool isDir = false;
    if (!logPath.empty() && isDirectory(logPath))
    {
      isDir = true;
      logPath = logPath + "/";
    }

    if (logPath.empty() || isDir)
    {
      std::stringstream sm;
      if (props.exists("batch") && seed != 0)
      {
        sm << "render_" << std::hex << std::setfill('0') << std::setw(16) << seed << ".log";
        logPath += sm.str();
      }
      else
      {
        logPath += "render.log";
      }
    }
  }

  setLogFile(logPath);

  #ifdef DEBUG
    LogWarn() << "Debug build";
  #endif

  // Log command line
  std::string commandLine = std::string(argv[0]) + " render ";
  for (int i = 1; i < argc; ++i)
     commandLine += std::string(argv[i]) + " ";

  Log() << "Command line: " << commandLine;

  // Log version
  Log() << "Commit: " << PRT_HASH;

  // Set log level
  setMaxLogLevel(logLevel);

  // Check the display settings
  if (imageSize.x <= 0 || imageSize.x % 8 != 0 ||
    imageSize.y <= 0 || imageSize.y % 8 != 0)
  {
    LogError() << "Resolution must be multiple of 8";
    return 1;
  }

  // CPU info
  CpuInfo cpuInfo;
  getCpuInfo(cpuInfo);
  Log() << "CPU: " << cpuInfo.brand;
  Log() << "CPU threads: " << getCpuCount();
  Log() << "CPU SIMD width: " << simdSize;

  // Initialize device
  const char* binPath = argv[0];
  std::shared_ptr<Device> device;

  if (deviceId == "cpu")
  {
    device = std::make_shared<DeviceCpu>();
  }
  else
  {
    LogError() << "Invalid device: " << deviceId;
    return 1;
  }

  std::string deviceInfo = device->getInfo();
  if (deviceInfo != cpuInfo.brand)
    Log() << "Device: " << deviceInfo;

  device->initScene(sceneFilename, props);

  // Create and start the UI
  props.set("viewFile", sceneBase + ".view");
  props.set("view", viewId);
  props.set("poiFile", sceneBase + ".poi");

  if (isBenchmarkMode)
    props.set("benchmark", outputPrefix);
  if (isOutputEnabled)
    props.set("output", outputPrefix);

  RenderWindow window(imageSize.x, imageSize.y, displayMode, device, props);
  window.setTitle("ProtoRay");
  window.run();

  return 0;
}

} // namespace prt
