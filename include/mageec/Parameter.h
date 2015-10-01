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


//===----------------------------- Parameter ------------------------------===//
//
// This file defines the available parameter types. A parameter is one of the
// components which forms the overall compiler configuration, and the machine
// learner can make decisions about parameters in order to change the behaviour
// of the compiler optimization.
//
//===----------------------------------------------------------------------===//

#ifndef MAGEEC_PARAMETER_H
#define MAGEEC_PARAMETER_H

#include "mageec/Types.h"

#include <cassert>
#include <memory>
#include <string>


namespace mageec {


/// \class ParameterBase
///
/// \brief Base class for arbitrary tunable compiler parameter types
class ParameterBase {
public:
  ParameterBase() = delete;

  // vtable may be emitted multiple times
  virtual ~ParameterBase() {}

  /// \brief Get the type of the parameter
  ParameterType getType() const { return m_param_type; }

  /// \brief Get the compiler-specific identifier associated with a parameter
  unsigned getParameterID() const { return m_param_id; }

  /// \brief Get the string identifier associated with a parameter
  std::string getName() const { return m_name; }

  /// \brief Get the value of the parameter as a blob of bytes
  virtual std::vector<uint8_t> toBlob() const = 0;

protected:
  /// \brief Create a parameter with the provided type and identifier
  ParameterBase(ParameterType param_type, unsigned param_id, std::string name)
    : m_param_type(param_type),
      m_param_id(param_id),
      m_name(name)
  {}

private:
  /// The MAGEEC name of the type of the parameter
  ParameterType m_param_type;

  /// Compiler-dependent identifier of the specific feature
  unsigned m_param_id;

  /// String identifier of the specific parameter, used for debug purposed
  std::string m_name;
};


/// \class Parameter
///
/// \brief Specific parameter type
///
/// Associates a given enumeration value for the parameter type with the
/// appropriate type of value.
template<ParameterType param_type, typename T>
class Parameter : public ParameterBase {
public:
  typedef T value_type;

  Parameter() = delete;
  ~Parameter() {}

  Parameter(unsigned param_id, const T& value, std::string name)
    : ParameterBase(param_type, param_id, name),
      m_value(value)
  {}

  /// \brief Get the value associated with this parameter.
  const T& getValue() const { return m_value; }

  /// \brief Convert the held parameter type to a binary blob
  std::vector<uint8_t> toBlob(void) const override {
    static_assert(std::is_integral<T>::value,
                  "Only integral types handled for now");

    std::vector<uint8_t> blob;
    blob.reserve(sizeof(T));

    const uint8_t *value = reinterpret_cast<const uint8_t *>(&m_value);
    for (unsigned i = 0; i < sizeof(T); ++i) {
      blob.push_back(value[i]);
    }
    return blob;
  }

  /// \brief Create a parameter of this type given the provided id, blob and
  /// name
  static std::unique_ptr<Parameter>
  fromBlob(unsigned parameter_id, std::vector<uint8_t> blob, std::string name)
  {
    static_assert(std::is_integral<T>::value,
                  "Only integral types handled for now");
    assert(blob.size() == sizeof(T));

    const T *value = reinterpret_cast<const T *>(blob.data());
    return std::unique_ptr<Parameter>(
        new Parameter(parameter_id, *value, name));
  }

private:
  const T m_value;
};


/// Types of parameters which are supported
typedef Parameter<ParameterType::kBool, bool>      BoolParameter;
typedef Parameter<ParameterType::kRange, int64_t>  RangeParameter;


} // end of namespace mageec


#endif // MAGEEC_PARAMETER_H

