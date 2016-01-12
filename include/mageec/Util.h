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


//===------------------------- MAGEEC utilities ---------------------------===//
//
// This file provides utility classes used by MAGEEC.
//
//===----------------------------------------------------------------------===//

#ifndef MAGEEC_UTIL_H
#define MAGEEC_UTIL_H

#include <array>
#include <cassert>
#include <vector>

#include "mageec/Types.h"


namespace mageec {
namespace util {


// Debug functionality.
extern bool withDebug();
extern void setDebug(bool debug = true);

std::ostream &dbg();
std::ostream &out();

#define MAGEEC_PREFIX "-- "
#define MAGEEC_ERR(msg)    mageec::util::dbg() << MAGEEC_PREFIX "error: " << msg << '\n'
#define MAGEEC_WARN(msg)   mageec::util::dbg() << MAGEEC_PREFIX "warning: " << msg << '\n'
#define MAGEEC_STATUS(msg) mageec::util::dbg() << MAGEEC_PREFIX << msg << '\n'
#define MAGEEC_DEBUG(msg)  if (mageec::util::withDebug()) { mageec::util::dbg() << MAGEEC_PREFIX << msg << '\n'; }


/// \class Version
///
/// Simple encapsulation of a version number. A version number is divided into
/// a 3-tuple, consisting of major, minor and patch numbers, and is supposed
/// to be treated as a semantic version number.
class Version {
public:
  Version() = delete;
  Version(unsigned major, unsigned minor, unsigned patch)
    : m_major(major), m_minor(minor), m_patch(patch)
  {}

  bool operator==(const Version& other)
  {
    return ((m_major == other.m_major) &&
            (m_minor == other.m_minor) &&
            (m_patch == other.m_patch));
  }

  operator std::string(void) const {
    return std::to_string(m_major) + "." +
           std::to_string(m_minor) + "." +
           std::to_string(m_patch);
  }

  unsigned getMajor() const { return m_major; }
  unsigned getMinor() const { return m_minor; }
  unsigned getPatch() const { return m_patch; }

private:
  unsigned m_major;
  unsigned m_minor;
  unsigned m_patch;
};


/// \class Option
///
/// Simple option type, used to encapsulate an object for returning when that
/// object may not exist.
template <typename T>
class Option {
public:
  Option(void) : m_populated(false) {}

  ~Option(void)
  {
    if (m_populated) { m_value.~T(); }
  }

  Option(std::nullptr_t nullp) : m_populated(false)
  {
    (void)nullp;
  }

  Option(T obj) : m_populated(true), m_value(obj) {}

  Option(const Option& other)
    : m_populated(other.m_populated)
  {
    if (other.m_populated) {
      new (&m_value) T(other.m_value);
    }
  }

  Option(Option&& other)
    : m_populated(other.m_populated)
  {
    if (other.m_populated) {
      other.m_populated = false;
      new (&m_value) T(std::move(other.m_value));
    }
  }

  Option& operator=(const Option& other)
  {
    if (other.m_populated) {
      if (m_populated) {
        m_value = other.m_value;
      }
      else {
        new (&m_value) T(other.m_value);
        m_populated = true;
      }
    }
    else {
      if (m_populated) {
        m_value.~T();
        m_populated = false;
      }
    }
    return *this;
  }

  Option& operator=(Option&& other)
  {
    if (other.m_populated) {
      if (m_populated) {
        m_value = std::move(other.m_value);
      }
      else {
        new (&m_value) T(std::move(other.m_value));
        m_populated = true;
      }
    }
    else {
      if (m_populated) {
        m_value.~T();
        m_populated = false;
      }
    }
    return *this;
  }

  operator bool(void) const { return m_populated; }

  T operator*(void) const {
    assert(m_populated && "Option value is empty!");
    return m_value;
  }

  T get(void) const {
    assert(m_populated && "Option value is empty!");
    return m_value;
  }

private:
  bool m_populated;
  union {
    // By placing this in an anonymous union, we avoid its default constructor
    // being called when we omit it from the initializer list.
    T m_value;
  };
};


/// \class UUID
///
/// Simple encapsulation of a universal identifier
class UUID {
public:
  UUID() = delete;
  UUID(const UUID &other) = default;
  UUID(UUID &&other) = default;

