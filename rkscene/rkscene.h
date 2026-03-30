// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <limits>
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <filesystem>
#include <fstream>
#include "json.hpp"

#if defined(RKSCENE_GZIP_SUPPORT)
  #include <boost/iostreams/filtering_stream.hpp>
  #include <boost/iostreams/filter/gzip.hpp>
#endif

namespace rkscene {

  #if defined(RKSCENE_FLOAT2)
    using float2 = RKSCENE_FLOAT2;
    static_assert(sizeof(float2) == sizeof(float) * 2, "invalid float2 size");
  #else
    struct float2
    {
      float x, y;

      float2() {}
      float2(float a) : x(a), y(a) {}
      float2(float x, float y) : x(x), y(y) {}

      bool operator ==(const float2& b) const { return x == b.x && y == b.y; }
      bool operator !=(const float2& b) const { return !(*this == b); }
    };
  #endif

  #if defined(RKSCENE_FLOAT3)
    using float3 = RKSCENE_FLOAT3;
    static_assert(sizeof(float3) == sizeof(float) * 3, "invalid float3 size");
  #else
    struct float3
    {
      float x, y, z;

      float3() {}
      float3(float a) : x(a), y(a), z(a) {}
      float3(float x, float y, float z) : x(x), y(y), z(z) {}

      bool operator ==(const float3& b) const { return x == b.x && y == b.y && z == b.z; }
      bool operator !=(const float3& b) const { return !(*this == b); }
    };

    inline float3 operator +(const float3& a, const float3& b)
    {
      return {a.x + b.x, a.y + b.y, a.z + b.z};
    }

    inline float3 operator *(float a, const float3& b)
    {
      return {a * b.x, a * b.y, a * b.z};
    }

    inline float3 operator *(const float3& a, float b)
    {
      return {a.x * b, a.y * b, a.z * b};
    }
  #endif

  #if defined(RKSCENE_FLOAT4)
    using float4 = RKSCENE_FLOAT4;
    static_assert(sizeof(float4) == sizeof(float) * 4, "invalid float4 size");
  #else
    struct float4
    {
      float x, y, z, w;

      float4() {}
      float4(float a) : x(a), y(a), z(a), w(a) {}
      float4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

      bool operator ==(const float4& b) const { return x == b.x && y == b.y && z == b.z && w == b.w; }
      bool operator !=(const float4& b) const { return !(*this == b); }
    };
  #endif

  #if defined(RKSCENE_FLOAT3X4)
    using float3x4 = RKSCENE_FLOAT3X4;
    static_assert(sizeof(float3x4) == sizeof(float) * 12, "invalid float3x4 size");
  #else
    struct float3x4
    {
      float3 v[4];

      float3x4() {}
      float3x4(const float3& v0, const float3& v1, const float3& v2, const float3& v3)
        : v{v0, v1, v2, v3} {}

      bool operator ==(const float3x4& b) const
      {
        return v[0] == b.v[0] && v[1] == b.v[1] && v[2] == b.v[2] && v[3] == b.v[3];
      }

      bool operator !=(const float3x4& b) const { return !(*this == b); }
    };

    inline float3x4 operator *(const float3x4& a, const float3x4& b)
    {
      float3x4 c;
      for (int i = 0; i < 3; ++i)
        c.v[i] = a.v[0] * b.v[i].x + a.v[1] * b.v[i].y + a.v[2] * b.v[i].z;
      c.v[3] = a.v[0] * b.v[3].x + a.v[1] * b.v[3].y + a.v[2] * b.v[3].z + a.v[3];
      return c;
    }

  #endif

  // -----------------------------------------------------------------------------------------------

  enum class TextureType
  {
    Default,
    Bump,
  };

  struct TextureTransform
  {
    float2 offset {0.f, 0.f};
    float2 scale  {1.f, 1.f};
  };

  template<typename FactorT>
  struct Texture
  {
    TextureType type {TextureType::Default};
    FactorT factor {1.f};
    std::string image;
    std::string swizzle;
    int texcoord {0};
    TextureTransform transform;

    Texture() {}
    Texture(const FactorT& value) : factor(value) {}

    template<typename U>
    Texture(const U& x, const U& y) : factor(x, y) {}

