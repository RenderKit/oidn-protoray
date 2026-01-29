// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <cstring>
#include <immintrin.h>
#include <memory>
#include "common.h"
#include "stream.h"

#if defined(__AVX512F__)
#define PRT_SIMD_REG_SIZE 64 // AVX-512
#else
#define PRT_SIMD_REG_SIZE 32 // AVX
#endif

#define PRT_CACHE_LINE_SIZE 64

#if !defined(PAGE_SIZE)
#define PAGE_SIZE 4096
#endif

#ifdef _MSC_VER
#define prt_align(ALIGNMENT) __declspec(align(ALIGNMENT))
#else
#define prt_align(ALIGNMENT) __attribute__((aligned(ALIGNMENT)))
#endif

#define prt_align_simd  prt_align(PRT_SIMD_REG_SIZE)
#define prt_align_cache prt_align(PRT_CACHE_LINE_SIZE)


namespace prt {

const size_t cacheLineSize = PRT_CACHE_LINE_SIZE;
const size_t pageSize = PAGE_SIZE;

void* alignedAlloc(size_t size, size_t alignment = 0);
void alignedFree(void* ptr);

// Typed copy
template <class T>
inline void copy(T* dest, const T* src, size_t count)
{
  memcpy(dest, src, count * sizeof(T));
}

// Prefetching
// -----------

prt_inline void prefetchL1(const void* data, int offset = 0)
{
  _mm_prefetch((const char*)data+offset, _MM_HINT_T0);
}

prt_inline void prefetchL2(const void* data, int offset = 0)
{
  _mm_prefetch((const char*)data+offset, _MM_HINT_T1);
}

prt_inline void prefetchL1Ex(const void* data, int offset = 0)
{
  _mm_prefetch((const char*)data+offset, _MM_HINT_T0);
}

prt_inline void prefetchL2Ex(const void* data, int offset = 0)
{
  _mm_prefetch((const char*)data+offset, _MM_HINT_T1);
}

prt_inline void prefetch2L1(const void* data, int offset = 0)
{
  _mm_prefetch((const char*)data+offset,    _MM_HINT_T0);
  _mm_prefetch((const char*)data+offset+64, _MM_HINT_T0);
}

prt_inline void prefetch2L1Ex(const void* data, int offset = 0)
{
  _mm_prefetch((const char*)data+offset,    _MM_HINT_T0);
  _mm_prefetch((const char*)data+offset+64, _MM_HINT_T0);
}

prt_inline void prefetch2L2Ex(const void* data, int offset = 0)
{
  _mm_prefetch((const char*)data+offset,    _MM_HINT_T1);
  _mm_prefetch((const char*)data+offset+64, _MM_HINT_T1);
}

// Memory object
// -------------

class Memory
{
private:
  void* data;
  size_t size;

public:
  prt_inline Memory() : data(0), size(0) {}

  Memory(size_t size)
  {
    this->data = alignedAlloc(size);
    this->size = size;
  }

  Memory(const Memory& other)
  {
    if (other.data)
    {
      data = alignedAlloc(other.size);
      size = other.size;
      memcpy(data, other.data, size);
    }
    else
    {
      data = 0;
      size = 0;
    }
  }

  prt_inline ~Memory()
  {
    if (data) free();
  }

  Memory& operator =(const Memory& other)
  {
    if (this != &other)
    {
      if (data) free();

      if (other.data)
      {
        data = alignedAlloc(other.size);
        size = other.size;
        memcpy(data, other.data, size);
      }
    }

    return *this;
  }

  void alloc(size_t size)
  {
    if (data) free();

    data = alignedAlloc(size);
    this->size = size;
  }

  void free()
  {
    alignedFree(data);
    data = 0;
    size = 0;
  }

  prt_inline void* getData()
  {
    return data;
  }

  prt_inline const void* getData() const
  {
    return data;
  }

  prt_inline size_t getSize() const
  {
    return size;
  }
};

Stream& operator >>(Stream& ism, Memory& mem);
Stream& operator <<(Stream& osm, const Memory& mem);

} // namespace prt
