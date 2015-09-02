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


//===-------------------------- Program Features --------------------------===//
//
// This contains the definition of the various types of MAGEEC feature which
// quantify some aspect of a program unit or its compilation. Features are
// extracted by a feature extractor, and used as input to the machine learner
// to choose a configuration of the compiler from an input program.
//
//===----------------------------------------------------------------------===//

#ifndef MAGEEC_FEATURES_H
#define MAGEEC_FEATURES_H

#include "mageec/Types.h"


namespace mageec {


/// \class FeatureBase
///
/// \brief Base class for arbitrary feature types
class FeatureBase {
public:
  FeatureBase() = delete;

  /// \brief Get the type of the feature
  FeatureType getType() const { return m_feature_type; }

  /// \brief Get the compiler-specific identifier associated with a feature
  unsigned getFeatureID() const { return m_feature_id; }

protected:
  /// \brief Create a feature with the provided type and identifier
  FeatureBase(FeatureType feature_type, unsigned feature_id)
    : m_feature_type(feature_type),
      m_feature_id(feature_id)
  {}

private:
  /// The MAGEEC name of the type of the feature
  FeatureType m_feature_type;

  /// Compiler-dependent identifier of the specific feature
  unsigned m_feature_id;
};


/// \class Feature
///
/// \brief Represents a specific type of feature
///
/// Associates a given enumeration value for the feature type with the
/// appropriate type of value.
template<FeatureType feature_type, typename T>
class Feature : public FeatureBase {
public:
  typedef T value_type;

  Feature() = delete;

  Feature(unsigned feature_id, const T& value)
    : FeatureBase(feature_type, feature_id),
      m_value(value)
  {}

  /// \brief Get the value associated with this feature.
  const T& getValue() const { return m_value; }

private:
  const T m_value;
};


/// Types of features which are supported by MAGEEC
typedef Feature<FeatureType::kBool, bool>    BoolFeature;
typedef Feature<FeatureType::kInt,  int64_t> IntFeature;


} // end of namespace mageec


#endif // MAGEEC_FEATURES_H
