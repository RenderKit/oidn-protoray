// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "image_texture.h"

namespace prt {

std::shared_ptr<Image<int>> bumpToNormalMap(const std::shared_ptr<ImageTexture>& bump);

} // namespace prt