    template<typename U>
    Texture(const U& x, const U& y, const U& z) : factor(x, y, z) {}

    bool operator ==(FactorT b) const { return factor == b && image.empty(); }
    bool operator !=(FactorT b) const { return !(*this == b); }
  };

  using Texture1f = Texture<float>;
  using Texture3f = Texture<float3>;
  using TextureNormal3f = Texture<float>;

  struct Material
  {
    std::string name;

    // Base
    Texture1f baseWeight {1.f};
    Texture3f baseColor {0.8f, 0.8f, 0.8f};
    Texture1f metalness {0.f};
    Texture1f diffuseRoughness {0.f};
    TextureNormal3f normal;

    // Specular
    Texture1f specularWeight {1.f};
    Texture3f specularColor {1.f, 1.f, 1.f};
    Texture1f specularRoughness {0.3f};
    float specularIor {1.5f};

    // Transmission
    Texture1f transmissionWeight {0.f};
    Texture3f transmissionColor {1.f, 1.f, 1.f};
    float transmissionDepth {1.f};

    // Subsurface
    Texture1f subsurfaceWeight {0.f};
    Texture3f subsurfaceColor {0.8f, 0.8f, 0.8f};
    Texture1f subsurfaceAnisotropy {0.f};

    // Coat
    Texture1f coatWeight {0.f};
    Texture3f coatColor {1.f, 1.f, 1.f};
    Texture1f coatRoughness {0.f};
    TextureNormal3f coatNormal;
    float coatIor {1.6f};

    // Sheen
    Texture1f sheenWeight {0.f};
    Texture3f sheenColor {1.f, 1.f, 1.f};
    Texture1f sheenRoughness {0.3f};

    // Emission
    Texture1f emissionLuminance {0.f};
    Texture3f emissionColor {1.f, 1.f, 1.f};

    // Geometry
    bool thinWalled {false};
    Texture1f opacity {1.f};
  };

  enum class GeometryType
  {
    Triangle
  };

  struct Geometry
  {
    std::string name;
    GeometryType type {GeometryType::Triangle};
    std::vector<uint32_t> indices;
    std::vector<float3> positions;
    std::vector<float3> normals;
    std::vector<std::vector<float2>> texcoords; // (0, 0) is bottom-left
    int material {-1};

    size_t getNumPrimitives() const
    {
      switch (type)
      {
      case GeometryType::Triangle:
        return indices.size() / 3;
      default:
        throw std::runtime_error("invalid geometry type");
      }
    }
  };

  struct Object
  {
    std::string name;
    std::vector<int> geometries;
    std::vector<float3x4> instances;
  };

  enum class LightType
  {
    SunSky
  };

  struct Light
  {
    std::string name;
    LightType type {LightType::SunSky};
    float3 direction {0.f, 0.f, 0.f};
  };

  struct Scene
  {
    std::string name;
    std::vector<std::shared_ptr<Material>> materials;
    std::vector<std::shared_ptr<Geometry>> geometries;
    std::vector<std::shared_ptr<Object>>   objects;
    std::vector<std::shared_ptr<Light>>    lights;
  };

  // -----------------------------------------------------------------------------------------------

  namespace detail
  {
    using json = nlohmann::json;

    template<typename T>
    struct ToString;

    template<> struct ToString<int>          { static constexpr const char* value = "int";      };
    template<> struct ToString<unsigned int> { static constexpr const char* value = "uint";     };
    template<> struct ToString<float>        { static constexpr const char* value = "float";    };
    template<> struct ToString<float2>       { static constexpr const char* value = "float2";   };
    template<> struct ToString<float3>       { static constexpr const char* value = "float3";   };
    template<> struct ToString<float4>       { static constexpr const char* value = "float4";   };
    template<> struct ToString<float3x4>     { static constexpr const char* value = "float3x4"; };

    template<typename T>
    inline json toJSON(const T& v) { return v; }

    template<> inline json toJSON(const float2& v) { return {v.x, v.y}; }
    template<> inline json toJSON(const float3& v) { return {v.x, v.y, v.z}; }
    template<> inline json toJSON(const float4& v) { return {v.x, v.y, v.z, v.w}; }

    template<typename T>
    inline T fromJSON(const json& v) { return v; }

