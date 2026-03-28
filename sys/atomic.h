// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common.h"
#include "memory.h"

namespace prt {

prt_inline int atomicSwap(volatile int* ptr, int val)
{
  return __sync_lock_test_and_set(ptr, val);
}

prt_inline uint32_t atomicSwap(volatile uint32_t* ptr, uint32_t val)
{
  return __sync_lock_test_and_set(ptr, val);
}

prt_inline int atomicCas(volatile int* ptr, int oldVal, int newVal)
{
  return __sync_val_compare_and_swap(ptr, oldVal, newVal);
}

prt_inline uint32_t atomicCas(volatile uint32_t* ptr, uint32_t oldVal, uint32_t newVal)
{
  return __sync_val_compare_and_swap(ptr, oldVal, newVal);
}

prt_inline int atomicAdd(volatile int* ptr, int val)
{
  return __sync_fetch_and_add(ptr, val);
}

prt_inline uint32_t atomicAdd(volatile uint32_t* ptr, uint32_t val)
{
  return __sync_fetch_and_add(ptr, val);
}

prt_inline float atomicAdd(volatile float* ptr, float val)
{
  union
  {
    float f;
    int i;
  } oldVal, newVal;

  for (; ;)
  {
    oldVal.f = *ptr;
    newVal.f = oldVal.f + val;
    if (atomicCas((volatile int*)ptr, oldVal.i, newVal.i) == oldVal.i)
      break;
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    __asm__ volatile("yield");
#else
    _mm_pause();
#endif
  }

  return oldVal.f;
}

// Atomic counter
template <class T>
class Atomic
{
private:
  volatile T data;

public:
  prt_inline Atomic() {}
  prt_inline Atomic(T a) : data(a) {}
  prt_inline Atomic(const Atomic<T>& a) : data(a.data) {}

  prt_inline Atomic<T>& operator =(T a)
  {
    data = a;
    return *this;
  }

  prt_inline Atomic<T>& operator =(const Atomic<T>& a)
  {
    data = a.data;
    return *this;
  }

  prt_inline operator T() const { return data; }

  prt_inline T operator +=(T a) { return atomicAdd(&data, a) + a; }
  prt_inline T operator -=(T a) { return atomicAdd(&data, T(-a)) - a; }

  prt_inline T operator ++() { return atomicAdd(&data, 1) + 1; }
  prt_inline T operator --() { return atomicAdd(&data, T(-1)) - 1; }

  prt_inline T operator ++(int) { return atomicAdd(&data, 1); }
  prt_inline T operator --(int) { return atomicAdd(&data, T(-1)); }
};

// Aligned atomic counter
template <class T>
class prt_align_cache AlignedAtomic
{
private:
  volatile T data;

public:
  prt_inline AlignedAtomic() {}
  prt_inline AlignedAtomic(T a) : data(a) {}
  prt_inline AlignedAtomic(const AlignedAtomic<T>& a) : data(a.data) {}

  prt_inline AlignedAtomic<T>& operator =(T a)
  {
    data = a;
    return *this;
  }

  prt_inline AlignedAtomic<T>& operator =(const AlignedAtomic<T>& a)
  {
    data = a.data;
    return *this;
  }

  prt_inline operator T() const { return data; }

  prt_inline T operator +=(T a) { return atomicAdd(&data, a) + a; }
  prt_inline T operator -=(T a) { return atomicAdd(&data, T(-a)) - a; }

  prt_inline T operator ++() { return atomicAdd(&data, 1) + 1; }
  prt_inline T operator --() { return atomicAdd(&data, T(-1)) - 1; }

  prt_inline T operator ++(int) { return atomicAdd(&data, 1); }
  prt_inline T operator --(int) { return atomicAdd(&data, T(-1)); }
};

} // namespace prt
