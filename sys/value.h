// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common.h"
#include "string.h"
#include "stream.h"
#include "constants.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"
#include "math/basis3.h"

namespace prt {

// Variant value
class Value
{
public:
  enum class Type
  {
    Empty,
    Int1,
    Int2,
    Int3,
    Float1,
    Float2,
    Float3,
    Basis3,
    Long1,
    Ulong1,
    Double1,
    String
  };

private:
  Type type;

  union
  {
    int i[9];
    float f[9];
    int64_t l[3];
    uint64_t ul[3];
    double d[3];
  } num;
  std::string str;

public:
  Value() : type(Type::Empty) {}
  Value(Empty) : type(Type::Empty) {}

  Value(int a) : type(Type::Int1) { num.i[0] = a; }

  Value(const Vec2i& a) : type(Type::Int2)
  {
    num.i[0] = a[0];
    num.i[1] = a[1];
  }

  Value(const Vec3i& a) : type(Type::Int3)
  {
    num.i[0] = a[0];
    num.i[1] = a[1];
    num.i[2] = a[2];
  }

  Value(float a) : type(Type::Float1) { num.f[0] = a; }

  Value(const Vec2f& a) : type(Type::Float2)
  {
    num.f[0] = a[0];
    num.f[1] = a[1];
  }

  Value(const Vec3f& a) : type(Type::Float3)
  {
    num.f[0] = a[0];
    num.f[1] = a[1];
    num.f[2] = a[2];
  }

  Value(const Basis3f& a) : type(Type::Basis3)
  {
    num.f[0] = a.U.x; num.f[1] = a.U.y; num.f[2] = a.U.z;
    num.f[3] = a.V.x; num.f[4] = a.V.y; num.f[5] = a.V.z;
    num.f[6] = a.N.x; num.f[7] = a.N.y; num.f[8] = a.N.z;
  }

  Value(int64_t a)  : type(Type::Long1)  { num.l[0]  = a; }
  Value(uint64_t a) : type(Type::Ulong1) { num.ul[0] = a; }

  Value(double a) : type(Type::Double1) { num.d[0] = a; }

  Value(const char* s) : type(Type::String) { str = s; }
  Value(const std::string& s) : type(Type::String) { str = s; }

  operator std::string() const;

  bool isEmpty() const
  {
    return type == Type::Empty;
  }

  Type getType() const
  {
    return type;
  }

  template <class T>
  T get(int base = 0) const;

  friend std::ostream& operator <<(std::ostream& osm, const Value& val);

  template <class StreamType>
  friend void serialize(StreamType& osm, const Value& val);

