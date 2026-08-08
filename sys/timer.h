// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common.h" // includes <Windows.h> on Windows

#if defined(__APPLE__)
#include <mach/mach_time.h>
#elif !defined(_WIN32)
#include <ctime>
#endif

namespace prt {

class Timer
{
private:
#if defined(_WIN32)
  double invCountsPerSec;
  LARGE_INTEGER startCount;
#elif defined(__APPLE__)
  double invCountsPerSec;
  uint64_t startTime;
#else
  timespec startTime;
#endif

public:
  Timer()
  {
#if defined(_WIN32)
    LARGE_INTEGER frequency;

    BOOL result = QueryPerformanceFrequency(&frequency);
    assert(result != 0 && "timer is not supported");

    invCountsPerSec = 1.0 / (double)frequency.QuadPart;
#elif defined(__APPLE__)
    mach_timebase_info_data_t timebaseInfo;
    mach_timebase_info(&timebaseInfo);
    invCountsPerSec = (double)timebaseInfo.numer / (double)timebaseInfo.denom * 1e-9;
#endif

    reset();
  }

  void reset()
  {
#if defined(_WIN32)
    BOOL result = QueryPerformanceCounter(&startCount);
    assert(result != 0 && "could not query counter");
#elif defined(__APPLE__)
    startTime = mach_absolute_time();
#else
    int result = clock_gettime(CLOCK_MONOTONIC, &startTime);
    assert(result == 0 && "could not get time");
#endif
  }

  double query() const
  {
#if defined(_WIN32)
    LARGE_INTEGER currentCount;

    BOOL result = QueryPerformanceCounter(&currentCount);
    assert(result != 0 && "could not query counter");

    return (currentCount.QuadPart - startCount.QuadPart) * invCountsPerSec;
#elif defined(__APPLE__)
    uint64_t endTime = mach_absolute_time();
    uint64_t elapsedTime = endTime - startTime;
    return (double)elapsedTime * invCountsPerSec;
#else
    timespec currentTime;

    int result = clock_gettime(CLOCK_MONOTONIC, &currentTime);
    assert(result == 0 && "could not get time");

    return (double)(currentTime.tv_sec - startTime.tv_sec) + (double)(currentTime.tv_nsec - startTime.tv_nsec) * 1e-9;
#endif
  }
};

} // namespace prt
