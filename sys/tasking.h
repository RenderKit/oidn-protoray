// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>
#include <tbb/blocked_range.h>
#include <tbb/blocked_range2d.h>
#include <tbb/task_arena.h>
#include "atomic.h"
#include "math/vec2.h"

namespace prt {

class Tasking
{
private:
  static int maxThreadCount;

public:
  template <class Kernel>
  static void run(const Vec2i& gridSize, Kernel&& kernel)
  {
    AlignedAtomic<int> nextTaskId = 0;
    int taskCount = gridSize.x * gridSize.y;

    tbb::parallel_for(tbb::blocked_range<int>(0, getThreadCount()), [&](const tbb::blocked_range<int>& r)
    {
      int threadId = getThreadIndex();

      for (; ;)
      {
        int taskId = nextTaskId++;
        if (taskId >= taskCount) break;

        Vec2i taskId2(taskId % gridSize.x, taskId / gridSize.x);
        kernel(taskId2, threadId);
      }
    }, tbb::static_partitioner());
  }

  static int getThreadCount()
  {
    return min(tbb::this_task_arena::max_concurrency(), maxThreadCount);
  }

  static int getThreadIndex()
  {
    return tbb::this_task_arena::current_thread_index();
  }

  static void setMaxThreadCount(int count)
  {
    maxThreadCount = count;
  }
};

} // namespace prt
