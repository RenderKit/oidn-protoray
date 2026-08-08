// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <fstream>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h> // _mkdir
#endif
#include "math/random.h"
#include "mutex.h"
#include "lock_guard.h"
#include "filesystem.h"

namespace prt {

std::string getFilenameBase(const std::string& filename)
{
  // FIXME
  return filename.substr(0, filename.find_last_of('.'));
}

std::string getFilenameExt(const std::string& filename)
{
  // FIXME
  return filename.substr(filename.find_last_of('.') + 1);
}

std::string getFilenamePrefix(const std::string& filename)
{
  auto pos = filename.find_last_of("/\\");
  if (pos == std::string::npos)
    return "";
  return filename.substr(0, pos+1);
}

std::string convertFilename(const std::string& filename)
{
  std::string newFilename = filename;
  std::replace(newFilename.begin(), newFilename.end(), '\\', '/');
  return newFilename;
}

bool isDirectory(const std::string& path)
{
  struct stat s;
  if (stat(path.c_str(), &s) != 0)
    return false;
#ifdef _WIN32
  // The CRT does not provide S_ISDIR
  return (s.st_mode & _S_IFMT) == _S_IFDIR;
#else
  return S_ISDIR(s.st_mode);
#endif
}

bool makeDirectory(const std::string& path)
{
#ifdef _WIN32
  return _mkdir(path.c_str()) == 0;
#else
  return mkdir(path.c_str(), 0777) == 0;
#endif
}

// Temp files
// ----------

namespace
{
  std::string tempPath;
  Random random(getRandomSeed64());
  Mutex mutex;
}

std::string getTempPath()
{
  return tempPath;
}

void setTempPath(const std::string& path)
{
  std::string newTempPath = convertFilename(path);

  // Remove trailing slashes
  for (int i = static_cast<int>(newTempPath.size() - 1); i >= 0; --i)
  {
    if (newTempPath[i] != '/')
    {
      newTempPath.erase(i + 1);
      break;
    }
  }

  tempPath = newTempPath + '/';
}

std::string makeTempFilename()
{
  LockGuard<Mutex> lockGuard(mutex);

  std::stringstream stringStream;
  stringStream << tempPath << ".temp-";

  for (int i = 0; i < 4; ++i)
  {
    stringStream << std::hex << std::setw(8) << std::setfill('0') << random.get1ui();
  }

  return stringStream.str();
}

} // namespace prt
