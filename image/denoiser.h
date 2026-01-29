// Copyright 2018 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>
#include <cmath>

namespace prt {

  class Denoiser
  {
  public:
  struct Params
  {
    const char* type = nullptr;
    float* color = nullptr;
    float* albedo = nullptr;
    float* normal = nullptr;
    float* output = nullptr;
    int width = 0;
    int height = 0;
    bool hdr = true;
    bool srgb = false;
    float hdrScale = NAN;
    int maxMemoryMB = -1;
    int numThreads = -1;
    int setAffinity = -1;
    int verbose = -1;
  };

  virtual ~Denoiser() {}

  // Initializes the denoising filter
  virtual void init(const Params& params) = 0;

  // Executes the filter and returns the actual denoising time in seconds
  virtual double execute(bool benchmark = false) = 0;

  virtual std::string getVersion() = 0;
  };

} // namespace prt
