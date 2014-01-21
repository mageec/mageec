/*  ELF Object Hasher Command
    Copyright (C) 2013 Embecosm Limited and University of Bristol

    This file is part of MAGEEC.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>. */

/** @file mageechash.cpp ELF Object Hasher Command. */
#include "mageec/elfhash.h"

#include <iostream>
#include <iomanip>
#include <string>

using namespace mageec;

int main(int argc, char const *argv[])
{
  bool failure = false;
  int f = 1;

  if (argc < 2)
  {
    std::cerr << "Usage: " << argv[0] << " file [file] ..." << std::endl;
    return 1;
  }

  for (; f < argc; f++)
  {
    hashedelf elffile;
    int err = elffile.hash (argv[f]);
    if (err != 0)
    {
      std::cerr << "Error with " << argv[f] << std::endl;
      failure = true;
    }
    else
      std::cout << std::setfill('0') << std::setw(16) << std::setbase(16)
                << elffile.getResult() << ' ' << argv[f] << std::endl;
  }

  if (failure)
    return 1;
  return 0;
}