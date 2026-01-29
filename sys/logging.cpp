// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <iostream>
#include <iomanip>
#include <fstream>

#include <ctime>

#include "file.h"
#include "mutex.h"
#include "lock_guard.h"
#include "timer.h"
#include "logging.h"

namespace prt {

namespace {

const char* logLevelNames[] = {"", "ERROR", "WARNING", "INFO"};

LogLevel maxLogLevel = LogLevel::Info;
bool isFileLoggingEnabled = false;
std::ofstream logFile;
Mutex logMutex;
Timer timer;

// Always runs on the host
void addLogMessage(LogLevel level, const std::string& text)
{
  // Acquire log lock
  LockGuard<Mutex> lock(logMutex);

  // Get the current time
  time_t rawTime;
  time(&rawTime);
  tm* decodedTime = localtime(&rawTime);

  // Make the header
  std::stringstream header;

  header << std::setfill('0') << "[" <<
            std::setw(2) << decodedTime->tm_hour << ":" <<
            std::setw(2) << decodedTime->tm_min << ":" <<
            std::setw(2) << decodedTime->tm_sec <<
            "] ";

  if (level != LogLevel::Info)
    header << "[" << logLevelNames[int(level)] << "] ";

  // Send the message to stdout
  if (level <= maxLogLevel)
    std::cout << header.str() << text << std::endl;

  // Save to file if necessary
  if (isFileLoggingEnabled)
  {
    // Make the full header
    std::stringstream fullHeader;

    fullHeader << std::setfill('0') << "[" <<
                  std::setw(4) << (decodedTime->tm_year + 1900) << "/" <<
                  std::setw(2) << (decodedTime->tm_mon + 1) << "/" <<
                  std::setw(2) << decodedTime->tm_mday <<
                  "|" <<
                  std::setw(2) << decodedTime->tm_hour << ":" <<
                  std::setw(2) << decodedTime->tm_min << ":" <<
                  std::setw(2) << decodedTime->tm_sec <<
                  "] ";

    if (level != LogLevel::Info)
      fullHeader << "[" << logLevelNames[int(level)] << "] ";

    // Save the message to the file
    logFile << fullHeader.str() << text << std::endl;
  }
}

} // namespace

void initLogging()
{
}

void initLogging(const std::string& filename)
{
  setLogFile(filename);
  initLogging();
}

void setLogFile(const std::string& filename)
{
  bool isExisting = File::exists(filename);
  logFile.open(filename.c_str(), std::ios::app);

  if (logFile.is_open())
  {
    isFileLoggingEnabled = true;
    if (isExisting)
      logFile << std::endl;
  }
  else
  {
    isFileLoggingEnabled = false;
    LogError() << "Could not open log file: " << filename;
  }
}

void setMaxLogLevel(LogLevel level)
{
  maxLogLevel = level;
}

LogLevel getMaxLogLevel()
{
  return maxLogLevel;
}

double getElapsedTime()
{
  return timer.query();
}

LogMessage::LogMessage(LogLevel level)
  : level(level)
{
  stream << std::fixed << std::setprecision(3);
}

LogMessage::~LogMessage()
{
  addLogMessage(level, stream.str());
}

} // namespace prt
