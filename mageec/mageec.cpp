/*  MAGEEC Main Coordinator
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

/** @file mageec.cpp MAGEEC Main Coordinator. */
#include "mageec/mageec.h"
#include "mageec/mageec-ml.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>

using namespace mageec;

int mageec_framework::init (std::string compiler_version,
                            std::string compiler_target)
{
  //FIXME: Use parameter for init function instead of environment variable.
  char *mode = getenv("MAGEEC_MODE");
  if (mode != NULL && 0 == strncmp(mode, "FILEML", sizeof("FILEML")))
    ml = new file_ml();
  else
    ml = new mageec_ml();

  std::cerr << "MAGEEC:  Targetting '" << compiler_version << "' for '"
       << compiler_target << "'" << std::endl;

  return ml->init (compiler_version, compiler_target);
}

void mageec_framework::new_file (std::string filename)
{
  std::cerr << "MAGEEC:  New source file " << filename << std::endl;
  featset.clear();
  assert(ml != NULL && "MAGEEC Not Initialized");
  ml->new_file();
}

void mageec_framework::end_file (void)
{
  std::cerr << "MAGEEC:  End of source file" << std::endl;
  featset.clear();
  assert(ml != NULL && "MAGEEC Not Initialized");
  ml->end_file();
}

void mageec_framework::finish (void)
{
  std::cerr << "MAGEEC:  Finish" << std::endl;
  assert(ml != NULL && "MAGEEC Not Initialized");
  ml->finish();
}

std::vector<mageec_pass*> mageec_framework::all_passes (void)
{
  assert(ml != NULL && "MAGEEC Not Initialized");
  return ml->all_passes();
}

void mageec_framework::take_features (std::string name,
                                      std::vector<mageec_feature*> features)
{
  featset[name] = features;
}

decision mageec_framework::make_decision (std::string pass,
                                          std::string function)
{
  // If we don't have a feature set, we use the compiler's decision.
  if (featset.count(function) == 0)
    return NATIVE_DECISION;

  std::vector<mageec_feature*> featureset = featset[function];
  
  mageec_pass *mpass = new basic_pass(pass);
  assert(ml != NULL && "MAGEEC Not Initialized");
  decision d = ml->make_decision(mpass, featureset);
  return d;
}
