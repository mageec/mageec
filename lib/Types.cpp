/*  Copyright (C) 2015, Embecosm Limited

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

//===--------------------------- MAGEEC types -----------------------------===//
//
// This file defines various enumeration types which are used by MAGEEC, and
// users of the MAGEEC library.
//
//===----------------------------------------------------------------------===//

#include "mageec/Types.h"

namespace mageec {

FeatureClass& operator++(FeatureClass &f) {
  f = static_cast<FeatureClass>(static_cast<TypeID>(f) + 1);
  return f;
}

} // end of namespace mageec

