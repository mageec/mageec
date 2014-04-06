/*  MAGEEC Program Feature Class
    Copyright (C) 2013, 2014 Embecosm Limited and University of Bristol

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

#include <iostream>
#include <string>
#include <vector>

namespace mageec
{
/**
 * MAGEEC Program Feature Base Type.
 * This class consists of a name "string" and an int to reveal that information.
 */
class mageec_feature
{
public:
  virtual std::string name() = 0;
  virtual std::string desc() = 0;
  virtual int get_feature() = 0;

  static void dump_vector(std::vector<mageec_feature*> features,
                          std::ostream &OS, bool json);
};

class basic_feature : public mageec_feature
{
  std::string feature_name;
  std::string feature_desc;
  int feature_value;
public:
  basic_feature (std::string name);
  basic_feature (std::string name, int value);
  basic_feature (std::string name, std::string desc, int value);
  std::string name();
  std::string desc();
  int get_feature();
};

} // End namespace mageec

#endif