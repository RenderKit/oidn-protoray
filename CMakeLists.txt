## Copyright 2015 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.16)

project(protoray)

# Policy: find_package() uses <PackageName>_ROOT variables
if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()

# Modules
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

# Compiler
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# CMake sets MSVC for every compiler targeting the MSVC ABI, but only cl.exe and
# clang-cl take MSVC-style flags: clang++/DPC++ on Windows wants GNU-style ones.
if(MSVC AND NOT CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU")
  set(PRT_MSVC_CLI TRUE)
else()
  set(PRT_MSVC_CLI FALSE)
endif()

if(PRT_MSVC_CLI)
  # MSVC brings its own optimization defaults, so only add what the code needs
  # on top of them.
  add_compile_options(
    /bigobj      # the template-heavy renderers blow past the default section limit
    /permissive- # conformance mode; the code is written against GCC/Clang
    /Zc:__cplusplus
    /utf-8
    /wd4068      # unknown pragma (#pragma unroll / #pragma nounroll)
  )
  # CMake's MSVC Debug config defines _DEBUG, but the code tests for DEBUG
  add_compile_definitions($<$<CONFIG:Debug>:DEBUG>)
else()
  # Append rather than replace. On Windows these defaults carry clang's C runtime
  # selection (-D_DEBUG -D_DLL -D_MT -Xclang --dependent-lib=msvcrtd); dropping it
  # leaves the link with no CRT at all.
  string(APPEND CMAKE_CXX_FLAGS_RELEASE " -O3")
  string(APPEND CMAKE_CXX_FLAGS_RELWITHDEBINFO " -g -O3")
  string(APPEND CMAKE_CXX_FLAGS_DEBUG " -g -O0 -DDEBUG")
endif()

if(WIN32)
  # These apply to every Windows compiler, not just cl.exe
  add_compile_definitions(
    # Windows headers are pulled in by <Windows.h>, Embree, SDL2 and OpenImageIO
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    # The CRT's _s replacements for fopen/sscanf/strcpy/... are not portable
    _CRT_SECURE_NO_WARNINGS
  )
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64|ARM64")
  set(HOST_ISA_DEFAULT "NEON")
else()
  set(HOST_ISA_DEFAULT "AVX2")
endif()

set(HOST_ISA "${HOST_ISA_DEFAULT}" CACHE STRING "Instruction set.")
set_property(CACHE HOST_ISA PROPERTY STRINGS "AVX2" "AVX512" "NEON")
if(HOST_ISA STREQUAL "NEON")
  set(HOST_CXX_FLAGS "") # NEON is enabled by default on AArch64
elseif(HOST_ISA STREQUAL "AVX2")
  if(PRT_MSVC_CLI)
    set(HOST_CXX_FLAGS "/arch:AVX2") # implies F16C, FMA, LZCNT, BMI1/2; MSVC gates no intrinsic on RDSEED
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    set(HOST_CXX_FLAGS "-xCORE-AVX2")
  else()
    set(HOST_CXX_FLAGS "-mf16c -mavx2 -mfma -mlzcnt -mbmi -mbmi2 -mrdseed") # AVX2 + RDSEED
  endif()
elseif(HOST_ISA STREQUAL "AVX512")
  if(PRT_MSVC_CLI)
    set(HOST_CXX_FLAGS "/arch:AVX512")
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    set(HOST_CXX_FLAGS "-xCORE-AVX512")
  else()
    set(HOST_CXX_FLAGS "-mavx512f -mavx512dq -mavx512cd -mavx512bw -mavx512vl -mf16c -mavx2 -mfma -mlzcnt -mbmi -mbmi2 -mrdseed")
  endif()
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

include_directories(${PROJECT_SOURCE_DIR})

# TBB
# oneTBB ships its own config (and vcpkg only ships that); the bundled FindTBB
# module is the fallback for installations that predate it.
find_package(TBB CONFIG QUIET)
if(NOT TBB_FOUND)
  find_package(TBB MODULE REQUIRED)
endif()

# Embree
# embree_ROOT has to be turned into a prefix path by hand: vcpkg's toolchain wraps
# find_package() in a macro whose policy scope has CMP0074 unset, so the ROOT
# variable would otherwise be silently ignored.
if(embree_ROOT)
  list(APPEND CMAKE_PREFIX_PATH "${embree_ROOT}")
elseif(DEFINED ENV{embree_ROOT})
  list(APPEND CMAKE_PREFIX_PATH "$ENV{embree_ROOT}")
endif()

find_package(embree 4 QUIET)
if(NOT embree_FOUND)
  # Nothing installed system-wide. Fall back to an official binary release unpacked
  # next to the source tree: those are named embree-<version>.<arch>.<os> on every
  # platform, e.g. ../embree-4.4.1.x64.windows or ../embree-4.4.1.x86_64.linux.
  file(GLOB PRT_EMBREE_DIRS "${PROJECT_SOURCE_DIR}/../embree-*")
  if(PRT_EMBREE_DIRS)
    list(SORT PRT_EMBREE_DIRS COMPARE NATURAL ORDER DESCENDING) # newest version first
    foreach(PRT_EMBREE_DIR ${PRT_EMBREE_DIRS})
      if(IS_DIRECTORY "${PRT_EMBREE_DIR}")
        list(APPEND CMAKE_PREFIX_PATH "${PRT_EMBREE_DIR}")
      endif()
    endforeach()
  endif()
  find_package(embree 4 REQUIRED)
endif()
message(STATUS "Found Embree ${EMBREE_VERSION}: ${embree_DIR}")

# OpenImageDenoise
option(PRT_OIDN_SUPPORT "OpenImageDenoise support" OFF)
if(PRT_OIDN_SUPPORT)
  add_compile_definitions(PRT_OIDN_SUPPORT)
  find_package(OpenImageDenoise 2 REQUIRED)
endif()

# Image I/O
option(PRT_IMAGE_SUPPORT "Image support" ON)
if(PRT_IMAGE_SUPPORT)
  add_compile_definitions(PRT_IMAGE_SUPPORT)
  find_package(OpenImageIO CONFIG REQUIRED)
endif()

# Convert
option(PRT_CONVERT_SUPPORT "Scene format conversion support" ON)
if(PRT_CONVERT_SUPPORT)
  find_package(Boost CONFIG COMPONENTS iostreams)
  if(NOT Boost_FOUND)
    find_package(Boost REQUIRED COMPONENTS iostreams)
  endif()
  add_compile_definitions(PRT_CONVERT_SUPPORT)
endif()

# GUI
option(PRT_GUI_SUPPORT "GUI support" ON)
if(PRT_GUI_SUPPORT)
  add_compile_definitions(PRT_GUI_SUPPORT)
endif()

if(APPLE)
  include_directories(/opt/local/include)
  link_directories(/opt/local/lib)
endif()

option(PRT_PROFILE_MODE "Performance counters" OFF)
if(PRT_PROFILE_MODE)
  add_compile_definitions(PRT_PROFILE_MODE)
endif()

option(PRT_MUTATION_SUPPORT "Material mutation support" ON)
if(PRT_MUTATION_SUPPORT)
  add_compile_definitions(PRT_MUTATION_SUPPORT)
endif()

# Other switches
#set(HOST_CXX_FLAGS "${HOST_CXX_FLAGS} -S -masm=intel") # generate host asm

# Find git version
set(PRT_HASH 0)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git)
  find_package(Git)
  if(GIT_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      OUTPUT_VARIABLE "PRT_HASH"
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()
endif()

# Generate config.h into the build tree, so that the source tree stays clean and
# separate build directories do not overwrite each other's copy
configure_file(
  "${PROJECT_SOURCE_DIR}/main/config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/main/config.h"
)
# BEFORE, so that a stale main/config.h left in the source tree by an older build
# cannot shadow the generated one
include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR})

