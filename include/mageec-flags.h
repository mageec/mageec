/*  MAGEEC Flag Class
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

/** @file mageec-flags.h MAGEEC Flags */
#ifndef __MAGEEC_FLAGS_H
#define __MAGEEC_FLAGS_H_

namespace mageec
{
/**
 * MAGEEC Flag Base Type
 * A flag consists of a descriptor and value and are used to communicate between
 * the machine learner and the compiler which flags to use and to store
 * in a machine learning database which passes were run.
 */
class mageec_flag
{
public:
  std::string name();
  /* FIXME Is an int sufficient? */
  virtual int get_feature() = 0;
};

} // End namespace mageec

#endif