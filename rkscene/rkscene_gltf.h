// Copyright 2026 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "rkscene.h"

namespace rkscene {

  namespace detail
  {
    inline const float3x4 identityTransform =
    {
      {1.f, 0.f, 0.f},
      {0.f, 1.f, 0.f},
      {0.f, 0.f, 1.f},
      {0.f, 0.f, 0.f}
    };

    class GLTFBuffer
    {
    public:
      GLTFBuffer(const json& jbuffer, const std::string& gltfFilename)
      {
        std::string filename = jbuffer["uri"].get<std::string>();
        if (!std::filesystem::path(filename).is_absolute())
          filename = (std::filesystem::absolute(gltfFilename).parent_path() / filename).string();

        const size_t byteLength = jbuffer["byteLength"];

      #if defined(RKSCENE_GZIP_SUPPORT)
        boost::iostreams::filtering_istream stream;

        std::ifstream file(filename, std::ios::binary);
        if (!file)
        {
          file.clear();
          file.open(filename + ".gz", std::ios::binary);
          if (!file)
            throw std::runtime_error("failed to open file: " + filename);
          stream.push(boost::iostreams::gzip_decompressor());
        }

        stream.push(file);
      #else
        std::ifstream stream(filename, std::ios::binary);
        if (!stream)
          throw std::runtime_error("failed to open file: " + filename);
      #endif

        data.resize(byteLength);
        stream.read(data.data(), byteLength);
        if (!stream)
          throw std::runtime_error("failed to read buffer data");
      }

      template<typename T>
      void loadArray(std::vector<T>& buffer, size_t byteOffset, size_t byteLength, size_t byteStride, size_t count)
      {
        if (byteStride == 0)
          byteStride = sizeof(T);
        if (count > 0 && (byteOffset + (count-1) * byteStride + sizeof(T)) > data.size())
          throw std::runtime_error("buffer view too small");

        buffer.resize(count);

        if (byteStride == sizeof(T))
        {
          std::memcpy((void*)buffer.data(), data.data() + byteOffset, count * sizeof(T));
        }
        else
        {
          const char* src = data.data() + byteOffset;
          for (size_t i = 0; i < count; ++i, src += byteStride)
            buffer[i] = *reinterpret_cast<const T*>(src);
        }
      }

    private:
      std::vector<char> data;
    };

    struct GLTFBufferView
    {
      int buffer {-1};
      size_t byteOffset {0};
      size_t byteLength {0};
      size_t byteStride {0};
    };

    inline GLTFBufferView loadGLTFBufferView(const json& jview)
    {
      GLTFBufferView view;
      view.buffer     = jview["buffer"];
      view.byteOffset = jview.value("byteOffset", 0);
      view.byteLength = jview["byteLength"];
      view.byteStride = jview.value("byteStride", 0);
      return view;
    }

    enum class GLTFType
    {
      Scalar,
      Vec2,
      Vec3,
      Vec4,
      Mat2,
      Mat3,
      Mat4
    };

    template<typename T>
    struct ToGLTFType;

    template<> struct ToGLTFType<uint16_t> { static constexpr auto value = GLTFType::Scalar; };
    template<> struct ToGLTFType<uint32_t> { static constexpr auto value = GLTFType::Scalar; };
    template<> struct ToGLTFType<float>    { static constexpr auto value = GLTFType::Scalar; };
    template<> struct ToGLTFType<float2>   { static constexpr auto value = GLTFType::Vec2; };
    template<> struct ToGLTFType<float3>   { static constexpr auto value = GLTFType::Vec3; };

    inline GLTFType toGLTFType(const std::string& typeStr)
    {
      if (typeStr == "SCALAR")
        return GLTFType::Scalar;
      else if (typeStr == "VEC2")
        return GLTFType::Vec2;
      else if (typeStr == "VEC3")
        return GLTFType::Vec3;
      else if (typeStr == "VEC4")
        return GLTFType::Vec4;
      else if (typeStr == "MAT2")
        return GLTFType::Mat2;
      else if (typeStr == "MAT3")
        return GLTFType::Mat3;
      else if (typeStr == "MAT4")
        return GLTFType::Mat4;
      else
        throw std::runtime_error("unknown glTF type: " + typeStr);
    }

    enum class GLTFComponentType
    {
      Byte   = 5120,
      UByte  = 5121,
      Short  = 5122,
      UShort = 5123,
      UInt   = 5125,
      Float  = 5126
    };

    template<typename T>
    struct ToGLTFComponentType;

