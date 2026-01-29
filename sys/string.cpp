// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <cmath>
#include "string.h"

namespace prt {

double parseTime(const std::string& str)
{
  int v[3];
  int n = sscanf(str.c_str(), "%d:%d:%d", &v[0], &v[1], &v[2]);
  double time = 0;
  for (int i = 0; i < n; ++i)
    time = time * 60 + v[i];
  return time;
}

Stream& operator <<(Stream& osm, const char* str)
{
  int size = (int)strlen(str);
  osm << size;
  osm.write(str, size);
  return osm;
}

Stream& operator <<(Stream& osm, const std::string& str)
{
  int size = (int)str.size();
  osm << size;
  osm.write(str.data(), size);
  return osm;
}

Stream& operator >>(Stream& ism, std::string& str)
{
  int size;
  ism >> size;
  if (size < 0)
    throw std::runtime_error("invalid string size in stream");
  str.resize(size);
  ism.readFull(str.data(), size);
  return ism;
}

} // namespace prt
