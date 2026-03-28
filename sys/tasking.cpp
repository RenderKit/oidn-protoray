// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#if !defined(__ARM_NEON) && !defined(__ARM_NEON__)
#include <immintrin.h>
#endif
#include "tasking.h"

// This improves TBB performance a bit (avoids calling into the kernel)
#if !defined(_WIN32)
int sched_yield()
{
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
  __asm__ __volatile__("yield" ::: "memory");
#else
  _mm_pause();
#endif
  return 0;
}
#endif

namespace prt {

  int Tasking::maxThreadCount = std::numeric_limits<int>::max();

}
