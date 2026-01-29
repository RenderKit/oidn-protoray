// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common.h"
#include "stream.h"

namespace prt {

enum class FileMode
{
  Open,
  OpenReadWrite,
  Create,
  CreateReadWrite,
  Append,
  AppendReadWrite
};

class File : public Stream, Uncopyable
{
private:
  FILE* handle;

public:
  File();
  File(const std::string& filename, FileMode mode = FileMode::Open);
  ~File();

  void open(const std::string& filename, FileMode mode = FileMode::Open);
  void create(const std::string& filename);
  void close();

  size_t read(void* dest, size_t count)
  {
    if (!handle)
      throw std::logic_error("file is not open");

    size_t readCount = fread(dest, 1, count, handle);
    return readCount;
  }

  void write(const void* src, size_t count)
  {
    if (!handle)
      throw std::logic_error("file is not open");

    size_t writeCount = fwrite(src, 1, count, handle);
    if (writeCount != count)
      throw std::runtime_error("fwrite failed");
  }

  bool isOpen() const
  {
    return handle != 0;
  }

  static bool exists(const std::string& filename);
  static bool remove(const std::string& filename);
};

} // namespace prt
