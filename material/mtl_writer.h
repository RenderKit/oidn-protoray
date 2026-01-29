// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/common.h"
#include "sys/props.h"

namespace prt {

class MtlWriter : Uncopyable
{
public:
  MtlWriter(const std::string& filename, const PropsMap& materials);

private:
  void writeMtl(const std::string& filename, const PropsMap& materials);
  void writeMaterial(FILE* f, const Props& material);
};

} // namespace prt
