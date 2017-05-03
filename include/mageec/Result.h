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

/// \class Result
///
/// \brief A single result as provided to a machine learner during training
///
/// This class encapsulates a single result value, as well as the FeatureSet of
/// the program unit which was compiled and the ParameterSet which the
/// program unit was compiled with to produce that value.
class Result {
  // ResultIterator is responsible for retrieving each result on demand
  friend class ResultIterator;

public:
  Result() = delete;

  /// \brief Get the features of the program unit
  ///
  /// \return The FeatureSet of the program unit which produced the result
  const FeatureSet& getFeatures(void) const { return m_features; }

  /// \brief Get the parameters of the compilation
  ///
  /// \return The ParameterSet the program unit was compiled with to
  /// produce the given result value
  const ParameterSet& getParameters(void) const { return m_parameters; }

  /// \brief Get the value of the result as as double
  double getValue(void) const { return m_value; }

private:
  /// \brief Construct a new result with the provided initial features,
  /// compilation parameters and result value.
  ///
  /// This is restricted to be accessible by friend classes. User code should
  /// never be creating results
  Result(FeatureSet features, ParameterSet parameters, double value)
      : m_features(features), m_parameters(parameters), m_value(value) {}

  /// Features for the program unit which produced the result value
  FeatureSet m_features;

  /// Parameters which were used to compile the program unit which produced
  /// the defined result value
  ParameterSet m_parameters;

  /// Value of the result
  double m_value;
};

} // end of namespace mageec

#endif // MAGEEC_RESULT_H