    template<> struct ToGLTFComponentType<uint8_t>  { static constexpr auto value = GLTFComponentType::UByte;  };
    template<> struct ToGLTFComponentType<int8_t>   { static constexpr auto value = GLTFComponentType::Byte;   };
    template<> struct ToGLTFComponentType<uint16_t> { static constexpr auto value = GLTFComponentType::UShort; };
    template<> struct ToGLTFComponentType<int16_t>  { static constexpr auto value = GLTFComponentType::Short;  };
    template<> struct ToGLTFComponentType<uint32_t> { static constexpr auto value = GLTFComponentType::UInt;   };
    template<> struct ToGLTFComponentType<int32_t>  { static constexpr auto value = GLTFComponentType::UInt;   };
    template<> struct ToGLTFComponentType<float>    { static constexpr auto value = GLTFComponentType::Float;  };
    template<> struct ToGLTFComponentType<float2>   { static constexpr auto value = GLTFComponentType::Float;  };
    template<> struct ToGLTFComponentType<float3>   { static constexpr auto value = GLTFComponentType::Float;  };

    inline GLTFComponentType toGLTFComponentType(int componentType)
    {
      if (componentType < 5120 || componentType > 5126)
        throw std::runtime_error("unknown glTF component type");
      return static_cast<GLTFComponentType>(componentType);
    }

    struct GLTFAccessor
    {
      int bufferView {-1};
      size_t byteOffset {0};
      size_t count {0};
      GLTFType type {GLTFType::Scalar};
      GLTFComponentType componentType {GLTFComponentType::Float};
    };

    inline GLTFAccessor loadGLTFAccessor(const json& jacc)
    {
      GLTFAccessor acc;
      acc.bufferView    = jacc["bufferView"];
      acc.byteOffset    = jacc.value("byteOffset", 0);
      acc.count         = jacc["count"];
      acc.type          = toGLTFType(jacc["type"]);
      acc.componentType = toGLTFComponentType(jacc["componentType"]);
      return acc;
    }

    struct GLTFPrimitive
    {
      int positionAccessor {-1};
      int normalAccessor {-1};
      std::vector<int> texcoordAccessors;
      int indicesAccessor {-1};
      int material {-1};
    };

    inline GLTFPrimitive loadGLTFPrimitive(const json& jprim)
    {
      GLTFPrimitive prim;
      json jattribs = jprim["attributes"];
      if (jattribs.contains("POSITION"))
        prim.positionAccessor = jattribs["POSITION"];
      if (jattribs.contains("NORMAL"))
        prim.normalAccessor = jattribs["NORMAL"];
      for (int i = 0; jattribs.contains("TEXCOORD_" + std::to_string(i)); ++i)
        prim.texcoordAccessors.push_back(jattribs["TEXCOORD_" + std::to_string(i)]);
      if (jprim.contains("indices"))
        prim.indicesAccessor = jprim["indices"];
      if (jprim.contains("material"))
        prim.material = jprim["material"];
      return prim;
    }

    struct GLTFMesh
    {
      std::string name;
      std::vector<GLTFPrimitive> primitives;
    };

    inline GLTFMesh loadGLTFMesh(const json& jmesh)
    {
      GLTFMesh mesh;
      if (jmesh.contains("name"))
        mesh.name = jmesh["name"];
      for (const auto& jprim : jmesh["primitives"])
        mesh.primitives.push_back(loadGLTFPrimitive(jprim));
      return mesh;
    }

    struct GLTFImage
    {
      std::string name;
      std::string uri;
    };

    inline GLTFImage loadGLTFImage(const json& jimage)
    {
      GLTFImage image;
      if (jimage.contains("name"))
        image.name = jimage["name"];
      if (jimage.contains("uri"))
        image.uri = jimage["uri"];
      return image;
    }

    struct GLTFTexture
    {
      std::string name;
      int source {-1};
    };

    inline GLTFTexture loadGLTFTexture(const json& jtex)
    {
      GLTFTexture tex;
      if (jtex.contains("name"))
        tex.name = jtex["name"];
      if (jtex.contains("source"))
        tex.source = jtex["source"];
      return tex;
    }

    struct GLTFTextureInfo
    {
      float scale {1.f}; // normals only
      int index {-1};
      int texCoord {0};
      TextureTransform transform;

      operator bool() const { return index >= 0; }
    };

    inline GLTFTextureInfo loadGLTFTextureInfo(const json& jtexi)
    {
      GLTFTextureInfo texi;

      if (jtexi.contains("scale"))
        texi.scale = jtexi["scale"];

      if (jtexi.contains("index"))
        texi.index = jtexi["index"];
      if (jtexi.contains("texCoord"))
        texi.texCoord = jtexi["texCoord"];

      if (jtexi.contains("extensions"))
      {
        const json& jext = jtexi["extensions"];
        if (jext.contains("KHR_texture_transform"))
        {
          const json& jtt = jext["KHR_texture_transform"];
          if (jtt.contains("offset"))
            texi.transform.offset = fromJSON<float2>(jtt["offset"]);
          if (jtt.contains("scale"))
            texi.transform.scale = fromJSON<float2>(jtt["scale"]);
        }
      }

      return texi;
    }

