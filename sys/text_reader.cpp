// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "text_reader.h"

namespace prt {

TextReader::TextReader(std::shared_ptr<Stream> sm)
{
  stream = sm;
  nextChar();
}

// Read the next character into the buffer
void TextReader::nextChar()
{
  char c;
  do
  {
    if (stream->read(&c, 1) == 0)
    {
      buffer = EOF;
      return;
    }
  } while (c == '\r');

  buffer = c;
}

int TextReader::readChar()
{
  int c = buffer;
  nextChar();
  return c;
}

int TextReader::peekChar()
{
  return buffer;
}

int TextReader::readLine(char* dest, int maxCount)
{
  assert(maxCount > 0);
  int count = 0;

  for (; ;)
  {
    int c = readChar();

    if (c == EOF && count == 0) return EOF;
    if (c == EOF || c == '\n') break;

    if (count < maxCount-1)
      dest[count++] = (char)c;
  }

  dest[count++] = 0;
  return count;
}

int TextReader::readString(char* dest, int maxCount)
{
  assert(maxCount > 0);
  int count = 0;

  for (; ;)
  {
    int c = peekChar();

    if (c == EOF)
    {
      if (count == 0) return EOF;
      break;
    }

    if (isspace(c))
    {
      if (count == 0)
      {
        readChar();
        continue;
      }
      break;
    }

    readChar();

    if (count < maxCount-1)
      dest[count++] = (char)c;
  }

  dest[count++] = 0;
  return count;
}

} // namespace prt
