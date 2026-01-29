// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vector>
#include <map>
#include "value.h"
#include "stream.h"

namespace prt {

// Property container
class Props
{
private:
  typedef std::vector<std::pair<std::string, Value>> Container;

  Container items;
  std::map<std::string, size_t> map; // maps names to indices in the container

public:
  typedef Container::iterator Iterator;
  typedef Container::const_iterator ConstIterator;

  int getSize() const
  {
    return (int)items.size();
  }

  bool isEmpty() const
  {
    return items.empty();
  }

  void clear()
  {
    items.clear();
    map.clear();
  }

  void set(const std::string& name)
  {
    set(name, empty);
  }

  void set(const std::string& name, const Value& value)
  {
    auto entry = map.find(name);
    if (entry == map.end())
    {
      map[name] = items.size();
      items.push_back(std::make_pair(name, value));
    }
    else
    {
      items[entry->second].second = value;
    }
  }

  template <class T = std::string>
  T get(const std::string& name) const
  {
    auto entry = map.find(name);
    if (entry == map.end())
      throw std::logic_error(std::string("property not found: ") + name);

    const Value& value = items[entry->second].second;
    return value.get<T>();
  }

  template <class T>
  T get(const std::string& name, const T& def, int base = 0) const
  {
    auto entry = map.find(name);
    if (entry == map.end())
      return def;

    const Value& value = items[entry->second].second;
    if (value.isEmpty())
      return def;
    return value.get<T>(base);
  }

  std::string get(const std::string& name, const char* def, int base = 0) const
  {
    auto entry = map.find(name);
    if (entry == map.end())
      return def;

    const Value& value = items[entry->second].second;
    if (value.isEmpty())
      return def;
    return value.get<std::string>(base);
  }

  bool exists(const std::string& name) const
  {
    return map.find(name) != map.end();
  }

  bool copy(const std::string& dest, const std::string& src)
  {
    auto entry = map.find(src);
    if (entry == map.end())
      return false;

    set(dest, items[entry->second].second);
    return true;
  }

  Props getSubProps(const std::string& parentName) const
  {
    Props subProps;
    std::string prefix = parentName + ".";

    for (const auto& i : items)
    {
      if (i.first.find(prefix) == 0)
      {
        std::string newName = i.first.substr(prefix.size());
        subProps.set(newName, i.second);
      }
    }

    return subProps;
  }

  Iterator begin() { return items.begin(); }
  Iterator end() { return items.end(); }
  ConstIterator begin() const { return items.begin(); }
  ConstIterator end() const { return items.end(); }
};

typedef std::map<std::string, Props> PropsMap;

// Serialization
// -------------

template <class StreamType>
inline void serialize(StreamType& osm, const Props& props)
{
  osm << props.getSize();

  for (auto& p : props)
    osm << p.first << p.second;
}

template <class StreamType>
inline void deserialize(StreamType& ism, Props& props)
{
  int size;
  ism >> size;

  props.clear();
  for (int i = 0; i < size; ++i)
  {
    std::string name;
    Value value;

    ism >> name >> value;
    props.set(name, value);
  }
}

inline Stream& operator <<(Stream& osm, const Props& props) { serialize(osm, props); return osm; }
inline Stream& operator >>(Stream& ism, Props& props) { deserialize(ism, props); return ism; }

inline std::ostream& operator <<(std::ostream& osm, const Props& props)
{
  bool isFirst = true;
  for (auto& i : props)
  {
    if (isFirst)
      isFirst = false;
    else
      osm << " ";

    osm << i.first << "=" << i.second;
  }
  return osm;
}

} // namespace prt
