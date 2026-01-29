// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <iostream>
#include "sys/string.h"
#include "sys/logging.h"
#include "sys/sysinfo.h"
#include "image/image_io.h"
#include "main.h"

namespace prt {

namespace
{
  std::string binPath;

  void printHeader()
  {
    std::cout << "ProtoRay " << std::string(PRT_HASH).substr(0, 8) << std::endl;
    std::cout << std::endl;
  }
}

} // namespace prt

int main(int argc, char* argv[])
{
  using namespace prt;

  printHeader();
  if (argc == 1)
  {
  #ifdef PRT_CONVERT_SUPPORT
    std::cout << "Commands: render, convert" << std::endl;
  #else
    std::cout << "Commands: render" << std::endl;
  #endif
    return 0;
  }

  try
  {
    initLogging();

    std::string appName = argv[1];
    binPath = argv[0];
    argv[1] = argv[0];

  #ifdef PRT_CONVERT_SUPPORT
    if (appName == "convert") return mainConvert(argc - 1, argv + 1);
  #endif
    if (appName == "render") return mainRender(argc - 1, argv + 1);
    if (appName == "sandbox") return mainSandbox(argc - 1, argv + 1);
    printHeader();
    std::cout << "Invalid command!" << std::endl;
    return 1;
  }
  catch (const std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << std::endl;
    return 2;
  }
}