    enum GLTFAlphaMode
    {
      OPAQUE,
      MASK,
      BLEND
    };

    struct GLTFMaterial
    {
      std::string name;

      // pbrMetallicRoughness
      float4 baseColorFactor {1.f, 1.f, 1.f, 1.f};
      GLTFTextureInfo baseColorTexture;
      float metallicFactor {1.f};
      float roughnessFactor {1.f};
      GLTFTextureInfo metallicRoughnessTexture;

      GLTFTextureInfo normalTexture;
      GLTFAlphaMode alphaMode {GLTFAlphaMode::OPAQUE};
      bool doubleSided {false};

      // KHR_materials_specular
      float specularFactor {1.f};
      GLTFTextureInfo specularTexture;
      float3 specularColorFactor {1.f, 1.f, 1.f};
      GLTFTextureInfo specularColorTexture;

      // KHR_materials_ior
      float ior {1.5f};

      // KHR_materials_transmission
      float transmissionFactor {0.f};
      GLTFTextureInfo transmissionTexture;

      // KHR_materials_sheen
      float3 sheenColorFactor {0.f, 0.f, 0.f};
      GLTFTextureInfo sheenColorTexture;
      float sheenRoughnessFactor {0.f};
      GLTFTextureInfo sheenRoughnessTexture;

      // KHR_materials_diffuse_transmission
      float diffuseTransmissionFactor {0.f};
      GLTFTextureInfo diffuseTransmissionTexture;
      float3 diffuseTransmissionColorFactor {1.f, 1.f, 1.f};
      GLTFTextureInfo diffuseTransmissionColorTexture;

      // KHR_materials_volume
      float thicknessFactor {0.f};
      //GLTFTextureInfo thicknessTexture; // ignored for ray tracing
      float attenuationDistance {std::numeric_limits<float>::infinity()};
      float3 attenuationColor {1.f, 1.f, 1.f};

      // KHR_materials_clearcoat
      float clearcoatFactor {0.f};
      GLTFTextureInfo clearcoatTexture;
      float clearcoatRoughnessFactor {0.f};
      GLTFTextureInfo clearcoatRoughnessTexture;
      GLTFTextureInfo clearcoatNormalTexture;

      float3 emissiveFactor {0.f, 0.f, 0.f};
      GLTFTextureInfo emissiveTexture;
      // KHR_materials_emissive_strength
      float emissiveStrength {1.f};

      // KHR_materials_unlit
      bool unlit {false};
    };

