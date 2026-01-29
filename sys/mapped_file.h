// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef _WIN32
// Windows
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

#include "common.h"

namespace prt {

class MappedFile : Uncopyable
{
private:
  void* data;
  size_t size;
  Access access;

#ifdef _WIN32
  // Windows
  HANDLE fileHandle;
  HANDLE mappingHandle;
#else
  // Linux
  int fileHandle;
#endif

public:
  MappedFile();
  ~MappedFile();

  void open(const std::string& filename, Access access);
  void create(const std::string& filename, size_t getSize);
  void close();

  bool isOpen() const
  {
    return data != 0;
  }

  const void* getData() const
  {
    return data;
  }

  void* getData()
  {
    return data;
  }

  size_t getSize() const
  {
    return size;
  }

  void resize(size_t getSize);

private:
  void init();
  void cleanup();
  void map();
  void unmap();
  void setFileSize(size_t getSize);
};

} // namespace prt
