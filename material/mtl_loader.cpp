// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <string>
#include <map>
#include "sys/logging.h"
#include "sys/filesystem.h"
#include "sys/file.h"
#include "sys/text_reader.h"
#include "mtl_loader.h"

namespace prt {

MtlLoader::MtlLoader(const std::string& filename, PropsMap& materials)
  : materials(materials)
{
  bufferSize = 256 * 1024;
  buffer = new char[bufferSize];

  parseMtl(filename);
}

MtlLoader::~MtlLoader()
{
  delete[] buffer;
}

void MtlLoader::parseMtl(const std::string& filename)
{
  Log() << "Loading MTL file: " << filename;

  TextReader file(std::make_shared<File>(filename));

  char head[256];

  while (file.readString(head, sizeof(head)) != EOF)
  {
    buffer[0] = 0;
    file.readLine(buffer, bufferSize);

    if (strcmp(head, "newmtl") == 0)
    {
      parseNewMtl();
    }
    else if (head[0] == '#')
    {
      // Comment
    }
    else
    {
      parseValue(head);
    }
  }
}

void MtlLoader::parseNewMtl()
{
  char name[256];
  if (sscanf(buffer, "%255s", name) != 1)
    throw std::runtime_error("invalid material");

  materialName = name;
}

void MtlLoader::parseValue(const char* name)
{
  char s[256];
  Vec3f v;

  switch (sscanf(buffer, "%f %f %f", &v.x, &v.y, &v.z))
  {
  case 0:
  case 1:
    // Parse as string
    if (sscanf(buffer, " %255[^\n]", s) != 1)
      throw std::runtime_error("invalid statement");
    materials[materialName].set(name, s);
    break;

  case 2:
    materials[materialName].set(name, Vec2f(v.x, v.y));
    break;

  case 3:
    materials[materialName].set(name, v);
    break;

  default:
    throw std::runtime_error("invalid statement");
  }
}

} // namespace prt
