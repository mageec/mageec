/*  MAGEEC Feature Implementation
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

/** @file features.cpp MAGEEC Program Features. */
#include "mageec-features.h"

using namespace mageec;

basic_feature::basic_feature(std::string feat_name)
{
  feature_name = feat_name;
}

basic_feature::basic_feature(std::string feat_name, int value)
{
  feature_name = feat_name;
  feature_value = value;
}

std::string basic_feature::name()
{
  return feature_name;
}

int basic_feature::get_feature()
{
  return feature_value;
}