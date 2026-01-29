// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common.h"
#include "string.h"

namespace prt {

extern const int cpuCount;
extern const std::string cpuIsa;

struct CpuInfo
{
  std::string brand;
};

int getCpuCount();
void forceCpuCount(int n);
void getCpuInfo(CpuInfo& info);

} // namespace prt

