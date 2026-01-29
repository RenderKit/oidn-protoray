// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <immintrin.h>
#include "tasking.h"

// This improves TBB performance a bit (avoids calling into the kernel)
#if !defined(_WIN32)
int sched_yield()
{
  _mm_pause();
  return 0;
}
#endif

namespace prt {

  int Tasking::maxThreadCount = std::numeric_limits<int>::max();

}
