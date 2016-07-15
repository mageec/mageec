/*  MAGEEC GCC Features
    Copyright (C) 2015 Embecosm Limited

    This file is part of MAGEEC

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

//===----------------------- MAGEEC GCC Features --------------------------===//
//
// This file defines various enumeration types used by the GCC feature
// extractor.
//
//===----------------------------------------------------------------------===//

#ifndef MAGEEC_GCC_PARAMETERS_H
#define MAGEEC_GCC_PARAMETERS_H

namespace ParameterID {
enum : unsigned {
  kPassSeq = 0,
};
} // end of namespace ParameterID

#endif // MAGEEC_GCC_PARAMETERS_H
