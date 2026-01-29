// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "sys/array.h"
#include "sys/logging.h"
#include "image_io.h"
#include "image.h"

namespace prt {

bool loadImage(const std::string& filename, Image4uc& image)
{
  ImageIo::Handle handle = ImageIo::open(filename);
  if (!handle)
    return false;

  ImageIo::Desc desc = ImageIo::getDesc(handle);
  if (desc.format != PixelFormat::Bgrx8 && desc.format != PixelFormat::Bgra8)
  {
    ImageIo::close(handle);
    LogError() << "Pixel format mismatch";
    return false;
  }

  image.alloc(desc.size);
  ImageIo::getData(handle, image.getData());
  ImageIo::close(handle);
  return true;
}

bool loadImage(const std::string& filename, Image3f& image)
{
  ImageIo::Handle handle = ImageIo::open(filename);
  if (!handle)
    return false;

  ImageIo::Desc desc = ImageIo::getDesc(handle);
  if (desc.format != PixelFormat::Rgb32f)
  {
    ImageIo::close(handle);
    LogError() << "Pixel format mismatch";
    return false;
  }

  image.alloc(desc.size);
  ImageIo::getData(handle, image.getData());
  ImageIo::close(handle);
  return true;
}

bool saveImage(const std::string& filename, const Image4uc& image)
{
  if (filename.find(".") == std::string::npos)
#ifdef PRT_IMAGE_SUPPORT
    return saveImage(filename + ".png", image);
#else
    return saveImage(filename + ".ppm", image);
#endif

  ImageIo::Desc desc;
  desc.size = image.getSize();
  desc.format = PixelFormat::Bgrx8;
  return ImageIo::save(filename, desc, image.getData());
}

bool saveImage(const std::string& filename, const Image<int>& image)
{
  if (filename.find(".png") == std::string::npos)
#ifdef PRT_IMAGE_SUPPORT
    return saveImage(filename + ".png", image);
#else
    return saveImage(filename + ".ppm", image);
#endif

  ImageIo::Desc desc;
  desc.size = image.getSize();
  desc.format = PixelFormat::Bgrx8;
  return ImageIo::save(filename, desc, image.getData());
}

bool saveImage(const std::string& filename, const Image3f& image, float scale, int fp)
{
  if (filename.find(".exr") == std::string::npos)
    return saveImage(filename + ".exr", image, scale, fp);

  ImageIo::Desc desc;
  desc.size = image.getSize();

  const float* src = (const float*)image.getData();

  if (fp == 32)
  {
    Array<float> dest(image.getWidth() * image.getHeight() * 3);
    for (int i = 0; i < dest.getSize(); ++i)
      dest[i] = src[i] * scale;

    desc.format = PixelFormat::Rgb32f;
    return ImageIo::save(filename, desc, dest.getData());
  }
  else if (fp == 16)
  {
    // Convert to FP16
    Array<uint16_t> dest(image.getWidth() * image.getHeight() * 3);
    for (int i = 0; i < dest.getSize(); ++i)
      dest[i] = toHalf(src[i] * scale);

    desc.format = PixelFormat::Rgb16f;
    return ImageIo::save(filename, desc, dest.getData());
  }
  else
  {
    throw std::invalid_argument("fp must be 16 or 32");
  }
}

bool saveImage(const std::string& filename, const Image2f& image, float scale, int fp)
{
  if (filename.find(".exr") == std::string::npos)
    return saveImage(filename + ".exr", image, scale, fp);

  ImageIo::Desc desc;
  desc.size = image.getSize();

  const float* src = (const float*)image.getData();

  if (fp == 32)
  {
    Array<float> dest(image.getWidth() * image.getHeight() * 2);
    for (int i = 0; i < dest.getSize(); ++i)
      dest[i] = src[i] * scale;

    desc.format = PixelFormat::Rg32f;
    return ImageIo::save(filename, desc, dest.getData());
  }
  else if (fp == 16)
  {
    // Convert to FP16
    Array<uint16_t> dest(image.getWidth() * image.getHeight() * 2);
    for (int i = 0; i < dest.getSize(); ++i)
      dest[i] = toHalf(src[i] * scale);

    desc.format = PixelFormat::Rg16f;
    return ImageIo::save(filename, desc, dest.getData());
  }
  else
  {
    throw std::invalid_argument("fp must be 16 or 32");
  }
}

bool saveImage(const std::string& filename, const Image1f& image, float scale, int fp)
{
  if (filename.find(".exr") == std::string::npos)
    return saveImage(filename + ".exr", image, scale, fp);

  ImageIo::Desc desc;
  desc.size = image.getSize();

  const float* src = image.getData();

  if (fp == 32)
  {
    Array<float> dest(image.getWidth() * image.getHeight());
    for (int i = 0; i < dest.getSize(); ++i)
      dest[i] = src[i] * scale;

    desc.format = PixelFormat::R32f;
    return ImageIo::save(filename, desc, dest.getData());
  }
  else if (fp == 16)
  {
    // Convert to FP16
    Array<uint16_t> dest(image.getWidth() * image.getHeight());
    for (int i = 0; i < dest.getSize(); ++i)
      dest[i] = toHalf(src[i] * scale);

    desc.format = PixelFormat::R16f;
    return ImageIo::save(filename, desc, dest.getData());
  }
  else
  {
    throw std::invalid_argument("fp must be 16 or 32");
  }
}

} // namespace prt
