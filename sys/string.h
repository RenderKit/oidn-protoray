// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstring>
#include <string>
#include <sstream>
#include "common.h"
#include "stream.h"

namespace prt {

template <class T>
inline std::string toString(const T& a)
{
  std::stringstream sm;
  sm << a;
  return sm.str();
}

template <class T>
inline std::string toString(const T& a, int precision, int base = 0)
{
  std::stringstream sm;
  sm << std::setbase(base) << std::fixed << std::setprecision(precision) << a;
  return sm.str();
}

template <class T>
inline T fromString(const std::string& str, int base = 0)
{
  std::stringstream sm(str);
  T a;
  sm >> std::setbase(base) >> a;
  return a;
}

double parseTime(const std::string& str);

Stream& operator <<(Stream& osm, const char* str);
Stream& operator <<(Stream& osm, const std::string& str);
Stream& operator >>(Stream& ism, std::string& str);

} // namespace prt
