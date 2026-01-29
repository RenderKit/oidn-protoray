// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common.h"
#include "logging.h"
#include "array.h"
#include "file.h"

namespace prt {

template <class T>
inline void loadBlob(const std::string& filename, T& obj)
{
  Log() << "Loading blob: " << filename;

  File file;
  file.open(filename);
  int id;
  file >> id;
  if (id != T::blobId)
    throw std::runtime_error("file has wrong blob ID: " + filename);
  file >> obj;
}

template <class T>
inline void saveBlob(const std::string& filename, const T& obj)
{
  Log() << "Saving blob: " << filename;

  File file;
  file.create(filename);
  int id = T::blobId;
  file << id;
  file << obj;
}

} // namespace prt
