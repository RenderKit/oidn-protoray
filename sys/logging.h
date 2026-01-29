// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <sstream>
#include "common.h"

namespace prt {

enum class LogLevel : int
{
  Error = 1,
  Warn,
  Info,
};

class LogMessage
{
private:
  std::ostringstream stream;
  LogLevel level;

public:
  explicit LogMessage(LogLevel level);
  ~LogMessage();

  template <class T>
  LogMessage& operator <<(const T& v)
  {
    stream << v;
    return *this;
  }
};


class Log : public LogMessage
{
public:
  Log() : LogMessage(LogLevel::Info) {}
};

class LogWarn : public LogMessage
{
public:
  LogWarn() : LogMessage(LogLevel::Warn) {}
};

class LogError : public LogMessage
{
public:
  LogError() : LogMessage(LogLevel::Error) {}
};


void initLogging();
void initLogging(const std::string& filename);
void setLogFile(const std::string& filename);
void setMaxLogLevel(LogLevel level);
LogLevel getMaxLogLevel();
double getElapsedTime();

} // namespace prt