    inline GLTFMaterial loadGLTFMaterial(const json& jmat)
    {
      GLTFMaterial mat;
      if (jmat.contains("name"))
        mat.name = jmat["name"];

      if (jmat.contains("pbrMetallicRoughness"))
      {
        const auto& jpbr = jmat["pbrMetallicRoughness"];
        if (jpbr.contains("baseColorFactor"))
          mat.baseColorFactor = fromJSON<float4>(jpbr["baseColorFactor"]);
        if (jpbr.contains("baseColorTexture"))
          mat.baseColorTexture = loadGLTFTextureInfo(jpbr["baseColorTexture"]);
        if (jpbr.contains("metallicFactor"))
          mat.metallicFactor = jpbr["metallicFactor"];
        if (jpbr.contains("roughnessFactor"))
          mat.roughnessFactor = jpbr["roughnessFactor"];
        if (jpbr.contains("metallicRoughnessTexture"))
          mat.metallicRoughnessTexture = loadGLTFTextureInfo(jpbr["metallicRoughnessTexture"]);
      }

      if (jmat.contains("normalTexture"))
        mat.normalTexture = loadGLTFTextureInfo(jmat["normalTexture"]);

      if (jmat.contains("alphaMode"))
      {
        const std::string modeStr = jmat["alphaMode"];
        if (modeStr == "OPAQUE")
          mat.alphaMode = GLTFAlphaMode::OPAQUE;
        else if (modeStr == "MASK")
          mat.alphaMode = GLTFAlphaMode::MASK;
        else if (modeStr == "BLEND")
          mat.alphaMode = GLTFAlphaMode::BLEND;
        else
          throw std::runtime_error("unknown glTF alpha mode: " + modeStr);
      }

      if (jmat.contains("doubleSided"))
        mat.doubleSided = jmat["doubleSided"];

      if (jmat.contains("emissiveFactor"))
        mat.emissiveFactor = fromJSON<float3>(jmat["emissiveFactor"]);
      if (jmat.contains("emissiveTexture"))
        mat.emissiveTexture = loadGLTFTextureInfo(jmat["emissiveTexture"]);

      if (jmat.contains("extensions"))
      {
        const json& jext = jmat["extensions"];

        if (jext.contains("KHR_materials_specular"))
        {
          const json& jspec = jext["KHR_materials_specular"];
          if (jspec.contains("specularFactor"))
            mat.specularFactor = jspec["specularFactor"];
          if (jspec.contains("specularTexture"))
            mat.specularTexture = loadGLTFTextureInfo(jspec["specularTexture"]);
          if (jspec.contains("specularColorFactor"))
            mat.specularColorFactor = fromJSON<float3>(jspec["specularColorFactor"]);
          if (jspec.contains("specularColorTexture"))
            mat.specularColorTexture = loadGLTFTextureInfo(jspec["specularColorTexture"]);
        }

        if (jext.contains("KHR_materials_ior"))
        {
          const json& jior = jext["KHR_materials_ior"];
          if (jior.contains("ior"))
            mat.ior = jior["ior"];
        }

        if (jext.contains("KHR_materials_transmission"))
        {
          const json& jtrans = jext["KHR_materials_transmission"];
          if (jtrans.contains("transmissionFactor"))
            mat.transmissionFactor = jtrans["transmissionFactor"];
          if (jtrans.contains("transmissionTexture"))
            mat.transmissionTexture = loadGLTFTextureInfo(jtrans["transmissionTexture"]);
        }

        if (jext.contains("KHR_materials_sheen"))
        {
          const json& jsheen = jext["KHR_materials_sheen"];
          if (jsheen.contains("sheenColorFactor"))
            mat.sheenColorFactor = fromJSON<float3>(jsheen["sheenColorFactor"]);
          if (jsheen.contains("sheenColorTexture"))
            mat.sheenColorTexture = loadGLTFTextureInfo(jsheen["sheenColorTexture"]);
          if (jsheen.contains("sheenRoughnessFactor"))
            mat.sheenRoughnessFactor = jsheen["sheenRoughnessFactor"];
          if (jsheen.contains("sheenRoughnessTexture"))
            mat.sheenRoughnessTexture = loadGLTFTextureInfo(jsheen["sheenRoughnessTexture"]);
        }

        if (jext.contains("KHR_materials_diffuse_transmission"))
        {
          const json& jdifft = jext["KHR_materials_diffuse_transmission"];
          if (jdifft.contains("diffuseTransmissionFactor"))
            mat.diffuseTransmissionFactor = jdifft["diffuseTransmissionFactor"];
          if (jdifft.contains("diffuseTransmissionTexture"))
            mat.diffuseTransmissionTexture = loadGLTFTextureInfo(jdifft["diffuseTransmissionTexture"]);
          if (jdifft.contains("diffuseTransmissionColorFactor"))
            mat.diffuseTransmissionColorFactor = fromJSON<float3>(jdifft["diffuseTransmissionColorFactor"]);
          if (jdifft.contains("diffuseTransmissionColorTexture"))
            mat.diffuseTransmissionColorTexture = loadGLTFTextureInfo(jdifft["diffuseTransmissionColorTexture"]);
        }

        if (jext.contains("KHR_materials_volume"))
        {
          const json& jvol = jext["KHR_materials_volume"];
          if (jvol.contains("thicknessFactor"))
            mat.thicknessFactor = jvol["thicknessFactor"];
          if (jvol.contains("attenuationDistance"))
            mat.attenuationDistance = jvol["attenuationDistance"];
          if (jvol.contains("attenuationColor"))
            mat.attenuationColor = fromJSON<float3>(jvol["attenuationColor"]);
        }

        if (jext.contains("KHR_materials_clearcoat"))
        {
          const json& jcoat = jext["KHR_materials_clearcoat"];
          if (jcoat.contains("clearcoatFactor"))
            mat.clearcoatFactor = jcoat["clearcoatFactor"];
          if (jcoat.contains("clearcoatTexture"))
            mat.clearcoatTexture = loadGLTFTextureInfo(jcoat["clearcoatTexture"]);
          if (jcoat.contains("clearcoatRoughnessFactor"))
            mat.clearcoatRoughnessFactor = jcoat["clearcoatRoughnessFactor"];
          if (jcoat.contains("clearcoatRoughnessTexture"))
            mat.clearcoatRoughnessTexture = loadGLTFTextureInfo(jcoat["clearcoatRoughnessTexture"]);
          if (jcoat.contains("clearcoatNormalTexture"))
            mat.clearcoatNormalTexture = loadGLTFTextureInfo(jcoat["clearcoatNormalTexture"]);
        }

        if (jext.contains("KHR_materials_emissive_strength"))
        {
          const json& jemis = jext["KHR_materials_emissive_strength"];
          if (jemis.contains("emissiveStrength"))
            mat.emissiveStrength = jemis["emissiveStrength"];
        }

        if (jext.contains("KHR_materials_unlit"))
          mat.unlit = true;
      }

      return mat;
    }