set(SOURCES
  accel/embree_intersector.cpp
  accel/embree_intersector.h
  accel/embree_intersector_single.h
  accel/embree_intersector_packet.h
  accel/embree_intersector_stream.h
  bsdf/albedo.h
  bsdf/albedo_tables.cpp
  bsdf/albedo_tables.h
  bsdf/bsdf.h
  bsdf/multi_bsdf.h
  bsdf/optics.h
  bsdf/diffuse.h
  bsdf/rough_diffuse.h
  bsdf/microfacet.h
  bsdf/ggx_distribution.h
  bsdf/beckmann_distribution.h
  bsdf/sheen_distribution.h
  bsdf/conductor.h
  bsdf/rough_conductor.h
  bsdf/dielectric.h
  bsdf/rough_dielectric.h
  bsdf/thin_dielectric.h
  bsdf/thin_rough_dielectric.h
  bsdf/coating.h
  bsdf/rough_coating.h
  bsdf/sheen.h
  bsdf/transparent.h
  camera/camera.h
  camera/pinhole_camera.h
  camera/thin_lens_camera.h
  sys/array.h
  sys/atomic.h
  sys/blob.h
  sys/common.h
  sys/condition.h
  sys/constants.h
  sys/file.cpp
  sys/file.h
  sys/filesystem.cpp
  sys/filesystem.h
  sys/fixed_hash_map.h
  sys/lock_guard.h
  sys/logging.cpp
  sys/logging.h
  sys/mapped_file.cpp
  sys/mapped_file.h
  sys/memory.cpp
  sys/memory.h
  sys/mutex.h
  sys/option.cpp
  sys/option.h
  sys/props.h
  sys/raw_array.h
  sys/static_array.h
  sys/stream.h
  sys/string.cpp
  sys/string.h
  sys/sysinfo.cpp
  sys/sysinfo.h
  sys/tasking.cpp
  sys/tasking.h
  sys/text_reader.cpp
  sys/text_reader.h
  sys/tick_counter.h
  sys/timer.h
  sys/value.h
  render/device.h
  render/device_cpu.cpp
  render/device_cpu.h
  render/integrator.h
  render/primary_integrator_single.h
  render/primary_integrator_packet.h
  render/ao_integrator_single.h
  render/ao_integrator_packet.h
  render/ao_hit_integrator_single.h
  render/po_integrator_single.h
  render/po_integrator_packet.h
  render/diffuse_integrator_single.h
  render/diffuse_integrator_packet.h
  render/diffuse2_integrator_single.h
  render/diffuse2_integrator_packet.h
  render/path_integrator_single.h
  render/path_integrator_packet.h
  render/debug_integrator_single.h
  render/renderer.h
  render/renderer_single.h
  render/renderer_packet.h
  render/renderer_stream.h
  render/primary_renderer_stream.h
  render/diffuse_renderer_stream.h
  render/diffuse2_renderer_stream.h
  render/ao_renderer_stream.h
  render/path_renderer_stream.h
  render/intersector_factory_single.cpp
  render/intersector_factory_single.h
  render/intersector_factory_packet.cpp
  render/intersector_factory_packet.h
  render/intersector_factory_stream.h
  render/renderer_factory.cpp
  render/renderer_factory.h
  render/renderer_factory_single.cpp
  render/renderer_factory_single.h
  render/renderer_factory_packet.cpp
  render/renderer_factory_packet.h
  render/renderer_factory_stream.cpp
  render/renderer_factory_stream.h
  render/scene.cpp
  render/scene.h
  render/transparent_filter.h
  geometry/geometry.h
  geometry/group.h
  geometry/instance.h
  geometry/triangle.cpp
  geometry/triangle.h
  geometry/triangle_mesh.cpp
  geometry/triangle_mesh.h
  geometry/triangle_mesh_builder.cpp
  geometry/triangle_mesh_builder.h
  geometry/triangle_soup.h
  geometry/quad.h
  geometry/vertex.h
  image/accum_buffer.h
  image/bump.cpp
  image/bump.h
  image/color.cpp
  image/color.h
  image/denoiser.h
  image/frame_buffer.cpp
  image/frame_buffer.h
  image/image.cpp
  image/image.h
  image/image_io.cpp
  image/image_io.h
  image/morton.cpp
  image/morton.h
  image/pixel.cpp
  image/pixel.h
  image/pixel_filter.h
  image/texture.h
  image/image_texture.cpp
  image/image_texture.h
  image/tone_mapper.h
  image/linear_tone_mapper.h
  image/reinhard_tone_mapper.h
  image/aces_tone_mapper.h
  image/filmic_tone_mapper.h
  image/log_tone_mapper.h
  light/light.h
  light/ambient_light.h
  light/distant_light.h
  light/image_light.h
  light/triangle_light.h
  light/sun_sky_light.cpp
  light/sun_sky_light.h
  light/sky_model/sky_model.cpp
  light/sky_model/sky_model.h
  light/sky_model/sky_model_data_ciexyz.h
  light/sky_model/sky_model_data_rgb.h
  light/sky_model/sky_model_data_spectral.h
  ${CMAKE_CURRENT_BINARY_DIR}/main/config.h
  main/main.h
  main/main.cpp
  main/render/input.h
  main/render/render.cpp
  main/render/render_window.cpp
  main/render/render_window.h
  main/render/stats_recorder.cpp
  main/render/stats_recorder.h
  main/render/view.cpp
  main/render/view.h
  main/render/window.cpp
  main/render/window.h
  main/sandbox/sandbox.cpp
  material/luminous.h
  material/material_factory.cpp
  material/material_factory.h
  material/material_param.h
  material/material.cpp
  material/material.h
  material/medium.h
  material/principled.cpp
  material/principled.h
  material/standard.cpp
  material/standard.h
  material/flakes.h
  material/car_paint.cpp
  material/car_paint.h
  material/marble.cpp
  material/marble.h
  material/mtl_loader.cpp
  material/mtl_loader.h
  material/mtl_writer.cpp
  material/mtl_writer.h
  math/basis3.h
  math/box1.h
  math/box2.h
  math/box3.h
  math/fast_box3.h
  math/fast_random.h
  math/fast_random_simd.h
  math/hash.h
  math/mat2.h
  math/mat3.h
  math/mat4.h
  math/math_common.h
  math/math.h
  math/math_avx2.h
  math/math_neon.h
  math/random.h
  math/simd_common.cpp
  math/simd_common.h
  math/simd.h
  math/vec2.h
  math/vec3.h
  math/vec4.h
  math/interpolation.h
  math/integration.h
  math/noise.h
  math/simd/vfloat16_avx512.h
  math/simd/vfloat4_avx.h
  math/simd/vfloat8_avx2.h
  math/simd/vint16_avx512.h
  math/simd/vint4_avx.h
  math/simd/vint8_avx2.h
  math/simd/vuint16_avx512.h
  math/simd/vuint8_avx2.h
  math/simd/vuint4_avx.h
  math/simd/vbool16_avx512.h
  math/simd/vbool4_avx.h
  math/simd/vbool8_avx.h
  math/simd/vfloat4_neon.h
  math/simd/vfloat8_neon.h
  math/simd/vint4_neon.h
  math/simd/vint8_neon.h
  math/simd/vuint4_neon.h
  math/simd/vuint8_neon.h
  math/simd/vbool4_neon.h
  math/simd/vbool8_neon.h
  core/intersector.h
  core/intersector_single.h
  core/intersector_packet.h
  core/intersector_stream.h
  core/ray.h
  core/ray_simd.h
  core/ray_stream.h
  core/ray_embree.h
  core/ray_stats.h
  core/shading_context.h
  sampling/shape_sampler.h
  sampling/distribution1d.cpp
  sampling/distribution1d.h
  sampling/distribution2d.cpp
  sampling/distribution2d.h
  sampling/random_sampler.h
  sampling/random_sampler_simd.h
  sampling/sobol_burley.cpp
  sampling/sobol_burley.h
  sampling/sobol_sampler.h
  sampling/sobol_sampler_simd.h
  sampling/sobol.cpp
  sampling/sobol.h
)

