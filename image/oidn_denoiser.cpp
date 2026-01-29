// Copyright 2018 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <stdexcept>
#include "sys/timer.h"
#include "oidn_denoiser.h"

namespace prt {

  void OIDNDenoiser::init(const Params& params)
  {
    this->params = params;
    numPixels = (size_t)params.width * params.height;
    systemMemorySupported = false;

    device = oidn::newDevice();

    const char* errorMessage;
    if (device.getError(errorMessage) != oidn::Error::None)
      throw std::runtime_error(errorMessage);
    device.setErrorFunction(errorCallback);

    if (params.numThreads > 0)
      device.set("numThreads", params.numThreads);
    if (params.setAffinity >= 0)
      device.set("setAffinity", bool(params.setAffinity));
    if (params.verbose >= 0)
      device.set("verbose", params.verbose);

    device.commit();

    systemMemorySupported = device.get<bool>("systemMemorySupported") &&
                            device.get<oidn::DeviceType>("type") == oidn::DeviceType::CPU; // slow on other devices

    filter = device.newFilter(params.type);

    if (systemMemorySupported)
    {
      filter.setImage("color", params.color, oidn::Format::Float3, params.width, params.height);
    }
    else
    {
      color = device.newBuffer(numPixels * sizeof(float) * 3);
      filter.setImage("color", color, oidn::Format::Float3, params.width, params.height);
    }

    if (params.albedo)
    {
      if (systemMemorySupported)
      {
        filter.setImage("albedo", params.albedo, oidn::Format::Float3, params.width, params.height);
      }
      else
      {
        albedo = device.newBuffer(numPixels * sizeof(float) * 3);
        filter.setImage("albedo", albedo, oidn::Format::Float3, params.width, params.height);
      }
    }

    if (params.normal)
    {
      if (systemMemorySupported)
      {
        filter.setImage("normal", params.normal, oidn::Format::Float3, params.width, params.height);
      }
      else
      {
        normal = device.newBuffer(numPixels * sizeof(float) * 3);
        filter.setImage("normal", normal, oidn::Format::Float3, params.width, params.height);
      }
    }

    if (systemMemorySupported)
      filter.setImage("output", params.output, oidn::Format::Float3, params.width, params.height);
    else
      filter.setImage("output", color, oidn::Format::Float3, params.width, params.height);

    if (params.hdr)
      filter.set("hdr", true);
    if (params.srgb)
      filter.set("srgb", true);

    if (std::isfinite(params.hdrScale))
      filter.set("hdrScale", params.hdrScale);

    if (params.maxMemoryMB >= 0)
      filter.set("maxMemoryMB", params.maxMemoryMB);

    filter.set("quality", oidn::Quality::Balanced);

    filter.commit();
  }

  double OIDNDenoiser::execute(bool benchmark)
  {
    Timer timer;

    if (systemMemorySupported)
    {
      filter.execute();
    }
    else
    {
      color.writeAsync(0, numPixels * sizeof(float) * 3, params.color);
      if (params.albedo)
        albedo.writeAsync(0, numPixels * sizeof(float) * 3, params.albedo);
      if (params.normal)
        normal.writeAsync(0, numPixels * sizeof(float) * 3, params.normal);
      filter.executeAsync();
      color.read(0, numPixels * sizeof(float) * 3, params.output);
    }

    return timer.query();
  }

  std::string OIDNDenoiser::getVersion()
  {
    const int major = device.get<int>("versionMajor");
    const int minor = device.get<int>("versionMinor");
    const int patch = device.get<int>("versionPatch");

    return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
  }

  void OIDNDenoiser::errorCallback(void* userPtr, oidn::Error error, const char* message)
  {
    throw std::runtime_error(message);
  }

} // namespace prt