    template<>
    inline float2 fromJSON(const json& v) { return {v[0], v[1]}; }

    template<>
    inline float3 fromJSON(const json& v) { return {v[0], v[1], v[2]}; }

    template<>
    inline float4 fromJSON(const json& v) { return {v[0], v[1], v[2], v[3]}; }

    class BinReader
    {
    public:
      BinReader(const std::string& filename, size_t byteLength)
        : file(filename, std::ios::binary),
          byteLength(byteLength)
      {
      #if defined(RKSCENE_GZIP_SUPPORT)
        bool isCompressed = std::filesystem::path(filename).extension() == ".gz";
        if (!isCompressed && !file)
        {
          file.clear();
          file.open(filename + ".gz", std::ios::binary);
          isCompressed = true;
        }

        if (!file)
          throw std::runtime_error("failed to open file: " + filename);

        if (isCompressed)
        {
          {
            boost::iostreams::filtering_istream stream;
            stream.push(boost::iostreams::gzip_decompressor());
            stream.push(file);

            data.resize(byteLength);
            stream.read(data.data(), byteLength);
            if (!stream)
              throw std::runtime_error("failed to read buffer data");
          }

          file.close();
        }
      #else
        if (!file)
          throw std::runtime_error("failed to open file: " + filename);
      #endif
      }

      template<typename T>
      void read(std::vector<T>& buffer, const json& jbuffer)
      {
        const std::string type = ToString<T>::value;
        if (jbuffer["type"] != type)
          throw std::runtime_error("unexpected buffer type");

        const size_t count = jbuffer["count"];
        const size_t byteOffset = jbuffer["byteOffset"];

        if (byteOffset + count * sizeof(T) > byteLength)
          throw std::runtime_error("buffer access out of bounds");

        buffer.resize(count);

        if (file.is_open())
        {
          file.seekg(byteOffset);
          file.read(reinterpret_cast<char*>(buffer.data()), count * sizeof(T));
          if (!file)
            throw std::runtime_error("failed to read binary data from file");
        }
        else
        {
          std::memcpy((void*)buffer.data(), data.data() + byteOffset, count * sizeof(T));
        }
      }

    private:
      std::ifstream file;
      size_t byteLength {0};
      std::vector<char> data;
    };

    class BinWriter
    {
    public:
      explicit BinWriter(const std::string& filename)
      {
      #if defined(RKSCENE_GZIP_SUPPORT)
        const std::string gzFilename = filename + ".gz";
        file.open(gzFilename, std::ios::binary);
        if (!file)
          throw std::runtime_error("failed to open file: " + gzFilename);
        stream.push(boost::iostreams::gzip_compressor(boost::iostreams::gzip::best_compression));
        stream.push(file);
      #else
        stream.open(filename, std::ios::binary);
        if (!stream)
          throw std::runtime_error("failed to open file: " + filename);
      #endif
      }

      template<typename T>
      json write(const std::vector<T>& buffer)
      {
        const std::string type = ToString<T>::value;
        json jbuffer = {
          {"type", type},
          {"count", buffer.size()},
          {"byteOffset", byteOffset}
        };

        stream.write(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(T));
        byteOffset += buffer.size() * sizeof(T);

        return jbuffer;
      }

      size_t getByteLength() const
      {
        return byteOffset;
      }

    private:
    #if defined(RKSCENE_GZIP_SUPPORT)
      std::ofstream file;
      boost::iostreams::filtering_ostream stream;
    #else
      std::ofstream stream;
    #endif

      size_t byteOffset = 0;
    };

    inline std::string getBinFilename(const std::string& filename)
    {
      std::filesystem::path path(filename);
      return path.replace_extension(".rkbin").string();
    }

    inline std::string getBinRelativeFilename(const std::string& filename)
    {
      const std::string binFilename = getBinFilename(filename);
      return std::filesystem::path(binFilename).filename().string();
    }

    template<typename T>
    inline void loadMaterialParam(const json& jmat, const std::string& name, T& param)
    {
      if (jmat.contains(name))
        param = fromJSON<T>(jmat[name]);
    }

