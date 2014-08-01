/*  MAGEEC Decision State Enumeration
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

/** @file mageec-decision.h MAGEEC Decision State Enumeration */
#ifndef __MAGEEC_DECISION_H_
#define __MAGEEC_DECISION_H_

#include <string>

namespace mageec
{

/**
 * enum for storing the state of a decision.
 */
enum decision
{
  /**
   * No decision available/made so should use compiler default.
   */
  NATIVE_DECISION,
  /**
   * Decision made that pass should be executed.
   */
  FORCE_EXECUTE,
  /**
   * Decision made that pass should not be executed.
   */
  FORCE_NOEXECUTE
};

} // End namespace mageec

#endif
