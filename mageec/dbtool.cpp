/*  MAGEEC Database Trainer
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

/** @file dbtool.cpp MAGEEC Database Tool.
    This tool will be used for manipulating MAGEEC databases. For now it only
    drives the MAGEEC learning process once a database is created.  */
#include "mageec/mageec.h"
#include <iostream>

using namespace mageec;

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " filename" << std::endl;
    return 1;
  }

  /* Load up specified database and learn from results. */
  mageec_ml learner;
  int loaded = learner.init(argv[1]);
  if (loaded != 0) {
    std::cerr << "Unable to load database `" << argv[1] << std::endl;
    return 1;
  }
  learner.process_results();
  learner.finish();
  return 0;
}
