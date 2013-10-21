/*  MAGEEC Main Interface
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

/** @file mageec.h MAGEEC Main Interface.
 * This is used by MAGEEC plugins to interface with the MAGEEC framework.
 */
#ifndef __MAGEEC_H__
#define __MAGEEC_H__

#include <string>
#include "mageec-features.h"
#include "mageec-ml.h"

/// MAchine Guided Energy Efficient Compilation
namespace mageec
{

/**
 * MAGEEC Framework
 */
class mageec_framework
{
  mageec::mageec_ml ml;

public:
  /**
   * Initilizes the MAGEEC Machine Learner.
   * @param compiler_version Compiler and version, e.g. GCC-4.8.
   * @param compiler_target Compiler target, e.g. arm-none-gnueabi.
   * @returns 0 if MAGEEC successfully set up, 1 otherwise.
   */
  int init (std::string compiler_version,
            std::string compiler_target);

  /**
   * Informs the framework (and machine learner) that a new file has
   * been loaded.
   * @param filename Name of file being compiled (for logging purposes).
   */
  void new_file (std::string filename);

  /**
   * Informs the framework (and machine learner) that the current
   * file has finished processing.
   */
  void end_file (void);

  /**
   * Disconnects MAGEEC from Machine Learner.
   */
  void finish (void);

  std::vector<mageec_flag*> all_flags (void);
};

} // End mageec namespace

#endif
