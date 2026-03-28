// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#if defined(_WIN32)
  #include <Windows.h>
  #include <intrin.h> // __cpuid
#else
  #include <unistd.h>
  #include <sys/resource.h>
  #if defined(__APPLE__)
    #include <sys/sysctl.h>
  #else
    #include <cpuid.h>
  #endif
#endif

#include "logging.h"
#include "sysinfo.h"

//#define PRT_SINGLE_THREADED

namespace prt {

const int cpuCount = getCpuCount();

#if defined(__AVX512F__)
  const std::string cpuIsa = "avx512";
#elif defined(__AVX2__)
  const std::string cpuIsa = "avx2";
#elif defined(__AVX__)
  const std::string cpuIsa = "avx";
#elif defined(__SSE__)
  const std::string cpuIsa = "sse";
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
  const std::string cpuIsa = "neon";
#else
  const std::string cpuIsa = "base";
#endif

#if (defined(__x86_64__) || defined(_M_X64))
#if !defined(__APPLE__)
prt_inline void cpuid(int cpuInfo[4], int functionID)
{
#if defined(_WIN32)
  __cpuid(cpuInfo, functionID);
#else
  __cpuid(functionID, cpuInfo[0], cpuInfo[1], cpuInfo[2], cpuInfo[3]);
#endif
}
#endif
#endif

int getCpuCount()
{
#if defined(PRT_SINGLE_THREADED)
  return 1;
#elif defined(_WIN32)
  SYSTEM_INFO systemInfo;
  GetSystemInfo(&systemInfo);
  return static_cast<int>(systemInfo.dwNumberOfProcessors);
#else
  return static_cast<int>(sysconf(_SC_NPROCESSORS_CONF));
#endif
}

void forceCpuCount(int n)
{
  const_cast<int&>(cpuCount) = n;
}

void getCpuInfo(CpuInfo& info)
{
#if defined(__APPLE__)
  char name[256] = {};
  size_t nameSize = sizeof(name)-1;
  if (sysctlbyname("machdep.cpu.brand_string", &name, &nameSize, nullptr, 0) == 0 && strlen(name) > 0)
  {
    info.brand = name;
    return;
  }
#elif defined(__x86_64__) || defined(_M_X64)
  int regs[3][4];
  char name[sizeof(regs)+1] = {};

  cpuid(regs[0], 0x80000000);
  if (static_cast<unsigned int>(regs[0][0]) >= 0x80000004)
  {
    cpuid(regs[0], 0x80000002);
    cpuid(regs[1], 0x80000003);
    cpuid(regs[2], 0x80000004);
    memcpy(name, regs, sizeof(regs));
    if (strlen(name) > 0)
    {
      info.brand = name;
      return;
    }
  }
#endif

  info.brand = "Unknown"; // fallback
}

} // namespace prt
