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


namespace mageec {


/// \class ParameterBase
///
/// \brief Base class for arbitrary tunable compiler parameter types
class ParameterBase {
public:
  ParameterBase() = delete;

  /// \brief Get the type of the parameter
  ParameterType getType() const { return m_param_type; }

  /// \brief Get the compiler-specific identifier associated with a parameter
  unsigned getParameterID() const { return m_param_id; }

protected:
  /// \brief Create a parameter with the provided type and identifier
  ParameterBase(ParameterType param_type, unsigned param_id)
    : m_param_type(param_type),
      m_param_id(param_id)
  {}

private:
  ParameterType m_param_type;

  /// Compiler-dependent identifier of the specific feature
  unsigned m_param_id;
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

  Parameter(unsigned param_id, const T& value)
    : ParameterBase(param_type, param_id),
      m_value(value)
  {}

  /// \brief Get the value associated with this parameter.
  const T& getValue() const { return m_value; }

private:
  const T m_value;
};


/// Types of parameters which are supported
typedef Parameter<ParameterType::kBool, bool>      BoolParameter;
typedef Parameter<ParameterType::kRange, int64_t>  RangeParameter;


} // end of namespace mageec


#endif // MAGEEC_PARAMETER_H

