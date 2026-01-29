// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/common.h"
#include "sys/array.h"
#include "sys/props.h"
#include "math/vec3.h"
#include "material.h"

namespace prt {

class MtlLoader : Uncopyable
{
private:
  char* buffer;
  int bufferSize;
  PropsMap& materials;
  std::string materialName;

public:
  MtlLoader(const std::string& filename, PropsMap& materials);
  ~MtlLoader();

private:
  void parseMtl(const std::string& filename);
  void parseNewMtl();
  void parseValue(const char* name);
};

} // namespace prt
