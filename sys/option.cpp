// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "file.h"
#include "text_reader.h"
#include "option.h"

namespace prt {

std::ostream& operator <<(std::ostream& osm, const Option& opt)
{
  if (!opt.name.empty())
    osm << "-" << opt.name;

  if (!opt.name.empty() && !opt.value.isEmpty())
    osm << "=";

  if (!opt.value.isEmpty())
    osm << opt.value;

  return osm;
}

void parseOptions(int argc, char* argv[], Array<Option>& opts)
{
  bool isOpt = false;
  Option opt;

  for (int i = 1; i < argc; ++i)
  {
    // Copy the arg
    std::vector<char> buffer(strlen(argv[i]) + 1);
    char* arg = buffer.data();
    strcpy(arg, argv[i]);

    // Check if it's an option
    if (strlen(arg) > 1 && arg[0] == '-' && isalpha(arg[1]))
    {
      // Push previous option if necessary
      if (isOpt) opts.pushBack(opt);

      // Skip leading '-'s
      do
      {
        ++arg;
      } while (*arg == '-');

      // Look for an '='
      char* eq = strchr(arg, '=');
      if (eq)
      {
        *eq = 0;
        char* value = eq+1;

        opt.name = arg;
        opt.value = value;

        opts.pushBack(opt);
        isOpt = false;
      }
      else
      {
        opt.name = arg;
        opt.value = empty;

        isOpt = true;
      }
    }
    else
    {
      if (!isOpt) opt.name.clear();
      opt.value = arg;

      opts.pushBack(opt);
      isOpt = false;
    }
  }

  if (isOpt) opts.pushBack(opt);
}

void parseOptions(const std::string& filename, Array<Option>& opts)
{
  char arg[4096];
  Array<char*> argv;

  TextReader reader(std::make_shared<File>(filename));

  argv.pushBack(0);
  while (reader.readString(arg, sizeof(arg)) != EOF)
  {
    char* str = new char[strlen(arg)+1];
    strcpy(str, arg);
    argv.pushBack(str);
  }

  parseOptions(argv.getSize(), argv.getData(), opts);

  for (char* str : argv)
    delete[] str;
}

} // namespace prt
