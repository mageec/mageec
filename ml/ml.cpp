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
  /* FIXME: The second parameter should be false, we do not want to be creating
     a new database here */
  db = new database(compiler_version + "-" + compiler_target + ".db", true);
  passes = db->get_pass_list();
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

std::vector<mageec_pass*> mageec_ml::all_passes (void)
{
  return passes;
}

void mageec_ml::add_result (std::vector<mageec_feature*> features __attribute__((unused)),
                            std::vector<mageec_pass*> passes,
                            int64_t metric,
                            bool good __attribute__((unused)))
{
  if (!db)
    return;

  result res = {.passlist = passes,
                .progname = "",
                .metric = metric};

  db->add_result(res);
}

void mageec_ml::add_result (result res)
{
  if (!db)
    return;
  db->add_result(res);
}
