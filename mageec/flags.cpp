/*  MAGEEC Flag Implementation
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

/** @file flags.cpp MAGEEC Compiler Flags. */
#include "mageec-flags.h"

using namespace mageec;

basic_flag::basic_flag(std::string feat_name)
{
  flag_name = feat_name;
}

std::string basic_flag::name()
{
  return flag_name;
}

int basic_flag::value()
{
  return 0;
}