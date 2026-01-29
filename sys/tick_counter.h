// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common.h"

#if defined(_MSC_VER)
#include <intrin.h>
#elif defined(__INTEL_COMPILER)
#include <ia32intrin.h>
#endif

namespace prt {

class TickCounter
{
private:
  uint64_t startCount;

public:
#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
  TickCounter()
  {
    reset();
  }

  void reset()
  {
    int cpuInfo[4];
    __cpuid(cpuInfo, 0);

    startCount = __rdtsc();
  }

  uint64_t query() const
  {
    //unsigned int aux;
    //uint64_t currentCount = __rdtscp(&aux);
    uint64_t currentCount = __rdtsc();

    int cpuInfo[4];
    __cpuid(cpuInfo, 0);

    return currentCount - startCount;
  }

  static uint64_t now()
  {
    //unsigned int aux;
    //return __rdtscp(&aux);
    return __rdtsc();
  }
#else
  static uint64_t now()
  {
    uint64_t x;
     __asm__ volatile ("rdtsc" : "=A" (x));
     return x;
  }
#endif
};

} // namespace prt
