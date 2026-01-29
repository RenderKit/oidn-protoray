// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/array.h"
#include "sys/props.h"

namespace prt {

class StatsRecorder
{
private:
  std::vector<std::string> names;
  std::vector<std::vector<double>> values;
  std::map<std::string, size_t> map; // maps names to indices in the values array

public:
  void reset();
  void add(const Props& stats);
  void getAverage(Props& avg);
  void saveCsv(const std::string& filename);
};

} // namespace prt
