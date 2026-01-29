// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifdef _WIN32
#include <malloc.h>
#else
#include <sys/mman.h>
#include <cstdlib>
#endif

#include "logging.h"
#include "memory.h"

namespace prt {

void* alignedAlloc(size_t size, size_t alignment)
{
  void* ptr;

  if (alignment == 0)
    alignment = size < pageSize ? cacheLineSize : pageSize;

#if defined(_MSC_VER)
  ptr = _aligned_malloc(size, alignment);
#elif defined(__INTEL_COMPILER)
  ptr = _mm_malloc(size, alignment);
#else
  if (posix_memalign(&ptr, max(alignment, sizeof(void*)), size) != 0) {
    ptr = 0;
  }
#endif

  if (ptr == 0)
    LogError() << "alignedAlloc failed (size=" << size << ", alignment=" << alignment << ")";

  return ptr;
}

void alignedFree(void* ptr)
{
#if defined(_MSC_VER)
  _aligned_free(ptr);
#elif defined(__INTEL_COMPILER)
  _mm_free(ptr);
#else
  free(ptr);
#endif
}

Stream& operator >>(Stream& ism, Memory& mem)
{
  size_t size;
  ism >> size;
  mem.alloc(size);
  ism.readFull(mem.getData(), size);
  return ism;
}

Stream& operator <<(Stream& osm, const Memory& mem)
{
  osm << mem.getSize();
  osm.write(mem.getData(), mem.getSize());
  return osm;
}

} // namespace prt
