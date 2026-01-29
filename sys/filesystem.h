// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "string.h"

namespace prt {

std::string getFilenameBase(const std::string& filename);
std::string getFilenameExt(const std::string& filename);
std::string getFilenamePrefix(const std::string& filename);
std::string convertFilename(const std::string& filename);

bool isDirectory(const std::string& path);
bool makeDirectory(const std::string& path);

std::string getTempPath();
void setTempPath(const std::string& path);
std::string makeTempFilename();

} // namespace prt