if(PRT_CONVERT_SUPPORT)
  set(SOURCES
    ${SOURCES}
    main/convert/convert.cpp
    main/convert/obj_loader.cpp
    main/convert/obj_loader.h
  )
endif()

if(PRT_OIDN_SUPPORT)
  set(SOURCES
    ${SOURCES}
    image/oidn_denoiser.cpp
    image/oidn_denoiser.h
  )
endif()

add_executable(protoray ${SOURCES})
if(HOST_CXX_FLAGS)
  separate_arguments(HOST_CXX_FLAGS_LIST NATIVE_COMMAND "${HOST_CXX_FLAGS}")
  target_compile_options(protoray PRIVATE ${HOST_CXX_FLAGS_LIST})
endif()
if(UNIX AND NOT APPLE)
  target_link_libraries(protoray rt)
endif()

# TBB
target_link_libraries(protoray TBB::tbb)

# Embree
target_link_libraries(protoray embree)

# OpenImageDenoise
if(PRT_OIDN_SUPPORT)
  target_link_libraries(protoray OpenImageDenoise)
endif()

# Image I/O
if(PRT_IMAGE_SUPPORT)
  target_link_libraries(protoray OpenImageIO::OpenImageIO)
endif()

# Convert
if(PRT_CONVERT_SUPPORT)
  if(TARGET Boost::iostreams)
    target_link_libraries(protoray Boost::iostreams)
  else()
    target_link_libraries(protoray ${Boost_LIBRARIES})
  endif()
endif()

# GUI
if(PRT_GUI_SUPPORT)
  find_package(SDL2 REQUIRED)
  if(TARGET SDL2::SDL2)
    # Deliberately not SDL2::SDL2main: we provide our own main() (SDL_MAIN_HANDLED)
    target_link_libraries(protoray SDL2::SDL2)
  else()
    target_include_directories(protoray PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(protoray ${SDL2_LIBRARIES})
  endif()
endif()

# Runtime dependencies
if(WIN32)
  # Windows has no rpath, so the Embree runtime has to sit next to the executable.
  # (vcpkg copies the dependencies it owns itself.)
  add_custom_command(TARGET protoray POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${EMBREE_ROOT_DIR}/bin/embree4.dll" "$<TARGET_FILE_DIR:protoray>"
    VERBATIM)
endif()
