/*  MAGEEC Main Coordinator
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

/** @file mageec.cpp MAGEEC Main Coordinator. */
#include "mageec.h"
#include "mageec-ml.h"
#include <iostream>

using namespace mageec;

int mageec_framework::init (std::string compiler_version,
                            std::string compiler_target)
{ 
  std::cerr << "MAGEEC:  Targetting '" << compiler_version << "' for '"
       << compiler_target << "'" << std::endl;

  return ml.init ("", "");
}

void mageec_framework::new_file (std::string filename)
{
  std::cerr << "MAGEEC:  New source file " << filename << std::endl;
  ml.new_file();
}

void mageec_framework::end_file (void)
{
  std::cerr << "MAGEEC:  End of source file" << std::endl;
  ml.end_file();
}

void mageec_framework::finish (void)
{
  std::cerr << "MAGEEC:  Finish" << std::endl;
  ml.finish();
}
