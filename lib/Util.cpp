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


//===-------------------------- MAGEEC utilities --------------------------===//
//
// This implements some utility methods used throughout the MAGEEC framework
//
//===----------------------------------------------------------------------===//

#include <cstdint>
#include <vector>

namespace mageec {
namespace util {


unsigned read16LE(std::vector<uint8_t>::const_iterator &it)
{
  unsigned res = 0;
  res |= static_cast<unsigned>(*it);
  res |= static_cast<unsigned>(*(it + 1)) << 8;
  return res;
}

void write16LE(std::vector<uint8_t> buf, unsigned value)
{
  buf.push_back(static_cast<uint8_t>(value));
  buf.push_back(static_cast<uint8_t>(value >> 8));
}


} // end of namespace util
} // end of namespace mageec

