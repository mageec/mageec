/*  MAGEEC Program Feature Class
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

/** @file mageec-features.h MAGEEC Features */
#ifndef __MAGEEC_FEATURES_H_
#define __MAGEEC_FEATURES_H_

namespace mageec
{
/**
 * MAGEEC Program Feature Base Type.
 * This class consists of a name "string" and an int to reveal that information.
 */
class mageec_feature
{
public:
  std::string name();
  virtual int get_feature() = 0;
};

} // End namespace mageec

#endif