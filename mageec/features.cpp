/*  MAGEEC Feature Implementation
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

/** @file features.cpp MAGEEC Program Features. */
#include "mageec/mageec-features.h"

using namespace mageec;

static std::string pad_string(std::string input, unsigned length, bool left)
{
  if (input.length() == length)
    return input;
  std::string pad(length-input.length(), ' ');
  if (left)
    return pad + input;
  return input + pad;
}

void mageec_feature::dump_vector(std::vector<mageec_feature*> features,
                                 std::ostream &OS, bool json)
{
  if (features.size() == 0)
    return;

  if (json)
  {
    OS << "[";
    for (unsigned i = 0, size = features.size(); i < size; i++)
    {
      OS << "{\"name\":\"" << features[i]->name() << "\",";
      OS << "\"desc\":\"" << features[i]->desc() << "\",";
      OS << "\"value\":" << features[i]->get_feature() << "}";
      if (i+1 != size)
        OS << ',';
    }
    OS << "]\n";
  } else {
    unsigned maxname = 0;
    unsigned maxdesc = 0;
    // Calculate the length of the longest name and description
    for (unsigned i = 0, size = features.size(); i < size; i++)
    {
      if (maxname < features[i]->name().length())
        maxname = features[i]->name().length();
      if (maxdesc < features[i]->desc().length())
        maxdesc = features[i]->desc().length();
    }
    for (unsigned i = 0, size = features.size(); i < size; i++)
    {
      OS << "  (";
      OS << pad_string(features[i]->name(), maxname, true);
      OS << ") ";
      OS << pad_string(features[i]->desc(), maxdesc, false);
      OS << ": ";
      OS << features[i]->get_feature();
      OS << '\n';
    }
  }
}

basic_feature::basic_feature(std::string feat_name)
{
  feature_name = feat_name;
}

basic_feature::basic_feature(std::string feat_name, int value)
{
  feature_name = feat_name;
  feature_desc = "";
  feature_value = value;
}

basic_feature::basic_feature(std::string feat_name, std::string feat_desc,
                             int value)
{
  feature_name = feat_name;
  feature_desc = feat_desc;
  feature_value = value;
}

std::string basic_feature::name()
{
  return feature_name;
}

std::string basic_feature::desc()
{
  return feature_desc;
}

int basic_feature::get_feature()
{
  return feature_value;
}
