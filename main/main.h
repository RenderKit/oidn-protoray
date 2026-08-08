// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "main/config.h" // generated into the build tree

namespace prt {

#ifdef PRT_CONVERT_SUPPORT
int mainConvert(int argc, char* argv[]);
#endif
int mainRender(int argc, char* argv[]);
int mainSandbox(int argc, char* argv[]);

} // namespace prt
