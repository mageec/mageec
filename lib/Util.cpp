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
#include <iostream>
#include <vector>

#include "mageec/Util.h"


namespace mageec {
namespace util {


std::ostream &dbg() { return std::cerr; }
std::ostream &out() { return std::cout; }


unsigned read16LE(std::vector<uint8_t>::const_iterator &it)
{
  unsigned res = 0;
  res |= static_cast<unsigned>(*it);
  res |= static_cast<unsigned>(*(it + 1)) << 8;
  return res;
}

void write16LE(std::vector<uint8_t> &buf, unsigned value)
{
  buf.push_back(static_cast<uint8_t>(value));
  buf.push_back(static_cast<uint8_t>(value >> 8));
}

uint64_t read64LE(std::vector<uint8_t>::const_iterator &it)
{
  unsigned res = 0;
  res |= static_cast<uint64_t>(*it);
  res |= static_cast<uint64_t>(*(it + 1)) <<  8;
  res |= static_cast<uint64_t>(*(it + 2)) << 16;
  res |= static_cast<uint64_t>(*(it + 3)) << 24;
  res |= static_cast<uint64_t>(*(it + 4)) << 32;
  res |= static_cast<uint64_t>(*(it + 5)) << 40;
  res |= static_cast<uint64_t>(*(it + 6)) << 48;
  res |= static_cast<uint64_t>(*(it + 7)) << 56;
  return res;
}

void write64LE(std::vector<uint8_t> &buf, uint64_t value)
{
  buf.push_back(static_cast<uint8_t>(value));
  buf.push_back(static_cast<uint8_t>(value >>  8));
  buf.push_back(static_cast<uint8_t>(value >> 16));
  buf.push_back(static_cast<uint8_t>(value >> 24));
  buf.push_back(static_cast<uint8_t>(value >> 32));
  buf.push_back(static_cast<uint8_t>(value >> 40));
  buf.push_back(static_cast<uint8_t>(value >> 48));
  buf.push_back(static_cast<uint8_t>(value >> 56));
}


/// \brief Simple CRC implementation
///
/// Based on crc32b from Hacker's Delight
/// (http://www.hackersdelight.org/hdcodetxt/crc.c.txt)
/// Expanded to support crc64 and nulls by Simon Cook
uint64_t crc64(uint8_t *message, unsigned len)
{
   unsigned i;
   int j;
   uint64_t byte, crc, mask;

   i = 0;
   crc = 0xFFFFFFFFFFFFFFFFULL;
   while (i < len) {
      byte = message[i];            // Get next byte.
      crc = crc ^ byte;
      for (j = 7; j >= 0; j--) {    // Do eight times.
         mask = -(crc & 1);
         crc = (crc >> 1) ^ (0xC96C5795D7870F42ULL & mask);
      }
      i = i + 1;
   }
   return ~crc;
}


} // end of namespace util
} // end of namespace mageec

