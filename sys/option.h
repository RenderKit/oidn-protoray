// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <iostream>
#include "array.h"
#include "value.h"

namespace prt {

// Command-line option
struct Option
{
  std::string name;
  Value value;
};

std::ostream& operator <<(std::ostream& osm, const Option& opt);
void parseOptions(int argc, char* argv[], Array<Option>& opts);
void parseOptions(const std::string& filename, Array<Option>& opts);

} // namespace prt
