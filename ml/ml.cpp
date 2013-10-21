/*  MAGEEC Machine Learner
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

/** @file ml.cpp MAGEEC Machine Learner */
#include "mageec-ml.h"
#include <iostream>

using namespace mageec;

int mageec_ml::init (std::string compiler_version,
                     std::string compiler_target)
{
  std::cerr << "LEARNER: Hello!" << std::endl;
  db = new database(compiler_version + "-" + compiler_target + ".db", true);
  flags = db->get_flag_list();
  return 0;
}

void mageec_ml::new_file (void)
{
  std::cerr << "LEARNER: New file" << std::endl;
}

void mageec_ml::end_file (void)
{
  std::cerr << "LEARNER: End file" << std::endl;
}

void mageec_ml::finish (void)
{
  std::cerr << "LEARNER: Goodbye!" << std::endl;
  delete db;
}

std::vector<mageec_flag*> mageec_ml::all_flags (void)
{
  return flags;
}

void mageec_ml::add_result (std::vector<mageec_feature*> features __attribute__((unused)),
                            std::vector<mageec_flag*> flags __attribute__((unused)),
                            int64_t metric __attribute__((unused)),
                            bool good __attribute__((unused)))
{

}