    template<typename FactorT>
    inline void loadMaterialParam(const json& jmat, const std::string& name, Texture<FactorT>& tex)
    {
      if (!jmat.contains(name))
        return;

      const json& jtex = jmat[name];

      if (jtex.is_string())
      {
        tex.factor = 1.f;
        tex.image = jtex;
      }
      else if (jtex.is_number() || jtex.is_array())
      {
        tex.factor = fromJSON<FactorT>(jtex);
      }
      else if (jtex.is_object())
      {
        const json& jtex = jmat[name];

        if (jtex.contains("type"))
        {
          const std::string type = jtex["type"];
          if (type == "bump")
            tex.type = TextureType::Bump;
          else
            throw std::runtime_error("unsupported texture type for material parameter");
        }

        if (jtex.contains("image"))
        {
          tex.factor = 1.f;
          tex.image = jtex["image"];
        }

        if (jtex.contains("factor"))
          tex.factor = fromJSON<FactorT>(jtex["factor"]);

        if (jtex.contains("swizzle"))
          tex.swizzle = jtex["swizzle"];

        if (jtex.contains("texcoord"))
          tex.texcoord = jtex["texcoord"];

        if (jtex.contains("transform"))
        {
          const json& jtextf = jtex["transform"];
          if (jtextf.contains("offset"))
            tex.transform.offset = fromJSON<float2>(jtextf["offset"]);
          if (jtextf.contains("scale"))
            tex.transform.scale = fromJSON<float2>(jtextf["scale"]);
        }
      }
    }

    inline std::shared_ptr<Material> loadMaterial(const json& jmat)
    {
      auto mat = std::make_shared<Material>();

      loadMaterialParam(jmat, "name", mat->name);

      loadMaterialParam(jmat, "baseWeight", mat->baseWeight);
      loadMaterialParam(jmat, "baseColor", mat->baseColor);
      loadMaterialParam(jmat, "metalness", mat->metalness);
      loadMaterialParam(jmat, "diffuseRoughness", mat->diffuseRoughness);
      loadMaterialParam(jmat, "normal", mat->normal);

      loadMaterialParam(jmat, "specularWeight", mat->specularWeight);
      loadMaterialParam(jmat, "specularColor", mat->specularColor);
      loadMaterialParam(jmat, "specularRoughness", mat->specularRoughness);
      loadMaterialParam(jmat, "specularIOR", mat->specularIor);

      loadMaterialParam(jmat, "transmissionWeight", mat->transmissionWeight);
      loadMaterialParam(jmat, "transmissionColor", mat->transmissionColor);
      loadMaterialParam(jmat, "transmissionDepth", mat->transmissionDepth);

      loadMaterialParam(jmat, "subsurfaceWeight", mat->subsurfaceWeight);
      loadMaterialParam(jmat, "subsurfaceColor", mat->subsurfaceColor);
      loadMaterialParam(jmat, "subsurfaceAnisotropy", mat->subsurfaceAnisotropy);

      loadMaterialParam(jmat, "coatWeight", mat->coatWeight);
      loadMaterialParam(jmat, "coatColor", mat->coatColor);
      loadMaterialParam(jmat, "coatRoughness", mat->coatRoughness);
      loadMaterialParam(jmat, "coatIOR", mat->coatIor);
      loadMaterialParam(jmat, "coatNormal", mat->coatNormal);

      loadMaterialParam(jmat, "sheenWeight", mat->sheenWeight);
      loadMaterialParam(jmat, "sheenColor", mat->sheenColor);
      loadMaterialParam(jmat, "sheenRoughness", mat->sheenRoughness);

      loadMaterialParam(jmat, "emissionLuminance", mat->emissionLuminance);
      loadMaterialParam(jmat, "emissionColor", mat->emissionColor);

      loadMaterialParam(jmat, "thinWalled", mat->thinWalled);
      loadMaterialParam(jmat, "opacity", mat->opacity);

      return mat;
    }

    template<typename T>
    inline void saveMaterialParam(json& jmat, const std::string& name, const T& param)
    {
      jmat[name] = toJSON(param);
    }

