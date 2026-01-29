## Copyright 2015 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.10)

project(protoray)

# Policy: find_package() uses <PackageName>_ROOT variables
if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()

# Modules
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

# Compiler
set(CMAKE_CXX_FLAGS "-std=c++17")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g ${CMAKE_CXX_FLAGS_RELEASE}")

set(HOST_ISA "AVX2" CACHE STRING "Instruction set.")
set_property(CACHE HOST_ISA PROPERTY STRINGS "AVX2" "AVX512")
if(HOST_ISA STREQUAL "AVX2")
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    set(HOST_CXX_FLAGS "-xCORE-AVX2")
  else()
    set(HOST_CXX_FLAGS "-mf16c -mavx2 -mfma -mlzcnt -mbmi -mbmi2 -mrdseed") # AVX2 + RDSEED
  endif()
elseif(HOST_ISA STREQUAL "AVX512")
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
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
find_package(TBB REQUIRED)

# Embree
find_package(embree 4 REQUIRED)

# OpenImageDenoise
option(PRT_OIDN_SUPPORT "OpenImageDenoise support" OFF)
if(PRT_OIDN_SUPPORT)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPRT_OIDN_SUPPORT")
  find_package(OpenImageDenoise 2 REQUIRED)
endif()

# Image I/O
option(PRT_IMAGE_SUPPORT "Image support" ON)
if(PRT_IMAGE_SUPPORT)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPRT_IMAGE_SUPPORT")
  find_package(OpenImageIO CONFIG REQUIRED)
endif()

# Convert
option(PRT_CONVERT_SUPPORT "Scene format conversion support" ON)
if(PRT_CONVERT_SUPPORT)
  find_package(Boost CONFIG COMPONENTS iostreams)
  if(NOT Boost_FOUND)
    find_package(Boost REQUIRED COMPONENTS iostreams)
  endif()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPRT_CONVERT_SUPPORT")
endif()

# GUI
option(PRT_GUI_SUPPORT "GUI support" ON)
if(PRT_GUI_SUPPORT)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPRT_GUI_SUPPORT")
endif()

if(APPLE)
  include_directories(/opt/local/include)
  link_directories(/opt/local/lib)
endif()

option(PRT_PROFILE_MODE "Performance counters" OFF)
if(PRT_PROFILE_MODE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPRT_PROFILE_MODE")
endif()

option(PRT_MUTATION_SUPPORT "Material mutation support" ON)
if(PRT_MUTATION_SUPPORT)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPRT_MUTATION_SUPPORT")
endif()

# Other switches
#set(HOST_CXX_FLAGS "${HOST_CXX_FLAGS} -S -masm=intel") # generate host asm

# Find git version
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git)
  find_package(Git)
  if(GIT_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      OUTPUT_VARIABLE "PRT_HASH"
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE)
  else()
    set(PRT_HASH 0)
  endif()
endif()

# Generate config.h
configure_file(
  "${PROJECT_SOURCE_DIR}/main/config.h.in"
  "${PROJECT_SOURCE_DIR}/main/config.h"
)

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
  main/config.h
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
set_target_properties(protoray PROPERTIES COMPILE_FLAGS ${HOST_CXX_FLAGS})
if(NOT APPLE)
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
  target_link_libraries(protoray ${Boost_LIBRARIES})
endif()

# GUI
if(PRT_GUI_SUPPORT)
  find_package(SDL2 REQUIRED)
  include_directories(protoray ${SDL2_INCLUDE_DIRS})
  target_link_libraries(protoray ${SDL2_LIBRARIES})
endif()
