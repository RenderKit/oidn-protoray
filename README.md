# Intel® Open Image Denoise – ProtoRay

This is a sample ray tracing application for demonstrating the usage of Intel
Open Image Denoise and for rendering training/validation datasets.


Compilation
-----------

Requirements:

- CPU: x86-64, ARM64

- OS: Linux, macOS

- C++17 compiler

- CMake 3.10 or newer

- [Intel® Embree](https://www.embree.org/)

- [oneAPI Threading Building Blocks (oneTBB)](https://github.com/uxlfoundation/oneTBB)

- [OpenImageIO](https://github.com/AcademySoftwareFoundation/OpenImageIO) (if
  `PRT_IMAGE_SUPPORT` CMake option is enabled)

- [Boost](https://www.boost.org/) (if `PRT_CONVERT_SUPPORT` CMake option is
  enabled)

- [Simple DirectMedia Layer (SDL) 2](https://www.libsdl.org/) (if
  `PRT_GUI_SUPPORT` CMake option is enabled)

- [Intel Open Image Denoise Library](https://github.com/RenderKit/oidn) (if
  `PRT_OIDN_SUPPORT` CMake option is enabled)