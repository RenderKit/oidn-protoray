// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifdef PRT_IMAGE_SUPPORT
#include <OpenImageIO/imageio.h>
OIIO_NAMESPACE_USING
#endif

#include "sys/array.h"
#include "sys/logging.h"
#include "sys/filesystem.h"
#include "math/vec3.h"
#include "math/vec4.h"
#include "image_io.h"

namespace prt {

struct ImageIo::HandleSt
{
#ifdef PRT_IMAGE_SUPPORT
  std::unique_ptr<ImageInput> in;
#endif
  ImageIo::Desc desc;
};

ImageIo::Handle ImageIo::open(const std::string& filename)
{
#ifdef PRT_IMAGE_SUPPORT
  Log() << "Loading image: " << filename;

  std::unique_ptr<ImageInput> in = ImageInput::open(filename.c_str());
  if (!in)
  {
    LogError() << "Could not load image";
    return 0;
  }

  PixelFormat format;
  const ImageSpec& spec = in->spec();

  if (spec.format.size() == 1)
  {
    switch (spec.nchannels)
    {
    case 1:
      format = PixelFormat::R8;
      break;

    case 3:
      format = PixelFormat::Bgrx8;
      break;

    default:
      format = PixelFormat::Bgra8;
      break;
    }
  }
  else
  {
    format = PixelFormat::Rgb32f;
  }

  Handle h = new HandleSt;
  h->in = std::move(in);
  h->desc.size = Vec2i(spec.width, spec.height);
  h->desc.format = format;
  return h;
#endif

  LogError() << "Unsupported image file format";
  return 0;
}

ImageIo::Desc ImageIo::getDesc(Handle h)
{
  return h->desc;
}

void ImageIo::getData(Handle h, void* data)
{
#ifdef PRT_IMAGE_SUPPORT
  int width  = h->desc.size.x;
  int height = h->desc.size.y;
  const ImageSpec& spec = h->in->spec();
  const int n = spec.nchannels;

  if (n == 1 && spec.format.size() == 1)
  {
    // R8
    h->in->read_image(0, 0, 0, n, TypeDesc::UINT8, data);
  }
  else if (n >= 2 && spec.format.size() == 1)
  {
    // BGRA8
    Array<uint8_t> src(width * height * n);
    h->in->read_image(0, 0, 0, n, TypeDesc::UINT8, src.getData());
    uint8_t* dest = (uint8_t*)data;

    for (int i = 0; i < width * height; ++i)
    {
      uint8_t r = src[i*n+0];
      uint8_t g = n >= 3 ? src[i*n+1] : r;
      uint8_t b = n >= 3 ? src[i*n+2] : r;
      uint8_t a = (n == 2 || n >= 4) ? src[i*n+(n-1)] : 0;

      dest[i*4+0] = b;
      dest[i*4+1] = g;
      dest[i*4+2] = r;
      dest[i*4+3] = a;
    }
  }
  else if (spec.format.size() > 1)
  {
    // RGB32F
    Array<float> src(width * height * n);
    h->in->read_image(0, 0, 0, n, TypeDesc::FLOAT, src.getData());
    float* dest = (float*)data;

    for (int i = 0; i < width * height; ++i)
    {
      float r = src[i*n+0];
      float g = n >= 3 ? src[i*n+1] : r;
      float b = n >= 3 ? src[i*n+2] : r;

      dest[i*3+0] = r;
      dest[i*3+1] = g;
      dest[i*3+2] = b;
    }
  }
#endif
}

void ImageIo::close(Handle h)
{
#ifdef PRT_IMAGE_SUPPORT
  h->in->close();
#endif
  delete h;
}

bool ImageIo::save(const std::string& filename, const ImageIo::Desc& desc, const void* data)
{
  Log() << "Saving image: " << filename;

  // Always use our own PPM writer
  if (desc.format == PixelFormat::Bgrx8 && getFilenameExt(filename) == "ppm")
  {
    FILE* file = fopen(filename.c_str(), "wb");
    if (file == 0)
    {
      LogError() << "Could not save image";
      return false;
    }

    fprintf(file, "P6\n%d %d\n255\n", desc.size.x, desc.size.y);

    for (int y = 0; y < desc.size.y; ++y)
    {
      const Vec4uc* srcLine = (const Vec4uc*)data + y * desc.size.x;

      for (int x = 0; x < desc.size.x; ++x)
      {
        fputc(srcLine[x].x, file);
        fputc(srcLine[x].y, file);
        fputc(srcLine[x].z, file);
      }
    }

    fclose(file);
    return true;
  }

#ifdef PRT_IMAGE_SUPPORT
  int n;
  TypeDesc type;

  switch (desc.format)
  {
  case PixelFormat::Bgrx8:
    n = 3;
    type = TypeDesc::UINT8;
    break;

  case PixelFormat::Bgra8:
    n = 4;
    type = TypeDesc::UINT8;
    break;

  case PixelFormat::Rg8:
    n = 2;
    type = TypeDesc::UINT8;
    break;

  case PixelFormat::R8:
    n = 1;
    type = TypeDesc::UINT8;
    break;

  case PixelFormat::Rgb16f:
    n = 3;
    type = TypeDesc::HALF;
    break;

  case PixelFormat::Rg16f:
    n = 2;
    type = TypeDesc::HALF;
    break;

  case PixelFormat::R16f:
    n = 1;
    type = TypeDesc::HALF;
    break;

  case PixelFormat::Rgb32f:
    n = 3;
    type = TypeDesc::FLOAT;
    break;

  case PixelFormat::Rg32f:
    n = 2;
    type = TypeDesc::FLOAT;
    break;

  case PixelFormat::R32f:
    n = 1;
    type = TypeDesc::FLOAT;
    break;

  default:
    LogError() << "Unsupported pixel format";
    return false;
  }

  std::unique_ptr<ImageOutput> out(ImageOutput::create(filename.c_str()));
  if (!out)
  {
    LogError() << "Unsupported image file format";
    return false;
  }

  ImageSpec spec(desc.size.x, desc.size.y, n, type);
  if (n == 1)
    spec.channelnames[0] = "Y";
  if (type == TypeDesc::HALF || type == TypeDesc::FLOAT)
    spec.attribute("compression", "zip:9");

  if (!out->open(filename.c_str(), spec))
  {
    LogError() << "Could not create image file";
    return false;
  }

  bool success = true;

  if (desc.format == PixelFormat::Bgrx8 || desc.format == PixelFormat::Bgra8)
  {
    const uint8_t* src = (const uint8_t*)data;
    Array<uint8_t> dest(desc.size.x * desc.size.y * n);

    for (int i = 0; i < desc.size.x * desc.size.y; ++i)
    {
      uint8_t b = src[i*4+0];
      uint8_t g = src[i*4+1];
      uint8_t r = src[i*4+2];

      dest[i*n+0] = r;
      dest[i*n+1] = g;
      dest[i*n+2] = b;

      if (n == 4)
      {
        uint8_t a = src[i*4+3];
        dest[i*n+3] = a;
      }
    }

    success = out->write_image(type, dest.getData());
  }
  else
  {
    success = out->write_image(type, data);
  }

  if (!success)
  {
    LogError() << "Could not write pixels: " << out->geterror();
    return false;
  }

  if (!out->close())
  {
    LogError() << "Error closing image file: " << out->geterror();
    return false;
  }

  return true;
#else
  // Only PPM is supported
  LogError() << "Unsupported image file format";
  return false;
#endif
}

} // namespace prt
