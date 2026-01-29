// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "image_io.h"
#include "image_texture.h"

namespace prt {

std::shared_ptr<ImageTextureBuffer> loadImageTexture(const std::string& filename)
{
  // Host
  ImageIo::Handle handle = ImageIo::open(filename);
  if (!handle)
    throw std::runtime_error("could not load image: " + filename);

  ImageIo::Desc desc = ImageIo::getDesc(handle);
  std::shared_ptr<ImageTextureBuffer> result;

  if (desc.format == PixelFormat::Bgrx8 || desc.format == PixelFormat::Bgra8)
  {
    std::shared_ptr<Image<int>> image = std::make_shared<Image<int>>(desc.size);
    ImageIo::getData(handle, image->getData());
    result = std::make_shared<ImageTextureBuffer>(image, desc.format);
  }
  else if (desc.format == PixelFormat::R8)
  {
    std::shared_ptr<Image<uint8_t>> image = std::make_shared<Image<uint8_t>>(desc.size);
    ImageIo::getData(handle, image->getData());
    result = std::make_shared<ImageTextureBuffer>(image, desc.format);
  }
  else if (desc.format == PixelFormat::Rgb32f)
  {
    std::shared_ptr<Image<Vec3f>> image = std::make_shared<Image<Vec3f>>(desc.size);
    ImageIo::getData(handle, image->getData());
    result = std::make_shared<ImageTextureBuffer>(image, desc.format);
  }
  else
  {
    ImageIo::close(handle);
    throw std::runtime_error("unsupported pixel format");
  }

  ImageIo::close(handle);
  return result;
}

} // namespace prt