    struct GLTFNode
    {
      std::string name;

      int mesh {-1};

      float3 translation {0.f, 0.f, 0.f};
      float4 rotation {0.f, 0.f, 0.f, 1.f};
      float3 scale {1.f, 1.f, 1.f};

      std::vector<int> children;

      bool hasTransform() const
      {
        return !(translation == float3(0.f, 0.f, 0.f) &&
                 rotation == float4(0.f, 0.f, 0.f, 1.f) &&
                 scale == float3(1.f, 1.f, 1.f));
      }

      float3x4 getTransform() const
      {
        if (!hasTransform())
          return identityTransform;

        const float4 q = rotation;
        const float qxx(q.x * q.x);
        const float qyy(q.y * q.y);
        const float qzz(q.z * q.z);
        const float qxz(q.x * q.z);
        const float qxy(q.x * q.y);
        const float qyz(q.y * q.z);
        const float qwx(q.w * q.x);
        const float qwy(q.w * q.y);
        const float qwz(q.w * q.z);

        const float m00 = 1.f - 2.f * (qyy + qzz);
        const float m01 = 2.f * (qxy + qwz);
        const float m02 = 2.f * (qxz - qwy);

        const float m10 = 2.f * (qxy - qwz);
        const float m11 = 1.f - 2.f * (qxx + qzz);
        const float m12 = 2.f * (qyz + qwx);

        const float m20 = 2.f * (qxz + qwy);
        const float m21 = 2.f * (qyz - qwx);
        const float m22 = 1.f - 2.f * (qxx + qyy);

        const float3 U {m00 * scale.x, m01 * scale.x, m02 * scale.x};
        const float3 V {m10 * scale.y, m11 * scale.y, m12 * scale.y};
        const float3 N {m20 * scale.z, m21 * scale.z, m22 * scale.z};

        return float3x4(U, V, N, translation);
      }
    };

    inline GLTFNode loadGLTFNode(const json& jnode)
    {
      GLTFNode node;
      if (jnode.contains("name"))
        node.name = jnode["name"];
      if (jnode.contains("mesh"))
        node.mesh = jnode["mesh"];
      if (jnode.contains("translation"))
        node.translation = fromJSON<float3>(jnode["translation"]);
      if (jnode.contains("rotation"))
        node.rotation = fromJSON<float4>(jnode["rotation"]);
      if (jnode.contains("scale"))
        node.scale = fromJSON<float3>(jnode["scale"]);
      if (jnode.contains("children"))
      {
        for (int child : jnode["children"])
          node.children.push_back(child);
      }
      return node;
    }

    struct GLTFScene
    {
      std::string name;
      std::vector<int> nodes;
    };

    inline GLTFScene loadGLTFScene(const json& jscene)
    {
      GLTFScene scene;
      if (jscene.contains("name"))
        scene.name = jscene["name"];
      for (int node : jscene["nodes"])
        scene.nodes.push_back(node);
      return scene;
    }

    struct GLTF
    {
      std::vector<GLTFBuffer> buffers;
      std::vector<GLTFBufferView> bufferViews;
      std::vector<GLTFAccessor> accessors;
      std::vector<GLTFMesh> meshes;
      std::vector<GLTFImage> images;
      std::vector<GLTFTexture> textures;
      std::vector<GLTFMaterial> materials;
      std::vector<GLTFNode> nodes;
      std::vector<GLTFScene> scenes;
      int scene {0};

      GLTFBuffer& getBuffer(int idx)
      {
        if (idx < 0 || idx >= static_cast<intptr_t>(buffers.size()))
          throw std::runtime_error("invalid glTF buffer index");
        return buffers[idx];
      }

      const GLTFBufferView& getBufferView(int idx) const
      {
        if (idx < 0 || idx >= static_cast<intptr_t>(bufferViews.size()))
          throw std::runtime_error("invalid glTF buffer view index");
        return bufferViews[idx];
      }

      const GLTFAccessor& getAccessor(int idx) const
      {
        if (idx < 0 || idx >= static_cast<intptr_t>(accessors.size()))
          throw std::runtime_error("invalid glTF accessor index");
        return accessors[idx];
      }

      const GLTFMesh& getMesh(int idx) const
      {
        if (idx < 0 || idx >= static_cast<intptr_t>(meshes.size()))
          throw std::runtime_error("invalid glTF mesh index");
        return meshes[idx];
      }

      const GLTFImage& getImage(int idx) const
      {
        if (idx < 0 || idx >= static_cast<intptr_t>(images.size()))
          throw std::runtime_error("invalid glTF image index");
        return images[idx];
      }