    template<typename FactorT>
    inline void saveMaterialParam(json& jmat, const std::string& name, const Texture<FactorT>& tex)
    {
      if (tex.type == TextureType::Default &&
          tex.factor == FactorT(1.f) && !tex.image.empty() &&
          tex.swizzle.empty() && tex.texcoord == 0 &&
          tex.transform.offset == float2{0.f, 0.f} &&
          tex.transform.scale  == float2{1.f, 1.f})
      {
        jmat[name] = tex.image;
      }
      else if (tex.image.empty())
      {
        jmat[name] = toJSON(tex.factor);
      }
      else
      {
        json jtex;

        if (tex.type == TextureType::Bump)
          jtex["type"] = "bump";

        if (tex.factor != FactorT(1.f))
          jtex["factor"] = toJSON(tex.factor);

        if (!tex.image.empty())
        {
          jtex["image"] = tex.image;

          if (!tex.swizzle.empty())
            jtex["swizzle"] = tex.swizzle;

          if (tex.texcoord != 0)
            jtex["texcoord"] = tex.texcoord;

          if (tex.transform.offset != float2{0.f, 0.f} ||
              tex.transform.scale  != float2{1.f, 1.f})
          {
            json jtextf;
            if (tex.transform.offset != float2{0.f, 0.f})
              jtextf["offset"] = toJSON(tex.transform.offset);
            if (tex.transform.scale != float2{1.f, 1.f})
              jtextf["scale"] = toJSON(tex.transform.scale);
            jtex["transform"] = jtextf;
          }
        }

        jmat[name] = jtex;
      }
    }

    inline json saveMaterial(const std::shared_ptr<Material>& mat)
    {
      json jmat;

      if (!mat->name.empty())
        saveMaterialParam(jmat, "name", mat->name);

      if (mat->metalness != 0.f || mat->transmissionWeight != 1.f)
      {
        if (mat->baseWeight != 1.f)
          saveMaterialParam(jmat, "baseWeight", mat->baseWeight);
        if (mat->metalness != 0.f || mat->subsurfaceWeight != 1.f)
          saveMaterialParam(jmat, "baseColor", mat->baseColor);
        if (mat->metalness != 0.f)
          saveMaterialParam(jmat, "metalness", mat->metalness);
        if (mat->metalness != 1.f)
          saveMaterialParam(jmat, "diffuseRoughness", mat->diffuseRoughness);
      }

      if (!mat->normal.image.empty())
        saveMaterialParam(jmat, "normal", mat->normal);

      saveMaterialParam(jmat, "specularWeight", mat->specularWeight);
      if (mat->specularWeight != 0.f || mat->metalness != 0.f || mat->transmissionWeight != 0.f)
      {
        if (mat->specularColor != float3{1.f, 1.f, 1.f} && mat->metalness == 0.f)
          saveMaterialParam(jmat, "specularColor", mat->specularColor);
        saveMaterialParam(jmat, "specularRoughness", mat->specularRoughness);
        if (mat->metalness != 1.f)
          saveMaterialParam(jmat, "specularIOR", mat->specularIor);
      }

      if (mat->transmissionWeight != 0.f)
      {
        saveMaterialParam(jmat, "transmissionWeight", mat->transmissionWeight);
        if (mat->transmissionColor != float3{1.f, 1.f, 1.f})
          saveMaterialParam(jmat, "transmissionColor", mat->transmissionColor);
        if (mat->transmissionDepth != 1.f)
          saveMaterialParam(jmat, "transmissionDepth", mat->transmissionDepth);
      }

      if (mat->subsurfaceWeight != 0.f)
      {
        saveMaterialParam(jmat, "subsurfaceWeight", mat->subsurfaceWeight);
        saveMaterialParam(jmat, "subsurfaceColor", mat->subsurfaceColor);
        saveMaterialParam(jmat, "subsurfaceAnisotropy", mat->subsurfaceAnisotropy);
      }

      if (mat->coatWeight != 0.f)
      {
        saveMaterialParam(jmat, "coatWeight", mat->coatWeight);
        if (mat->coatColor != float3{1.f, 1.f, 1.f})
          saveMaterialParam(jmat, "coatColor", mat->coatColor);
        saveMaterialParam(jmat, "coatRoughness", mat->coatRoughness);
        saveMaterialParam(jmat, "coatIOR", mat->coatIor);
        if (!mat->coatNormal.image.empty())
          saveMaterialParam(jmat, "coatNormal", mat->coatNormal);
      }

      if (mat->sheenWeight != 0.f)
      {
        saveMaterialParam(jmat, "sheenWeight", mat->sheenWeight);
        if (mat->sheenColor != float3{1.f, 1.f, 1.f})
          saveMaterialParam(jmat, "sheenColor", mat->sheenColor);
        saveMaterialParam(jmat, "sheenRoughness", mat->sheenRoughness);
      }

      if (mat->emissionLuminance != 0.f)
      {
        saveMaterialParam(jmat, "emissionLuminance", mat->emissionLuminance);
        if (mat->emissionColor != float3{1.f, 1.f, 1.f})
          saveMaterialParam(jmat, "emissionColor", mat->emissionColor);
      }

      if (mat->thinWalled)
        saveMaterialParam(jmat, "thinWalled", mat->thinWalled);
      if (mat->opacity != 1.f)
        saveMaterialParam(jmat, "opacity", mat->opacity);

      return jmat;
    }