  explicit UUID(std::array<uint8_t, 16> data)
    : m_data(data)
  {}

  UUID& operator=(const UUID &other) = default;
  UUID& operator=(UUID &&other) = default;

  bool operator==(const UUID& other) const { return m_data == other.m_data; }
  bool operator!=(const UUID& other) const { return m_data != other.m_data; }
  bool operator<(const UUID& other) const { return m_data < other.m_data; }

  std::array<uint8_t, 16> data(void) const { return m_data; }
  unsigned size(void) const {
    return static_cast<unsigned>(m_data.size());
  }

  /// \brief Output a UUID to a string
  ///
  /// The string is output in canonical form, ie
  /// xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx, where 'x' corresponds to a
  /// hexidecimal digit.
  operator std::string(void) const {
    std::string out;
    for (unsigned i = 0; i < 16; ++i) {
      uint8_t nibble[2] = {
        static_cast<uint8_t>( m_data[i] & 0xf),
        static_cast<uint8_t>((m_data[i] >> 4) & 0xf)
      };
      for (auto &x : nibble) {
        x = (x <= 9) ? (x + '0') : ((x - 10) + 'a');
      }
      if ((i == 4) || (i == 6) || (i == 8) || (i == 10)) {
        out = out + '-';
      }
      out = out + static_cast<char>(nibble[1]) + static_cast<char>(nibble[0]);
    }
    return out;
  }


  /// \brief Parse a UUID from an input string
  ///
  /// The string is expected to be in canonical form, with the format 
  /// xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx, where 'x' corresponds to a
  /// hexidecimal digit. Any trailing characters are ignored.
  ///
  /// \return The UUID, or an empty Option if the string could not be
  /// parsed as one.
  static Option<UUID> parse(std::string s)
  {
    if (s.length() < 36) {
      return nullptr;
    }

    std::array<uint8_t, 16> uuid;
    for (unsigned i = 0; i < 16; ++i) {
      unsigned j = (i * 2) + (i >= 4) + (i >= 6) + (i >= 8) + (i >= 10);

      uint8_t hex[2] = {
        static_cast<uint8_t>(s[j + 0]),
        static_cast<uint8_t>(s[j + 1])
      };
      for (auto &x : hex) {
        x = ((x <= '9') && (x >= '0')) ? (x - '0') :
            ((x <= 'f') && (x >= 'a')) ? (x - 'a' + 10) :
            ((x <= 'F') && (x >= 'A')) ? (x - 'A' + 10) : 255;
        if (x == 255) {
          return nullptr;
        }
      }
      uuid[i] = static_cast<uint8_t>((hex[0] << 4) | hex[1]);
    }
    if ((s[8] != '-') || (s[13] != '-') || (s[18] != '-') || (s[23] != '-')) {
      return nullptr;
    }
    return UUID(uuid);
  }


private:
  std::array<uint8_t, 16> m_data;
};


/// \brief Read a 16-bit little endian value from a byte vector, advancing
/// the iterator in the process.
///
/// It is assumed that the end of the iterator will not be encountered
/// when reading the value.
unsigned read16LE(std::vector<uint8_t>::const_iterator &it);


/// \brief Write a 16-bit little endian value to a byte vector
void write16LE(std::vector<uint8_t> &buf, unsigned value);


/// \brief Read a 64-bit little endian value from a byte vector, advancing
/// the iterator in the process.
///
/// It is assumed that the end of the iterator will not be encountered
/// when reading the value
uint64_t read64LE(std::vector<uint8_t>::const_iterator &it);


/// \brief Write a 64-bit little endian value to a byte vector
void write64LE(std::vector<uint8_t> &buf, uint64_t value);


/// \brief Calculate a CRC64 across a blob
uint64_t crc64(uint8_t *message, unsigned len);


/// \brief Convert from a metric to a string
std::string metricToString(Metric metric);


/// \brief Convert from a string to a metric if possible.
/// 
/// \return The metric if possible, or an empty Option type if not.
Option<Metric> stringToMetric(std::string metric);


} // end of namespace util
} // end of namespace mageec


#endif // MAGEEC_UTIL_H
