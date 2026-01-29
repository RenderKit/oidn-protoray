// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cfloat>
#include <cmath>
#include "sys/common.h"

namespace prt {

// Rounding mode
enum class Round : int
{
  Nearest = 0,
  Down = 1,
  Up = 2,
  Truncate = 3,
  Default = 4 // use MXCSR.RC
};

// Traits
// ------

template <class T>
struct ToFloat { typedef float Type; };

template <class T>
struct ToInt { typedef int Type; };

template <class T>
struct ToUInt { typedef uint Type; };

template <class T>
struct ToDouble { typedef double Type; };

template <class T>
struct ToBool { typedef bool Type; };

// Helper types
// ------------

template <class T>
using ToFloatT = typename ToFloat<T>::Type;

template <class T>
using ToIntT = typename ToInt<T>::Type;

template <class T>
using ToUIntT = typename ToUInt<T>::Type;

template <class T>
using ToDoubleT = typename ToDouble<T>::Type;

template <class T>
using ToBoolT = typename ToBool<T>::Type;

} // namespace prt
