// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <unordered_map>
#include "sys/array.h"
#include "sys/string.h"
#include "sys/props.h"
#include "image/image_texture.h"
#include "medium.h"

namespace prt {

class Material;

class MaterialFactory
{
private:
  std::unordered_map<std::string, std::shared_ptr<ImageTextureBuffer>> textures;
  std::unordered_map<std::string, std::shared_ptr<ImageTextureBuffer>> bumpNormalTextures;
  std::shared_ptr<Media> media;
  bool texturingEnabled;
  bool flipNormalMapYEnabled;
  bool emissiveEnabled;
  std::string path;

public:
  MaterialFactory(const std::string& path, const Props& props);

  // Materials
  std::shared_ptr<Material> make(const Props& props);
  std::shared_ptr<Material> makeDefault();
  static bool isEmissive(const Props& props);

  // Textures
  std::shared_ptr<ImageTexture> makeTexture(const std::string& filename, const ImageTextureParams& params);
  std::shared_ptr<ImageTexture> makeBumpNormalTexture(const std::string& filename, const ImageTextureParams& params);

  // Media
  std::shared_ptr<Medium> makeMedium(const Vec3f& color, float ior, int& id);
  const std::shared_ptr<Media>& getMedia() const { return media; }

  prt_inline bool isTexturingEnabled() const
  {
    return texturingEnabled;
  }

  prt_inline bool isFlipNormalMapYEnabled() const
  {
    return flipNormalMapYEnabled;
  }

  prt_inline bool isEmissiveEnabled() const
  {
    return emissiveEnabled;
  }
};

} // namespace prt
