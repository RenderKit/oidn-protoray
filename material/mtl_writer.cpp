// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "mtl_writer.h"

namespace prt {

MtlWriter::MtlWriter(const std::string& filename, const PropsMap& materials)
{
  writeMtl(filename, materials);
}

void MtlWriter::writeMtl(const std::string& filename, const PropsMap& materials)
{
  FILE* f = fopen(filename.c_str(), "wt");

  for (const auto& mat : materials)
  {
    fprintf(f, "newmtl %s\n", mat.first.c_str());
    writeMaterial(f, mat.second);
    fprintf(f, "\n");
  }

  fclose(f);
}

void MtlWriter::writeMaterial(FILE* f, const Props& material)
{
  for (const auto& i : material)
  {
    fprintf(f, "\t%s ", i.first.c_str());
    Value value = i.second;

    if (value.getType() == Value::Type::Float3)
    {
      Vec3f v = value.get<Vec3f>();
      fprintf(f, "%f %f %f\n", v.x, v.y, v.z);
    }
    else
    {
      std::string s = value.get<std::string>();
      fprintf(f, "%s\n", s.c_str());
    }
  }
}

} // namespace prt
