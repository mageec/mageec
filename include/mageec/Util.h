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


namespace mageec {
namespace util {


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


/// \class UUID
///
/// Simple encapsulation of a universal identifier
class UUID {
public:
  UUID() = delete;

  explicit UUID(std::array<uint8_t, 16> data)
    : m_data(data)
  {}

  bool operator==(const UUID& other) const { return m_data == other.m_data; }
  bool operator<(const UUID& other) const { return m_data < other.m_data; }

  std::array<uint8_t, 16> data(void) const { return m_data; }
  unsigned size(void) const { return m_data.size(); }

private:
  const std::array<uint8_t, 16> m_data;
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

  Option(T obj) : m_populated(true), m_value(obj) {}

  Option(const Option& other)
    : m_populated(other.m_populated)
  {
    if (m_populated) { m_value = T(other.m_value); }
  }

  Option(Option&& other)
    : m_populated(std::move(other.m_populated))
  {
    if (m_populated) { m_value = T(std::move(other.m_value)); }
  }

  Option& operator=(const Option& other)
  {
    if (m_populated) { m_value = other.m_value; }
    return *this;
  }

  Option& operator=(Option&& other)
  {
    if (m_populated) { m_value = std::move(other.m_value); }
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


} // end of namespace util
} // end of namespace mageec


#endif // MAGEEC_UTIL_H