  template <class StreamType>
  friend void deserialize(StreamType& ism, Value& val);
};

template <>
inline int Value::get(int base) const
{
  switch (type)
  {
  case Type::Int1:
    return num.i[0];

  case Type::Float1:
    return (int)num.f[0];

  case Type::Long1:
    return (int)num.l[0];

  case Type::Ulong1:
    return (int)num.ul[0];

  case Type::Double1:
    return (int)num.d[0];

  case Type::String:
    return fromString<int>(str, base);

  default:
    throw std::logic_error("incompatible type");
  }
}

template <>
inline Vec2i Value::get(int base) const
{
  switch (type)
  {
  case Type::Int1:
    return Vec2i(num.i[0]);

  case Type::Float1:
    return Vec2i((int)num.f[0]);

  case Type::Int2:
    return Vec2i(num.i[0], num.i[1]);

  case Type::Float2:
    return Vec2i((int)num.f[0], (int)num.f[1]);

  case Type::String:
    return fromString<Vec2i>(str, base);

  default:
    throw std::logic_error("incompatible type");
  }
}

template <>
inline Vec3i Value::get(int base) const
{
  switch (type)
  {
  case Type::Int1:
    return Vec3i(num.i[0]);

  case Type::Float1:
    return Vec3i((int)num.f[0]);

  case Type::Int3:
    return Vec3i(num.i[0], num.i[1], num.i[2]);

  case Type::Float3:
    return Vec3i((int)num.f[0], (int)num.f[1], (int)num.f[2]);

  case Type::String:
    return fromString<Vec3i>(str, base);

  default:
    throw std::logic_error("incompatible type");
  }
}

template <>
inline float Value::get(int base) const
{
  switch (type)
  {
  case Type::Int1:
    return (float)num.i[0];

  case Type::Float1:
    return num.f[0];

  case Type::Long1:
    return (float)num.l[0];

  case Type::Ulong1:
    return (float)num.ul[0];

  case Type::Double1:
    return (float)num.d[0];

  case Type::String:
    return fromString<float>(str, base);

  default:
    throw std::logic_error("incompatible type");
  }
}

template <>
inline Vec2f Value::get(int base) const
{
  switch (type)
  {
  case Type::Int1:
    return Vec2f((float)num.i[0]);

  case Type::Float1:
    return Vec2f(num.f[0]);

  case Type::Int2:
    return Vec2f((float)num.i[0], (float)num.i[1]);

  case Type::Float2:
    return Vec2f(num.f[0], num.f[1]);

  case Type::String:
    return fromString<Vec2f>(str, base);

  default:
    throw std::logic_error("incompatible type");
  }
}

template <>
inline Vec3f Value::get(int base) const
{
  switch (type)
  {
  case Type::Int1:
    return Vec3f((float)num.i[0]);

  case Type::Float1:
    return Vec3f(num.f[0]);

  case Type::Int3:
    return Vec3f((float)num.i[0], (float)num.i[1], (float)num.i[2]);

  case Type::Float3:
    return Vec3f(num.f[0], num.f[1], num.f[2]);

  case Type::String:
    return fromString<Vec3f>(str, base);

  default:
    throw std::logic_error("incompatible type");
  }
}

template <>
inline Basis3f Value::get(int base) const
{
  switch (type)
  {
  case Type::Basis3:
    return Basis3f(Vec3f(num.f[0], num.f[1], num.f[2]),
                   Vec3f(num.f[3], num.f[4], num.f[5]),
                   Vec3f(num.f[6], num.f[7], num.f[8]));

  //case Type::string:
  //    return fromString<Basis3f>(str);

  default:
    throw std::logic_error("incompatible type");
  }
}

template <>
inline int64_t Value::get(int base) const
{
  switch (type)
  {
  case Type::Int1:
    return num.i[0];

  case Type::Float1:
    return (int64_t)num.f[0];

  case Type::Long1:
    return num.l[0];

  case Type::Ulong1:
    return (int64_t)num.ul[0];

  case Type::Double1:
    return (int64_t)num.d[0];

  case Type::String:
    return fromString<int64_t>(str, base);

  default:
    throw std::logic_error("incompatible type");
  }
}

template <>
inline uint64_t Value::get(int base) const
{
  switch (type)
  {
  case Type::Int1:
    return (uint64_t)num.i[0];

  case Type::Float1:
    return (uint64_t)num.f[0];

  case Type::Long1:
    return (uint64_t)num.l[0];

  case Type::Ulong1:
    return num.ul[0];

  case Type::Double1:
    return (uint64_t)num.d[0];

  case Type::String:
    return fromString<uint64_t>(str, base);

  default:
    throw std::logic_error("incompatible type");
  }
}

template <>
inline double Value::get(int base) const
{
  switch (type)
  {
  case Type::Int1:
    return num.i[0];

  case Type::Float1:
    return num.f[0];

  case Type::Long1:
    return (double)num.l[0];

  case Type::Ulong1:
    return (double)num.ul[0];

  case Type::Double1:
    return num.d[0];

  case Type::String:
    return fromString<double>(str, base);

  default:
    throw std::logic_error("incompatible type");
  }
}

template <>
inline std::string Value::get(int base) const
{
  if (type == Type::Empty)
    return std::string();

  if (type == Type::String)
    return str;

  std::stringstream sm;
  sm << std::setbase(base) << *this;
  return sm.str();
}

inline Value::operator std::string() const
{
  return get<std::string>();
}

inline std::ostream& operator <<(std::ostream& osm, const Value& val)
{
  switch (val.type)
  {
  case Value::Type::Int1:
    return osm << val.num.i[0];

  case Value::Type::Int2:
    return osm << Vec2i(val.num.i[0], val.num.i[1]);

  case Value::Type::Int3:
    return osm << Vec3i(val.num.i[0], val.num.i[1], val.num.i[2]);

  case Value::Type::Float1:
    return osm << val.num.f[0];

  case Value::Type::Float2:
    return osm << Vec2f(val.num.f[0], val.num.f[1]);

  case Value::Type::Float3:
    return osm << Vec3f(val.num.f[0], val.num.f[1], val.num.f[2]);

  case Value::Type::Basis3:
    return osm << Basis3f(Vec3f(val.num.f[0], val.num.f[1], val.num.f[2]),
                          Vec3f(val.num.f[3], val.num.f[4], val.num.f[5]),
                          Vec3f(val.num.f[6], val.num.f[7], val.num.f[8]));

  case Value::Type::Long1:
    return osm << val.num.l[0];

  case Value::Type::Ulong1:
    return osm << val.num.ul[0];

  case Value::Type::Double1:
    return osm << val.num.d[0];

  case Value::Type::String:
    return osm << val.str;

  default:
    return osm;
  }
}

// Serialization
// -------------

template <class StreamType>
void serialize(StreamType& osm, const Value& val)
{
  osm << val.type;

  switch (val.type)
  {
  case Value::Type::Empty:
    break;

  case Value::Type::String:
    osm << val.str;
    break;

  default:
    osm << val.num;
    break;
  }
}

template <class StreamType>
void deserialize(StreamType& ism, Value& val)
{
  ism >> val.type;

  switch (val.type)
  {
  case Value::Type::Empty:
    break;

  case Value::Type::String:
    ism >> val.str;
    break;

  default:
    val.str.clear();
    ism >> val.num;
    break;
  }
}

inline Stream& operator <<(Stream& osm, const Value& val) { serialize(osm, val); return osm; }
inline Stream& operator >>(Stream& ism, Value& val) { deserialize(ism, val); return ism; }

} // namespace prt