    inline std::shared_ptr<Geometry> loadGeometry(const json& jgeom, BinReader& binFile)
    {
      auto geom = std::make_shared<Geometry>();

      const std::string type = jgeom["type"];
      if (type == "triangle")
        geom->type = GeometryType::Triangle;
      else
        throw std::runtime_error("unsupported geom type");

      if (jgeom.contains("name"))
        geom->name = jgeom["name"];

      if (jgeom.contains("positions"))
        binFile.read(geom->positions, jgeom["positions"]);

      if (jgeom.contains("normals"))
        binFile.read(geom->normals, jgeom["normals"]);

      if (jgeom.contains("texcoords"))
      {
        const json& jtexcoords = jgeom["texcoords"];
        if (jtexcoords.is_array())
        {
          for (const auto& jtexcoordSet : jtexcoords)
          {
            geom->texcoords.emplace_back();
            binFile.read(geom->texcoords.back(), jtexcoordSet);
          }
        }
        else
        {
          geom->texcoords.emplace_back();
          binFile.read(geom->texcoords.back(), jtexcoords);
        }
      }

      if (jgeom.contains("indices"))
        binFile.read(geom->indices, jgeom["indices"]);

      if (jgeom.contains("material"))
        geom->material = jgeom["material"];

      return geom;
    }

    inline json saveGeometry(const std::shared_ptr<Geometry>& geom, BinWriter& binFile)
    {
      json jgeom;

      if (!geom->name.empty())
        jgeom["name"] = geom->name;

      switch (geom->type)
      {
      case GeometryType::Triangle:
        jgeom["type"] = "triangle";
        break;
      default:
        throw std::runtime_error("unsupported geom type");
      }

      if (!geom->positions.empty())
        jgeom["positions"] = binFile.write(geom->positions);

      if (!geom->normals.empty())
        jgeom["normals"] = binFile.write(geom->normals);

      if (!geom->texcoords.empty())
      {
        if (geom->texcoords.size() > 1)
        {
          json jtexcoordSets = json::array();
          for (const auto& texcoordSet : geom->texcoords)
            jtexcoordSets.push_back(binFile.write(texcoordSet));
          jgeom["texcoords"] = jtexcoordSets;
        }
        else
          jgeom["texcoords"] = binFile.write(geom->texcoords[0]);
      }

      if (!geom->indices.empty())
        jgeom["indices"] = binFile.write(geom->indices);

      if (geom->material >= 0)
        jgeom["material"] = geom->material;

      return jgeom;
    }

    inline std::shared_ptr<Object> loadObject(const json& jobj, BinReader& binFile)
    {
      auto obj = std::make_shared<Object>();

      if (jobj.contains("name"))
        obj->name = jobj["name"];

      if (jobj.contains("geometries"))
      {
        for (int i : jobj["geometries"])
          obj->geometries.push_back(i);
      }

      if (jobj.contains("instances"))
        binFile.read(obj->instances, jobj["instances"]);

      return obj;
    }

