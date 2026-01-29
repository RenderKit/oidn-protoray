// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "sys/common.h"
#include "sys/string.h"
#include "sys/sysinfo.h"
#include "sys/logging.h"
#include "sys/filesystem.h"
#include "sys/blob.h"
#include "sys/option.h"
#include "sys/timer.h"
#include "geometry/triangle_mesh_builder.h"
#include "material/mtl_loader.h"
#include "material/material_factory.h"
#include "obj_loader.h"

#define RKSCENE_GZIP_SUPPORT
#include "rkscene/rkscene.h"
#include "rkscene/rkscene_gltf.h"

namespace prt {

int mainConvert(int argc, char* argv[])
{
  if (argc < 2)
  {
    std::cout << "Usage: protoray convert [options]" << std::endl;
    return 0;
  }

  setLogFile("convert.log");
  // Parse the options
  Array<Option> opts;
  parseOptions(argc, argv, opts);

  std::string input;
  std::string output;
  Props convertProps;

  for (Option& opt : opts)
  {
    if (opt.name.empty())
    {
      if (!input.empty())
      {
        LogError() << "Multiple inputs";
        return 1;
      }
      input = opt.value;
    }
    else if (opt.name == "o")
    {
      if (!output.empty())
      {
        LogError() << "Multiple outputs";
        return 1;
      }
      output = opt.value;
    }
    else
    {
      Log() << "Option: " << opt;
      convertProps.set(opt.name, opt.value);
    }
  }

  std::string inputBase = getFilenameBase(input);
  std::string inputExt  = getFilenameExt(input);
  if (inputExt == "gz")
  {
    inputExt  = getFilenameExt(inputBase) + ".gz";
    inputBase = getFilenameBase(inputBase);
  }

  if (inputExt == "gltf")
  {
    if (output.empty())
      output = inputBase + ".rkscene";

    Log() << "Loading glTF: " << input;
    auto scene = rkscene::loadGLTF(input);
    Log() << "Saving rkscene: " << output;

    rkscene::save(*scene, output);
  }
  else if (inputExt == "obj" || inputExt == "obj.gz")
  {
    if (output.empty())
    output = inputBase + ".mesh";

    TriangleMesh mesh;
    TriangleSoup soup;

    ObjLoader objLoader;
    objLoader.load(input, soup, inputExt == "obj.gz");

    // Remap material IDs (lights first)
    if (!soup.materialNames.isEmpty())
    {
      std::string mtlPath = inputBase + ".mtl";
      PropsMap matLib;
      if (File::exists(mtlPath) && !convertProps.exists("no-mtl"))
      {
        MtlLoader(mtlPath, matLib);
        Log() << "Remapping material IDs";

        Array<bool> lightFlags;

        // Check which materials are lights
        for (const std::string& matName : soup.materialNames)
        {
          auto mat = matLib.find(matName);
          if (mat != matLib.end())
          {
            lightFlags.pushBack(MaterialFactory::isEmissive(mat->second));
          }
          else
          {
            // Unknown material, not a light
            lightFlags.pushBack(false);
          }
        }

        lightFlags[0] = true; // first material is the environment

        Array<int> matIdMap;
        int lightCount = 0;
        for (int i = 0; i < lightFlags.getSize(); ++i)
        {
          if (lightFlags[i])
          {
            swap(soup.materialNames[i], soup.materialNames[lightCount]);
            matIdMap.pushBack(lightCount);
            matIdMap[lightCount] = i;
            lightCount++;
          }
          else
          {
            matIdMap.pushBack(i);
          }
        }

        // Remap
        for (FatTriangle& tri : soup.triangles)
        {
          if (tri.matId < 0 || tri.matId >= matIdMap.getSize())
            throw std::logic_error("invalid matId");
          tri.matId = matIdMap[tri.matId];
        }
      }
    }

    TriangleMeshBuilder(soup, mesh);
    saveBlob(output, mesh);
  }
  else
  {
    LogError() << "Unsupported input format";
    return 1;
  }

  return 0;
}

} // namespace prt
