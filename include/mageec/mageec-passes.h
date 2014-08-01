/*  MAGEEC Pass Class
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

/** @file mageec-passes.h MAGEEC Passes */
#ifndef __MAGEEC_PASSES_H_
#define __MAGEEC_PASSES_H_

#include <string>

namespace mageec
{
/**
 * MAGEEC Pass Base Type
 * A pass consists of a descriptor and value and are used to communicate between
 * the machine learner and the compiler which passes to use and to store
 * in a machine learning database which passes were run.
 */
class mageec_pass
{
public:
  virtual ~mageec_pass();

  virtual std::string name() = 0;
  /* FIXME Is an int sufficient? */
  virtual int value() = 0;
};

class basic_pass : public mageec_pass
{
  std::string pass_name;
public:
  basic_pass(std::string name);
  std::string name();
  int value();
};

} // End namespace mageec

#endif
