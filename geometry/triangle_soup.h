// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/array.h"
#include "sys/common.h"
#include "geometry/triangle.h"

namespace prt {

struct TriangleSoup
{
  Array<FatTriangle> triangles;
  Array<std::string> materialNames;
};

} // namespace prt
