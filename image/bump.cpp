// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "sys/logging.h"
#include "sys/tasking.h"
#include "image_texture.h"
#include "color.h"
#include "bump.h"

namespace prt {

std::shared_ptr<Image<int>> bumpToNormalMap(const std::shared_ptr<ImageTexture>& bump)
{
  Log() << "Converting bump to normal map";

  Vec2i size = bump->getSize();
  std::shared_ptr<Image<int>> normal = std::make_shared<Image<int>>(size);

  tbb::parallel_for(tbb::blocked_range<int>(0, size.y), [&](const tbb::blocked_range<int>& r)
  {
    for (int y = r.begin(); y != r.end(); ++y)
    {
      for (int x = 0; x < size.x; ++x)
      {
        // Sobel operator
        float f00 = average(bump->get3f(Vec2i(x-1,y-1)));
        float f10 = average(bump->get3f(Vec2i(x  ,y-1)));
        float f20 = average(bump->get3f(Vec2i(x+1,y-1)));

        float f01 = average(bump->get3f(Vec2i(x-1,y  )));
        float f21 = average(bump->get3f(Vec2i(x+1,y  )));

        float f02 = average(bump->get3f(Vec2i(x-1,y+1)));
        float f12 = average(bump->get3f(Vec2i(x  ,y+1)));
        float f22 = average(bump->get3f(Vec2i(x+1,y+1)));

        float dx = f20-f00 + 2.0f*(f21-f01) + f22-f02;
        float dy = f02-f00 + 2.0f*(f12-f10) + f22-f20;

        Vec3f N = normalize(Vec3f(-dx, -dy, 1.0f));

        (*normal)[y][x] = encodeBgr8Linear((N + 1.0f) * 0.5f);
      }
    }
  });

  return normal;
}

} // namespace prt
