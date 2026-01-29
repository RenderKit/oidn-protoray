// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <fstream>
#include "logging.h"
#include "file.h"

namespace prt {

File::File()
  : handle(0)
{
}

File::File(const std::string& filename, FileMode mode)
  : handle(0)
{
  open(filename, mode);
}

File::~File()
{
  if (handle)
  {
    try
    {
      close();
    }
    catch (...)
    {}
  }
}

void File::open(const std::string& filename, FileMode mode)
{
  if (handle)
    throw std::logic_error("file is already open");

  const char* modeStr = 0;
  switch (mode)
  {
  case FileMode::Open:            modeStr = "rb";  break;
  case FileMode::OpenReadWrite:   modeStr = "r+b"; break;
  case FileMode::Create:          modeStr = "wb";  break;
  case FileMode::CreateReadWrite: modeStr = "w+b"; break;
  case FileMode::Append:          modeStr = "ab";  break;
  case FileMode::AppendReadWrite: modeStr = "a+b"; break;

  default:
    throw std::invalid_argument("invalid file mode");
  }

  handle = fopen(filename.c_str(), modeStr);
  if (!handle)
    throw std::runtime_error("could not open file: " + filename);
}

void File::create(const std::string& filename)
{
  open(filename, FileMode::Create);
}

void File::close()
{
  if (!handle)
    throw std::logic_error("file is not open");

  if (fclose(handle) != 0)
    throw std::runtime_error("fclose failed");

  handle = 0;
}

bool File::exists(const std::string& filename)
{
  // FIXME
  std::ifstream is(filename.c_str());
  return (bool)is;
}

bool File::remove(const std::string& filename)
{
  return std::remove(filename.c_str()) == 0;
}

} // namespace prt
