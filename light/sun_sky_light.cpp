// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "sys/logging.h"
#include "sys/tasking.h"
#include "math/integration.h"
#include "image/color.h"
#include "image/image_texture.h"
#include "sampling/shape_sampler.h"
#include "distant_light.h"
#include "image_light.h"
#include "sky_model/sky_model.h"
#include "sun_sky_light.h"

namespace prt {

SunSkyLight::SunSkyLight(const Props& props, float sceneRadius)
{
  Log() << "Precomputing sun/sky light";

  const float lambdaMin = 320.0f;
  const float lambdaMax = 720.0f;

  Vec3f sunDir = normalize(props.get<Vec3f>("direction"));
  float albedo = props.get("albedo", 0.2f);

  float turbidity = props.get("turbidity", 3.0f);
  if (turbidity < 1.0f || turbidity > 10.0)
    throw std::invalid_argument("turbidity must be between 1 and 10");

  float horizon = clamp(props.get("horizon", 0.f), 0.01f, 1.f);

  float radiusScale = props.get("radiusScale", 1.f);
  float sunScale = props.get("sunScale", 1.f);
  float skyScale = props.get("skyScale", 1.f);
  float sunSaturation = props.get("sunSaturation", -1.f); // negative means no saturation change
  float skySaturation = props.get("skySaturation", -1.f); // negative means no saturation change
  float sunHue = props.get("sunHue", 0.f);
  float skyHue = props.get("skyHue", 0.f);

  double sunTheta = min(double(acosSafe(sunDir.y)), 0.999*double(pi)/2.0);
  double sunPhi = atan2(sunDir.x, -sunDir.z);

  double sunElevation = double(pi)/2.0 - sunTheta;

  ArHosekSkyModelState* model = arhosekskymodelstate_alloc_init(sunElevation, turbidity, albedo);
  float sunRadius = float(model->solar_radius);

  // Integrate the solar radiance
  auto sunRadianceFunc = [=](Vec2f s)
  {
    float pdf;
    s = min(s, Vec2f(1.0f - 1e-6f));
    Vec3d d = makeFrame(sunDir) * uniformSampleCone(pdf, s, sunRadius);

    double theta = min(acosSafe(d.y), 0.999*double(pi)/2.0);
    double phi = atan2(d.x, -d.z);
    double cosGamma = cos(theta) * cos(sunTheta) + sin(theta) * sin(sunTheta) * cos(phi - sunPhi);
    double gamma = acosSafe(cosGamma);

    Vec3f radiance = zero;
    float radianceNorm = zero;

    for (int i = 0; i < cieSize; ++i)
    {
      if (cieLambda[i] >= lambdaMin && cieLambda[i] <= lambdaMax)
      {
        double r = arhosekskymodel_solar_radiance(model, theta, gamma, cieLambda[i]) - arhosekskymodel_radiance(model, theta, gamma, cieLambda[i]);
        radiance += float(r) * cieXyz(i);
        radianceNorm += cieY[i];
      }
    }

    return xyzToRgb(radiance / radianceNorm);
  };

  Vec3f sunRadiance = integrate2D(sunRadianceFunc, 0.0f, 1.0f, 5.0f);

  sunRadiance = rgbToHsv(sunRadiance);
  sunRadiance.x += sunHue; // offset hue
  if (sunSaturation >= 0.f)
    sunRadiance.y = sunSaturation; // change saturation
  sunRadiance = hsvToRgb(sunRadiance);
  sunRadiance *= sunScale;

  // Create the sun light
  if (radiusScale != 1.f)
  {
    sunRadiance *= sqr(sin(sunRadius)/sin(sunRadius*radiusScale));
    sunRadius *= radiusScale;
  }
  Props sunLightProps;
  sunLightProps.set("Ke", sunRadiance);
  sunLightProps.set("direction", props.get<Vec3f>("direction"));
  sunLightProps.set("angle", sunRadius * 2.0f);

  //Log() << "Sun: " << sunLightProps;

  sunLight = std::make_shared<DistantLight>(sunLightProps, sceneRadius);

  // Create the sky light
  const int skyResolution = 512;
  const int skyLutResolution = 128;
  Vec2i skySize(skyResolution, skyResolution/2);
  std::shared_ptr<Image3f> skyImage = std::make_shared<Image3f>(skySize);

  tbb::parallel_for(tbb::blocked_range<int>(0, skySize.y), [&](const tbb::blocked_range<int>& r)
  {
    for (int y = r.begin(); y != r.end(); ++y)
    {
      for (int x = 0; x < skySize.x; ++x)
      {
        const double maxTheta = 0.999*double(pi)/2.0;
        const double maxThetaHorizon = (horizon+1.0)*double(pi)/2.0;

        Vec3f radiance = zero;
        double theta = (double(y) + 0.5) / skySize.y * double(pi);

        if (theta <= maxThetaHorizon)
        {
          float shadow = (horizon > 0.f) ? float(clamp((maxThetaHorizon - theta) / (maxThetaHorizon - maxTheta), 0., 1.)) : 1.f;
          theta = min(theta, maxTheta);

          double phi = ((double(x) + 0.5) / skySize.x - 0.5) * (2.0*double(pi));

          double cosGamma = cos(theta) * cos(sunTheta) + sin(theta) * sin(sunTheta) * cos(phi - sunPhi);
          double gamma = acosSafe(cosGamma);

          float radianceNorm = zero;

          for (int i = 0; i < cieSize; ++i)
          {
            if (cieLambda[i] >= lambdaMin && cieLambda[i] <= lambdaMax)
            {
              double r = arhosekskymodel_radiance(model, theta, gamma, cieLambda[i]);
              radiance += float(r) * cieXyz(i);
              radianceNorm += cieY[i];
            }
          }

          radiance = xyzToRgb(radiance / radianceNorm) * shadow;
          radiance *= skyScale;
        }

        (*skyImage)[y][x] = max(radiance, Vec3f(0.0f));
      }
    }
  });

  // Change hue/saturation if needed
  if (skyHue > 0.f || skySaturation >= 0.f)
  {
    float skySaturationScale = 1.f;

    if (skySaturation >= 0.f)
    {
      float maxSkySaturation = 0.f;

      for (int y = 0; y < skySize.y; ++y)
      {
        for (int x = 0; x < skySize.x; ++x)
        {
          Vec3f radiance = (*skyImage)[y][x];
          radiance = rgbToHsv(radiance);
          maxSkySaturation = max(maxSkySaturation, radiance.y);
        }
      }

      skySaturationScale = (maxSkySaturation > 0.f) ? min(skySaturation / maxSkySaturation, 100.f) : 1.f;
    }

    for (int y = 0; y < skySize.y; ++y)
    {
      for (int x = 0; x < skySize.x; ++x)
      {
        Vec3f radiance = (*skyImage)[y][x];
        radiance = rgbToHsv(radiance);
        radiance.x += skyHue; // offset hue
        radiance.y *= skySaturationScale; // change saturation
        radiance = hsvToRgb(radiance);
        (*skyImage)[y][x] = radiance;
      }
    }
  }

  //saveImage("sky.exr", *skyImage);

  Props skyLightProps;
  skyLightProps.set("lutSize", skyLutResolution);
  skyLight = std::make_shared<ImageLight>(std::make_shared<ImageTextureBuffer>(skyImage, PixelFormat::Rgb32f), skyLightProps, sceneRadius);

  arhosekskymodelstate_free(model);
}

} // namespace prt

