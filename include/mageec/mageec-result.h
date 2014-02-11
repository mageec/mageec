/*  MAGEEC Machine Learner Result
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

/** @file mageec-result.h MAGEEC Result Structure */
#ifndef __MAGEEC_RESULT_H__
#define __MAGEEC_RESULT_H__

#include <stdint.h>

namespace mageec
{
/**
 * Machine Learning Test Result Structure
 */
struct result
{
  std::vector<mageec_pass*> passlist;
  std::vector<mageec_feature*> featlist;
  std::string progname;
  int64_t metric;
};
}

#endif