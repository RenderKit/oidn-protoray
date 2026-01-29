// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "stream.h"

namespace prt {

class TextReader : Uncopyable
{
private:
  std::shared_ptr<Stream> stream;
  int buffer; // one char

  void nextChar();

public:
  TextReader(std::shared_ptr<Stream> sm);

  int readChar();
  int peekChar();
  int readLine(char* dest, int maxCount);
  int readString(char* dest, int maxCount);
};

} // namespace prt