      const GLTFTexture& getTexture(int idx) const
      {
        if (idx < 0 || idx >= static_cast<intptr_t>(textures.size()))
          throw std::runtime_error("invalid glTF texture index");
        return textures[idx];
      }

      const GLTFMaterial& getMaterial(int idx) const
      {
        if (idx < 0 || idx >= static_cast<intptr_t>(materials.size()))
          throw std::runtime_error("invalid glTF material index");
        return materials[idx];
      }

      const GLTFNode& getNode(int idx) const
      {
        if (idx < 0 || idx >= static_cast<intptr_t>(nodes.size()))
          throw std::runtime_error("invalid glTF node index");
        return nodes[idx];
      }

      const GLTFScene& getScene(int idx) const
      {
        if (idx < 0 || idx >= static_cast<intptr_t>(scenes.size()))
          throw std::runtime_error("invalid glTF scene index");
        return scenes[idx];
      }

      template<typename InT, typename OutT>
      void loadArray(std::vector<OutT>& array, int accessorIdx)
      {
        const GLTFAccessor& accessor = getAccessor(accessorIdx);
        const GLTFBufferView& bufferView = getBufferView(accessor.bufferView);
        GLTFBuffer& buffer = getBuffer(bufferView.buffer);

        if (typeid(InT) == typeid(OutT))
        {
          buffer.loadArray(array,
                           bufferView.byteOffset + accessor.byteOffset,
                           bufferView.byteLength,
                           bufferView.byteStride,
                           accessor.count);
        }
        else
        {
          std::vector<InT> tempArray;
          buffer.loadArray(tempArray,
                           bufferView.byteOffset + accessor.byteOffset,
                           bufferView.byteLength,
                           bufferView.byteStride,
                           accessor.count);

          array.resize(tempArray.size());
          for (size_t i = 0; i < tempArray.size(); ++i)
            array[i] = static_cast<OutT>(tempArray[i]);
        }
      }

      void loadArray(std::vector<uint32_t>& array, int accessorIdx)
      {
        const GLTFAccessor& accessor = getAccessor(accessorIdx);
        if (accessor.type != GLTFType::Scalar)
          throw std::runtime_error("glTF type mismatch");

        switch (accessor.componentType)
        {
        case GLTFComponentType::Byte:
          loadArray<int8_t>(array, accessorIdx);
          return;
        case GLTFComponentType::UByte:
          loadArray<uint8_t>(array, accessorIdx);
          return;
        case GLTFComponentType::Short:
          loadArray<int16_t>(array, accessorIdx);
          return;
        case GLTFComponentType::UShort:
          loadArray<uint16_t>(array, accessorIdx);
          return;
        case GLTFComponentType::UInt:
          loadArray<uint32_t>(array, accessorIdx);
          return;
        default:
          throw std::runtime_error("glTF component type mismatch");
        }
      }

      void loadArray(std::vector<float2>& array, int accessorIdx)
      {
        const GLTFAccessor& accessor = getAccessor(accessorIdx);
        if (accessor.type != GLTFType::Vec2 || accessor.componentType != GLTFComponentType::Float)
          throw std::runtime_error("glTF type or component type mismatch");

        loadArray<float2>(array, accessorIdx);
      }

      void loadArray(std::vector<float3>& array, int accessorIdx)
      {
        const GLTFAccessor& accessor = getAccessor(accessorIdx);
        if (accessor.type != GLTFType::Vec3 || accessor.componentType != GLTFComponentType::Float)
          throw std::runtime_error("glTF type or component type mismatch");

        loadArray<float3>(array, accessorIdx);
      }
    };

    inline std::shared_ptr<GLTF> loadGLTF(const std::string& filename)
    {
      std::ifstream jsonFile(filename);
      if (!jsonFile)
        throw std::runtime_error("failed to open file: " + filename);

      json jdoc;
      jsonFile >> jdoc;

      auto gltf = std::make_shared<GLTF>();

      if (jdoc.contains("buffers"))
      {
        for (const auto& jbuffer : jdoc["buffers"])
          gltf->buffers.emplace_back(jbuffer, filename);
      }

      if (jdoc.contains("bufferViews"))
      {
        for (const auto& jview : jdoc["bufferViews"])
          gltf->bufferViews.push_back(loadGLTFBufferView(jview));
      }

      if (jdoc.contains("accessors"))
      {
        for (const auto& jacc : jdoc["accessors"])
          gltf->accessors.push_back(loadGLTFAccessor(jacc));
      }

      if (jdoc.contains("meshes"))
      {
        for (const auto& jmesh : jdoc["meshes"])
          gltf->meshes.push_back(loadGLTFMesh(jmesh));
      }

      if (jdoc.contains("images"))
      {
        for (const auto& jimage : jdoc["images"])
          gltf->images.push_back(loadGLTFImage(jimage));
      }

      if (jdoc.contains("textures"))
      {
        for (const auto& jtex : jdoc["textures"])
          gltf->textures.push_back(loadGLTFTexture(jtex));
      }

      if (jdoc.contains("materials"))
      {
        for (const auto& jmat : jdoc["materials"])
          gltf->materials.push_back(loadGLTFMaterial(jmat));
      }

      if (jdoc.contains("nodes"))
      {
        for (const auto& jnode : jdoc["nodes"])
          gltf->nodes.push_back(loadGLTFNode(jnode));
      }

      if (jdoc.contains("scenes"))
      {
        for (const auto& jscene : jdoc["scenes"])
          gltf->scenes.push_back(loadGLTFScene(jscene));
      }

      if (jdoc.contains("scene"))
        gltf->scene = jdoc["scene"];

      return gltf;
    }

