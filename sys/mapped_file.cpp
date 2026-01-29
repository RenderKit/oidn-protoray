// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef _WIN32
// Linux
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#include "logging.h"
#include "mapped_file.h"

namespace prt {

MappedFile::MappedFile()
{
  init();
}

MappedFile::~MappedFile()
{
  if (data)
  {
    try
    {
      close();
    }
    catch (...)
    {}
  }
}

void MappedFile::open(const std::string& filename, Access access)
{
  if (data)
    throw std::logic_error("file is already open");

  this->access = access;

#ifdef _WIN32
  // Windows
  DWORD accessFlags;
  if (access == Access::Read)
    accessFlags = GENERIC_READ;
  else if (access == Access::Write)
    accessFlags = GENERIC_WRITE;
  else if (access == Access::ReadWrite)
    accessFlags = GENERIC_READ | GENERIC_WRITE;
  else
    throw std::invalid_argument("invalid access mode");

  fileHandle = CreateFileA(filename.c_str(),
                           accessFlags,
                           FILE_SHARE_READ,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

  if (fileHandle == INVALID_HANDLE_VALUE)
  {
    cleanup();
    throw std::runtime_error("could not open file: " + filename);
  }

  LARGE_INTEGER fileSize;
  if (GetFileSizeEx(fileHandle, &fileSize) == 0)
  {
    cleanup();
    throw std::runtime_error("GetFileSizeEx failed");
  }
  this->size = (size_t)fileSize.QuadPart;
#else
  // Linux
  int accessFlags;
  if (access == Access::Read)
    accessFlags = O_RDONLY;
  else if (access == Access::Write)
    accessFlags = O_WRONLY;
  else if (access == Access::ReadWrite)
    accessFlags = O_RDWR;
  else
    throw std::invalid_argument("invalid access mode");

  fileHandle = ::open(filename.c_str(), accessFlags, S_IRWXU);
  if (fileHandle == -1)
  {
    cleanup();
    throw std::runtime_error("could not open file: " + filename);
  }

  struct stat fileInfo;
  bool success = ::fstat(fileHandle, &fileInfo) == 0;
  if (!success)
  {
    cleanup();
    throw std::runtime_error("fstat failed");
  }
  this->size = (size_t)fileInfo.st_size;
#endif

  map();
}

void MappedFile::create(const std::string& filename, size_t size)
{
  if (data)
    throw std::logic_error("file is already open");

  if (size == 0)
    throw std::invalid_argument("size cannot be zero");

  access = Access::ReadWrite;
  this->size = size;

#ifdef _WIN32
  // Windows
  fileHandle = CreateFileA(filename.c_str(),
                           GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ,
                           NULL,
                           CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);
  if (fileHandle == INVALID_HANDLE_VALUE)
  {
    cleanup();
    throw std::runtime_error("could not open file: " + filename);
  }
#else
  // Linux
  int accessFlags = O_CREAT | O_TRUNC | O_RDWR;
  fileHandle = ::open(filename.c_str(), accessFlags, S_IRUSR | S_IWUSR);
  if (fileHandle == -1)
  {
    cleanup();
    throw std::runtime_error("could not open file: " + filename);
  }
#endif

  setFileSize(size);
  map();
}

void MappedFile::close()
{
  if (!data)
    throw std::logic_error("file is not open");

  cleanup();
}

void MappedFile::resize(size_t size)
{
  if (!data)
    throw std::logic_error("file is not open");

  if (access == Access::Read)
    throw std::logic_error("cannot change size in read-only mode");

  if (size == 0)
    throw std::invalid_argument("size cannot be zero");

  unmap();
  setFileSize(size);
  this->size = size;
  map();
}

void MappedFile::init()
{
  data = 0;
  this->size = 0;
  access = Access::Read;

#ifdef _WIN32
  // Windows
  fileHandle = INVALID_HANDLE_VALUE;
  mappingHandle = INVALID_HANDLE_VALUE;
#else
  // Linux
  fileHandle = -1;
#endif
}

void MappedFile::cleanup()
{
  unmap();

#ifdef _WIN32
  // Windows
  if (fileHandle != INVALID_HANDLE_VALUE)
    CloseHandle(fileHandle);
#else
  // Linux
  if (fileHandle != -1)
    ::close(fileHandle);
#endif

  init();
}

void MappedFile::map()
{
#ifdef _WIN32
  // Windows
  mappingHandle = CreateFileMappingA(fileHandle,
                                     NULL,
                                     (access == Access::Read) ? PAGE_READONLY : PAGE_READWRITE,
                                     0, 0,
                                     NULL);
  if (mappingHandle == 0)
  {
    cleanup();
    throw std::runtime_error("CreateFileMappingA failed");
  }

  data = MapViewOfFile(mappingHandle,
                       (access == Access::Read) ? FILE_MAP_READ : FILE_MAP_WRITE,
                       0, 0,
                       0);
  if (data == 0)
  {
    cleanup();
    throw std::runtime_error("MapViewOfFile failed");
  }
#else
  // Linux
  data = ::mmap(0, size,
                (access == Access::Read) ? PROT_READ : (PROT_READ | PROT_WRITE),
                (access == Access::Read) ? MAP_PRIVATE : MAP_SHARED,
                fileHandle, 0);

  if (data == MAP_FAILED)
  {
    data = 0;
    cleanup();
    throw std::runtime_error("mmap failed");
  }
#endif
}

void MappedFile::unmap()
{
#ifdef _WIN32
  // Windows
  if (data != 0)
  {
    UnmapViewOfFile(data);
    data = 0;
  }

  if (mappingHandle != INVALID_HANDLE_VALUE)
  {
    CloseHandle(mappingHandle);
    mappingHandle = INVALID_HANDLE_VALUE;
  }
#else
  // Linux
  if (data != 0)
  {
    ::munmap(data, size);
    data = 0;
  }
#endif

  data = 0;
}

void MappedFile::setFileSize(size_t size)
{
#ifdef _WIN32
  // Windows
  LARGE_INTEGER distance;
  distance.QuadPart = size;
  if (SetFilePointerEx(fileHandle, distance, NULL, FILE_BEGIN) == 0)
  {
    cleanup();
    throw std::runtime_error("SetFilePointerEx failed");
  }
  if (SetEndOfFile(fileHandle) == 0)
  {
    cleanup();
    throw std::runtime_error("SetEndOfFile failed");
  }
#else
  // Linux
  bool success = ::ftruncate(fileHandle, size) != -1;
  if (!success)
  {
    cleanup();
    throw std::runtime_error("ftruncate failed");
  }
#endif
}

} // namespace prt
