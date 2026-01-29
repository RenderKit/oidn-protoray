// Copyright 2018 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <OpenImageDenoise/oidn.hpp>
#include "denoiser.h"

namespace prt {

  class OIDNDenoiser : public Denoiser
  {
  private:
    Params params;
    oidn::DeviceRef device;
    oidn::FilterRef filter;
    oidn::BufferRef color, albedo, normal;
    size_t numPixels;
    bool systemMemorySupported;

  public:
    void init(const Params& params) override;
    double execute(bool benchmark) override;
    std::string getVersion() override;

  private:
    static void errorCallback(void* userPtr, oidn::Error error, const char* message);
  };

} // namespace prt