    inline json saveObject(const std::shared_ptr<Object>& obj, BinWriter& binFile)
    {
      json jobj;

      if (!obj->name.empty())
        jobj["name"] = obj->name;

      auto& jobjGeoms = jobj["geometries"];
      for (int i : obj->geometries)
        jobjGeoms.push_back(i);

      if (!obj->instances.empty())
        jobj["instances"] = binFile.write(obj->instances);

      return jobj;
    }

    inline std::shared_ptr<Light> loadLight(const json& jlight)
    {
      auto light = std::make_shared<Light>();

      const std::string type = jlight["type"];
      if (type == "sunSky")
        light->type = LightType::SunSky;
      else
        throw std::runtime_error("unsupported light type");

      if (jlight.contains("direction"))
        light->direction = fromJSON<float3>(jlight["direction"]);

      return light;
    }

    inline json saveLight(const std::shared_ptr<Light>& light)
    {
      json jlight;

      if (!light->name.empty())
        jlight["name"] = light->name;

      switch (light->type)
      {
      case LightType::SunSky:
        jlight["type"] = "sunSky";
        break;
      default:
        throw std::runtime_error("unsupported light type");
      }

      if (light->type == LightType::SunSky)
        jlight["direction"] = toJSON(light->direction);

      return jlight;
    }

    inline std::shared_ptr<Scene> loadScene(const std::string& filename)
    {
      std::ifstream jsonFile(filename);
      if (!jsonFile)
        throw std::runtime_error("failed to open file: " + filename);

      json jscene;
      jsonFile >> jscene;

      std::string binFilename;
      size_t binByteLength = 0;
      if (jscene.contains("buffers"))
      {
        const json& jbuffers = jscene["buffers"];
        if (!jbuffers.is_array() || jbuffers.size() != 1)
          throw std::runtime_error("currently only single buffer is supported");
        binFilename = jbuffers[0]["uri"];
        binByteLength = jbuffers[0]["byteLength"];

        if (!std::filesystem::path(binFilename).is_absolute())
          binFilename = (std::filesystem::absolute(filename).parent_path() / binFilename).string();
      }
      else
      {
        throw std::runtime_error("no buffers specified");
      }

      BinReader binFile(binFilename, binByteLength);

      auto scene = std::make_shared<Scene>();

      if (jscene.contains("materials"))
      {
        for (const auto& jmat : jscene["materials"])
          scene->materials.push_back(loadMaterial(jmat));
      }

      if (jscene.contains("geometries"))
      {
        for (const auto& jgeom : jscene["geometries"])
          scene->geometries.push_back(loadGeometry(jgeom, binFile));
      }

      if (jscene.contains("objects"))
      {
        for (const auto& jobj : jscene["objects"])
          scene->objects.push_back(loadObject(jobj, binFile));
      }

      if (jscene.contains("lights"))
      {
        for (const auto& jlight : jscene["lights"])
          scene->lights.push_back(loadLight(jlight));
      }

      return scene;
    }

    inline void saveScene(const Scene* scene, const std::string& filename)
    {
      BinWriter binFile(getBinFilename(filename));

      json jscene;

      if (!scene->name.empty())
        jscene["name"] = scene->name;

      json& jmats = jscene["materials"];
      for (const auto& material : scene->materials)
        jmats.push_back(saveMaterial(material));

      json& jgeoms = jscene["geometries"];
      for (const auto& geom : scene->geometries)
        jgeoms.push_back(saveGeometry(geom, binFile));

      json& jobjs = jscene["objects"];
      for (const auto& object : scene->objects)
        jobjs.push_back(saveObject(object, binFile));

      json& jlights = jscene["lights"];
      for (const auto& light : scene->lights)
        jlights.push_back(saveLight(light));

      json& jbuffers = jscene["buffers"];
      jbuffers.push_back({
        {"uri", getBinRelativeFilename(filename)},
        {"byteLength", binFile.getByteLength()}
      });

      std::ofstream jsonFile(filename);
      if (!jsonFile)
        throw std::runtime_error("failed to open file: " + filename);

      jsonFile << jscene.dump(2);
    }
  };

  // -----------------------------------------------------------------------------------------------

  inline std::shared_ptr<Scene> load(const std::string& filename)
  {
    return detail::loadScene(filename);
  }

  inline void save(const Scene& scene, const std::string& filename)
  {
    detail::saveScene(&scene, filename);
  }

} // namespace rkscene