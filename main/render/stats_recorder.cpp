// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <fstream>
#include "sys/logging.h"
#include "stats_recorder.h"

namespace prt {

void StatsRecorder::reset()
{
  names.clear();
  values.clear();
  map.clear();
}

void StatsRecorder::add(const Props& stats)
{
  for (const auto& i : stats)
  {
    std::string name = i.first;
    size_t valuesId;
    auto nameIter = map.find(name);

    if (nameIter == map.end())
    {
      names.push_back(name);
      valuesId = values.size();
      values.emplace_back();
      map[name] = valuesId;
    }
    else
    {
      valuesId = nameIter->second;
    }

    double value = i.second.get<double>();
    values[valuesId].push_back(value);
  }
}

void StatsRecorder::getAverage(Props& avg)
{
  for (const std::string& name : names)
  {
    size_t valuesId = map[name];

    int count = (int)values[valuesId].size();
    double sum = 0.0;

    for (int t = 0; t < count; ++t)
      sum += values[valuesId][t];

    sum /= count;

    avg.set(name, sum);
  }
}

void StatsRecorder::saveCsv(const std::string& filename)
{
  size_t maxCount = 0;
  for (size_t i = 0; i < values.size(); ++i)
    maxCount = max(maxCount, values[i].size());

  FILE* file = fopen(filename.c_str(), "wt");

  for (size_t i = 0; i < names.size(); ++i)
  {
    if (i > 0)
      fprintf(file, ",");
    fprintf(file, "%s", names[i].c_str());
  }
  fprintf(file, "\n");

  for (size_t t = 0; t < maxCount; ++t)
  {
    for (size_t i = 0; i < names.size(); ++i)
    {
      const std::string& name = names[i];
      size_t valuesId = map[name];

      if (i > 0)
        fprintf(file, ",");
      if (t < values[valuesId].size())
        fprintf(file, "%f", values[valuesId][t]);
    }
    fprintf(file, "\n");
  }

  fclose(file);
}

} // namespace prt
