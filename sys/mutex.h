// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common.h" // includes <Windows.h> on Windows

#ifndef _WIN32
#include <pthread.h>
#endif

namespace prt {

class Mutex : Uncopyable
{
private:
#ifdef _WIN32
  CRITICAL_SECTION handle;
#else
  pthread_mutex_t handle;
#endif

public:
  friend class Condition;

  Mutex()
  {
#ifdef _WIN32
    InitializeCriticalSection(&handle);
#else
    int result = pthread_mutex_init(&handle, NULL);
    assert(result == 0 && "Could not create mutex.");
#endif
  }

  ~Mutex()
  {
#ifdef _WIN32
    DeleteCriticalSection(&handle);
#else
    pthread_mutex_destroy(&handle);
#endif
  }

  void lock()
  {
#ifdef _WIN32
    EnterCriticalSection(&handle);
#else
    int result = pthread_mutex_lock(&handle);
    assert(result == 0 && "Could not lock mutex.");
#endif
  }

  bool tryLock()
  {
#ifdef _WIN32
    return TryEnterCriticalSection(&handle) != 0;
#else
    return pthread_mutex_trylock(&handle) == 0;
#endif
  }

  void unlock()
  {
#ifdef _WIN32
    LeaveCriticalSection(&handle);
#else
    int result = pthread_mutex_unlock(&handle);
    assert(result == 0 && "Could not unlock mutex.");
#endif
  }
};

} // namespace prt
