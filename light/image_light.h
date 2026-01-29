// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/logging.h"
#include "sys/props.h"
#include "sys/array.h"
#include "sampling/distribution2d.h"
#include "sampling/shape_sampler.h"
#include "image/image_texture.h"
#include "image/color.h"
#include "light.h"

namespace prt {

class ImageLight : public EnvironmentLight
{
private:
  std::shared_ptr<ImageTexture> color;
  Distribution2D distribution;
  float sceneRadius;

public:
  ImageLight(const std::shared_ptr<ImageTextureBuffer>& texture, const Props& props, float sceneRadius)
  {
    color = texture->getTexture(ImageTextureParams(TextureAddress::Repeat, TextureAddress::Clamp));
    Vec2i size = color->getSize();

    // TODO: better bucketing
    int bucketSize = 1;
    int maxSize = props.get("lutSize", 0);
    if (maxSize > 0)
    {
      while ((max(size.x, size.y) > maxSize) && size.x % 2 == 0 && size.y % 2 == 0)
      {
        size /= 2;
        bucketSize *= 2;
      }
    }

    Log() << "ImageLight LUT size: " << size;

    Array<float> weights(size.x * size.y);

    for (int y = 0; y < size.y; ++y)
    {
      for (int x = 0; x < size.x; ++x)
      {
        float sum = 0.0f;
        for (int y2 = 0; y2 < bucketSize; ++y2)
        {
          for (int x2 = 0; x2 < bucketSize; ++x2)
            sum += luminance(color->get3f(Vec2i(x, y) * bucketSize + Vec2i(x2, y2)));
        }

        weights[y*size.x + x] = sum * sin((y + 0.5f) / size.y * float(pi));
      }
    }

    distribution.init(weights.getData(), size);
    this->sceneRadius = sceneRadius;
  }

  Vec3f eval(const Vec3f& wi, float& pdf) const
  {
    float theta = acosSafe(wi.y);
    float phi = atan2(wi.x, -wi.z);

    float u = 0.5f + (phi * (1.0f/(2.0f*float(pi))));
    float v = 1.0f - theta * (1.0f/float(pi));

    Vec2i size = distribution.getSize();
    Vec2i i = clamp(toInt(Vec2f(u, v) * toFloat(size)), Vec2i(0), size-1);
    pdf = distribution.pdf(i) * toFloat(size.x) * toFloat(size.y) * rcp(2.0f*float(pi)*float(pi)*sin(theta));
    return color->get3f(Vec2f(u, v));
  }

  Vec3vf eval(vbool m, const Vec3vf& wi, vfloat& pdf) const
  {
    vfloat theta = acosSafe(wi.y);
    vfloat phi = atan2(wi.x, -wi.z);

    vfloat u = 0.5f + (phi * (1.0f/(2.0f*float(pi))));
    vfloat v = 1.0f - theta * (1.0f/float(pi));

    Vec2vi size = distribution.getSize();
    Vec2vi i = clamp(toInt(Vec2vf(u, v) * toFloat(size)), Vec2vi(0), size-vint(1));
    pdf = distribution.pdf(m, i) * toFloat(size.x) * toFloat(size.y) * rcp(2.0f*float(pi)*float(pi)*sin(theta));
    return color->get3f(m, Vec2vf(u, v));
  }

  Vec3f sample(const ShadingContext& ctx, Vec3f& wi, float& pdf, float& dist, const LightSample& s) const
  {
    Vec2f sv = s.v;
    Vec2i i = distribution.sampleReuse(pdf, sv);

    Vec2i size = distribution.getSize();
    float u = (toFloat(i.x) + sv.x) * rcp(toFloat(size.x));
    float v = (toFloat(i.y) + sv.y) * rcp(toFloat(size.y));

    float theta = (1.0f - v) * float(pi);
    float phi = (u - 0.5f) * (2.0f*float(pi));

    wi = Vec3f(sin(phi)*sin(theta), cos(theta), -cos(phi)*sin(theta));
    pdf *= toFloat(size.x) * toFloat(size.y) * rcp(2.0f*float(pi)*float(pi)*sin(theta));
    dist = posMax;

    return color->get3f(Vec2f(u, v)) * rcp(pdf);
  }

  Vec3vf sample(vbool m, const ShadingContextSimd& ctx, Vec3vf& wi, vfloat& pdf, vfloat& dist, const LightSampleSimd& s) const
  {
    Vec2vf sv = s.v;
    Vec2vi i = distribution.sampleReuse(m, pdf, sv);

    Vec2vi size = distribution.getSize();
    vfloat u = (toFloat(i.x) + sv.x) * rcp(toFloat(size.x));
    vfloat v = (toFloat(i.y) + sv.y) * rcp(toFloat(size.y));

    vfloat theta = (1.0f - v) * float(pi);
    vfloat phi = (u - 0.5f) * (2.0f*float(pi));

    vfloat sinPhi, cosPhi;
    sincos(phi, sinPhi, cosPhi);
    vfloat sinTheta, cosTheta;
    sincos(theta, sinTheta, cosTheta);

    wi = Vec3vf(sinPhi*sinTheta, cosTheta, -cosPhi*sinTheta);
    pdf *= toFloat(size.x) * toFloat(size.y) * rcp(2.0f*float(pi)*float(pi)*sinTheta);
    dist = posMax;

    return color->get3f(m, Vec2vf(u, v)) * rcp(pdf);
  }

  float getPower() const
  {
    Vec2i size = color->getSize();
    float sumL = 0.f;

    for (int y = 0; y != size.y; ++y)
    {
      for (int x = 0; x < size.x; ++x)
        sumL += luminance(color->get3f(Vec2i(x,y)));
    }

    return 4.f * sqr(float(pi)) * sqr(sceneRadius) * (sumL / float(size.x * size.y));
  }
};

} // namespace prt
