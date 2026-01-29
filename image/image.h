// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/memory.h"
#include "sys/tasking.h"
#include "sys/logging.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"

namespace prt {

template <class T>
class Image
{
private:
  T* data;
  Vec2i size;

public:
  prt_inline Image() : data(0), size(0) {}

  explicit Image(const Vec2i& size)
  {
    init(size);
  }

  Image(int width, int height)
  {
    init(Vec2i(width, height));
  }

  Image(const Image& other)
  {
    init(other.size);
    memcpy(data, other.data, size.x * size.y * sizeof(T));
  }

  Image& operator =(const Image& other)
  {
    alloc(other.size);
    memcpy(data, other.data, size.x * size.y * sizeof(T));
    return *this;
  }

  ~Image()
  {
    free();
  }

  void alloc(const Vec2i& size)
  {
    free();
    init(size);
  }

  void alloc(int width, int height)
  {
    alloc(Vec2i(width, height));
  }

  void free()
  {
    alignedFree(data);
    data = nullptr;
    size = zero;
  }

  void clear()
  {
    tbb::parallel_for(tbb::blocked_range<int>(0, size.x*size.y), [&](const tbb::blocked_range<int>& r)
    {
      for (int i = r.begin(); i != r.end(); ++i)
        data[i] = zero;
    });
  }

  prt_inline operator bool() const
  {
    return data != nullptr;
  }

  prt_inline const T* operator [](int y) const
  {
    assert(y >= 0 && y < size.y);
    return data + y * size.x;
  }

  prt_inline T* operator [](int y)
  {
    assert(y >= 0 && y < size.y);
    return data + y * size.x;
  }

  prt_inline const T* getData() const
  {
    return data;
  }

  prt_inline T* getData()
  {
    return data;
  }

  prt_inline Vec2i getSize() const
  {
    return size;
  }

  prt_inline int getWidth() const
  {
    return size.x;
  }

  prt_inline int getHeight() const
  {
    return size.y;
  }

private:
  void init(const Vec2i& size)
  {
    data = (T*)alignedAlloc(size.x * size.y * sizeof(T));
    this->size = size;
  }
};

typedef Image<Vec4uc> Image4uc; // BGRA uint8
typedef Image<Vec3f> Image3f;   // RGB float
typedef Image<Vec2f> Image2f;   // XY float
typedef Image<float> Image1f;   // float

bool loadImage(const std::string& filename, Image4uc& image);
bool loadImage(const std::string& filename, Image3f& image);

bool saveImage(const std::string& filename, const Image4uc& image);
bool saveImage(const std::string& filename, const Image<int>& image);
bool saveImage(const std::string& filename, const Image3f& image, float scale = 1.f, int fp = 16);
bool saveImage(const std::string& filename, const Image2f& image, float scale = 1.f, int fp = 16);
bool saveImage(const std::string& filename, const Image1f& image, float scale = 1.f, int fp = 16);

// Try to save the image multiple times if it fails (e.g. due to network error)
template <typename... Args>
inline bool saveImageSafe(Args... args)
{
  for (int i = 0; i < 3; ++i)
  {
    if (saveImage(args...))
      return true;
  }

  LogError() << "Failed to save image";
  return false;
}

} // namespace prt
