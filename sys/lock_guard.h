// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common.h"

namespace prt {

template <class Lock>
class LockGuard : Uncopyable
{
private:
  Lock& lock;

public:
  explicit LockGuard(Lock& lock)
    : lock(lock)
  {
    lock.lock();
  }

  ~LockGuard()
  {
    lock.unlock();
  }
};

} // namespace prt