    struct GLTFConverter
    {
      std::shared_ptr<GLTF> gltf;   // input
      std::shared_ptr<Scene> scene; // output

      std::unordered_map<int, int> meshToObjectMap;

      int convertMeshToObject(int meshIdx)
      {
        auto it = meshToObjectMap.find(meshIdx);
        if (it != meshToObjectMap.end())
          return it->second;

        const GLTFMesh& mesh = gltf->getMesh(meshIdx);

        auto obj = std::make_shared<Object>();
        obj->name = mesh.name;

        for (const GLTFPrimitive& prim : mesh.primitives)
        {
          auto geom = std::make_shared<Geometry>();

          if (prim.positionAccessor >= 0)
            gltf->loadArray(geom->positions, prim.positionAccessor);

          if (prim.normalAccessor >= 0)
            gltf->loadArray(geom->normals, prim.normalAccessor);

          for (int texcoordAccessor : prim.texcoordAccessors)
          {
            geom->texcoords.emplace_back();
            gltf->loadArray(geom->texcoords.back(), texcoordAccessor);

            // Flip V coordinate
            for (float2& uv : geom->texcoords.back())
              uv.y = 1.f - uv.y;
          }

          if (prim.indicesAccessor >= 0)
            gltf->loadArray(geom->indices, prim.indicesAccessor);

          geom->material = prim.material;

          int geomIdx = static_cast<int>(scene->geometries.size());
          scene->geometries.push_back(geom);
          obj->geometries.push_back(geomIdx);
        }

        int objIdx = static_cast<int>(scene->objects.size());
        scene->objects.push_back(obj);
        meshToObjectMap[meshIdx] = objIdx;
        return objIdx;
      }

      void convertNode(int nodeIdx, const float3x4& parentTransform)
      {
        const GLTFNode& node = gltf->getNode(nodeIdx);
        const float3x4 transform = parentTransform * node.getTransform();

        if (node.mesh >= 0)
        {
          int objIdx = convertMeshToObject(node.mesh);
          scene->objects[objIdx]->instances.push_back(transform);
        }

        for (int childIdx : node.children)
          convertNode(childIdx, transform);
      }
    };

    template<class FactorT>
    inline void convertGLTFTextureInfo(const std::shared_ptr<GLTF>& gltf, const GLTFTextureInfo& gtexi, Texture<FactorT>& tex)
    {
      if (gtexi.index >= 0)
      {
        const GLTFTexture& gtex = gltf->getTexture(gtexi.index);
        if (gtex.source >= 0)
        {
          const GLTFImage& gimg = gltf->getImage(gtex.source);
          tex.image = gimg.uri;
        }

        tex.texcoord = gtexi.texCoord;
        tex.transform = gtexi.transform;
        tex.transform.offset.y = 1.f - tex.transform.offset.y - tex.transform.scale.y; // flip V
      }
    }

    inline void convertGLTFTexture(const std::shared_ptr<GLTF>& gltf,
                                   float factor, const GLTFTextureInfo& gtexi, Texture1f& tex,
                                   const std::string& swizzle = "")
    {
      convertGLTFTextureInfo(gltf, gtexi, tex);
      tex.factor = factor;
      tex.swizzle = swizzle;
    }

    inline void convertGLTFTexture(const std::shared_ptr<GLTF>& gltf, float3 factor, const GLTFTextureInfo& gtexi, Texture3f& tex)
    {
      convertGLTFTextureInfo(gltf, gtexi, tex);
      tex.factor = factor;
    }

    inline void convertGLTFTexture(const std::shared_ptr<GLTF>& gltf, float4 factor, const GLTFTextureInfo& gtexi, Texture3f& tex)
    {
      convertGLTFTextureInfo(gltf, gtexi, tex);
      tex.factor = {factor.x, factor.y, factor.z};
    }

