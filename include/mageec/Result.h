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

//===--------------------------- MAGEEC results ---------------------------===//
//
// Holds the definition of MAGEEC results, which are extracted from the
// database and provided to the machine learner when training.
//
//===----------------------------------------------------------------------===//

#ifndef MAGEEC_RESULT_H
#define MAGEEC_RESULT_H

#include "mageec/AttributeSet.h"
#include "mageec/Types.h"
#include "mageec/Util.h"

#include <cassert>
#include <string>
#include <vector>

namespace mageec {

class ResultIterator;

class Result {
  // ResultIterator is responsible for creating each result on demand
  friend class ResultIterator;

public:
  Result() = delete;

  /// \brief Get the features of the program unit
  const FeatureSet& getFeatures(void) const { return m_features; }

  /// \brief Get the parameters of the compilation
  const ParameterSet& getParameters(void) const { return m_parameters; }

  /// \brief Get the numerical value of the result
  double getValue(void) const { return m_value; }

private:
  /// \brief Construct a new result with the provided initial features,
  /// compilation parameters and result value.
  ///
  /// This is restricted to be accessible by friend classes. User code should
  /// never be creating results
  Result(FeatureSet features, ParameterSet parameters, double value)
      : m_features(features), m_parameters(parameters), m_value(value) {}

  /// \brief Features for the program unit
  FeatureSet m_features;

  /// \brief Parameters applicable to the overall compilation of the program
  /// unit.
  ParameterSet m_parameters;

  /// \brief Value of the result
  double m_value;
};

} // end of namespace mageec

#endif // MAGEEC_RESULT_H
