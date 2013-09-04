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
#include "elfhash.h"

#include <iostream>
#include <iomanip>
#include <string>

using namespace std;
using namespace mageec;

int main(int argc, char const *argv[])
{
  bool verbose = false;
  bool failure = false;
  int f = 1;

  if (argc < 2 || (argc == 2 && string(argv[1]) == "-v"))
  {
    cerr << "Usage: " << argv[0] << " [-v] file [file] ..." << endl;
    return 1;
  }

  if (string(argv[1]) == "-v")
  {
    verbose = true;
    f = 2;
  }

  for (; f < argc; f++)
  {
    hashedelf elffile(verbose);
    int err = elffile.hash (argv[f]);
    if (err != 0)
    {
      cerr << "Error with " << argv[f] << endl;
      failure = true;
    }
    else
      cout << setfill('0') << setw(16) << setbase(16) << elffile.getResult()
           << ' ' << argv[f] << endl;
  }

  if (failure)
    return 1;
  return 0;
}