    inline void convertGLTFTexture(const std::shared_ptr<GLTF>& gltf, const GLTFTextureInfo& gtexi, TextureNormal3f& tex)
    {
      convertGLTFTexture(gltf, gtexi.scale, gtexi, tex);
    }

    inline std::shared_ptr<Scene> loadGLTFScene(const std::string& filename)
    {
      auto gltf = loadGLTF(filename);

      GLTFConverter converter;
      converter.gltf = gltf;
      converter.scene = std::make_shared<Scene>();

      const GLTFScene& gscene = gltf->getScene(gltf->scene);

      // Convert materials
      for (const GLTFMaterial& gmat : gltf->materials)
      {
        auto mat = std::make_shared<Material>();
        mat->name = gmat.name;

        convertGLTFTexture(gltf, gmat.baseColorFactor, gmat.baseColorTexture, mat->baseColor);
        convertGLTFTexture(gltf, gmat.metallicFactor, gmat.metallicRoughnessTexture, mat->metalness, "b");
        convertGLTFTexture(gltf, gmat.roughnessFactor, gmat.metallicRoughnessTexture, mat->specularRoughness, "g");
        convertGLTFTexture(gltf, gmat.normalTexture, mat->normal);

        convertGLTFTexture(gltf, gmat.specularFactor, gmat.specularTexture, mat->specularWeight, "a");
        convertGLTFTexture(gltf, gmat.specularColorFactor, gmat.specularColorTexture, mat->specularColor);
        mat->specularIor = gmat.ior;

        convertGLTFTexture(gltf, gmat.transmissionFactor, gmat.transmissionTexture, mat->transmissionWeight, "r");
        if (gmat.transmissionFactor != 0.f)
          convertGLTFTexture(gltf, gmat.baseColorFactor, gmat.baseColorTexture, mat->transmissionColor);

        if (gmat.sheenColorFactor != float3(0.f, 0.f, 0.f))
          mat->sheenWeight = 1.f;
        convertGLTFTexture(gltf, gmat.sheenColorFactor, gmat.sheenColorTexture, mat->sheenColor);
        convertGLTFTexture(gltf, gmat.sheenRoughnessFactor, gmat.sheenRoughnessTexture, mat->sheenRoughness, "a");

        convertGLTFTexture(gltf, gmat.diffuseTransmissionFactor, gmat.diffuseTransmissionTexture, mat->subsurfaceWeight, "a");
        convertGLTFTexture(gltf, gmat.diffuseTransmissionColorFactor, gmat.diffuseTransmissionColorTexture, mat->subsurfaceColor);
        if (gmat.diffuseTransmissionFactor != 0.f)
          mat->subsurfaceAnisotropy = 1.f; // diffuse transmission only

        if (gmat.thicknessFactor != 0.f && std::isfinite(gmat.attenuationDistance))
        {
          mat->transmissionDepth = gmat.attenuationDistance;
          mat->transmissionColor = gmat.attenuationColor;
        }

        convertGLTFTexture(gltf, gmat.clearcoatFactor, gmat.clearcoatTexture, mat->coatWeight, "r");
        convertGLTFTexture(gltf, gmat.clearcoatRoughnessFactor, gmat.clearcoatRoughnessTexture, mat->coatRoughness, "g");
        convertGLTFTexture(gltf, gmat.clearcoatNormalTexture, mat->coatNormal);
        mat->coatIor = 1.5f;

        if (gmat.emissiveFactor != float3(0.f, 0.f, 0.f))
        {
          convertGLTFTexture(gltf, gmat.emissiveFactor, gmat.emissiveTexture, mat->emissionColor);
          mat->emissionLuminance = gmat.emissiveStrength;
        }

        mat->thinWalled = gmat.thicknessFactor == 0.f;
        if (gmat.alphaMode == GLTFAlphaMode::MASK || gmat.alphaMode == GLTFAlphaMode::BLEND)
          convertGLTFTexture(gltf, gmat.baseColorFactor.w, gmat.baseColorTexture, mat->opacity, "a");

        converter.scene->materials.push_back(mat);
      }

      // Convert nodes
      for (int nodeIdx : gscene.nodes)
        converter.convertNode(nodeIdx, identityTransform);

      // Remove redundant identity transform from non-instanced objects
      for (auto& obj : converter.scene->objects)
      {
        if (obj->instances.size() == 1 && obj->instances[0] == identityTransform)
          obj->instances.clear();
      }

      return converter.scene;
    }

  } // namespace detail

  // -----------------------------------------------------------------------------------------------

  inline std::shared_ptr<Scene> loadGLTF(const std::string& filename)
  {
    return detail::loadGLTFScene(filename);
  }

} // namespace rkscene