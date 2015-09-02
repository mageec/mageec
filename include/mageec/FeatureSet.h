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


//===---------------------------- Feature Set -----------------------------===//
//
// A set of features, there may be no duplicates of a given feature id
// within a set.
//
//===----------------------------------------------------------------------===//

#ifndef MAGEEC_FEATURE_SET_H
#define MAGEEC_FEATURE_SET_H

#include <memory>
#include <set>
#include <utility>

#include "mageec/Feature.h"
#include "mageec/Types.h"


namespace mageec {


class FeatureBase;


class FeatureSet {
private:
  /// \brief Function object to sort features by comparing their ids
  struct FeatureIDComparator {
    bool operator() (FeatureBase* const &lhs,
                     FeatureBase* const &rhs) const {
      return lhs->getFeatureID() < rhs->getFeatureID();
    }
  };

public:
  /// \brief constant iterator to features in the set
  typedef std::set<FeatureBase*,
                   FeatureIDComparator>::const_iterator const_iterator;


  /// \brief Construct a new empty feature set
  FeatureSet() : m_features() {}

  FeatureSet(const FeatureSet& other) = default;
  FeatureSet(FeatureSet&& other) = default;

  FeatureSet& operator=(const FeatureSet& other) {
    for (FeatureBase* feature : m_features) {
      delete feature;
    }
    m_features = other.m_features;
    return *this;
  }
  FeatureSet& operator=(FeatureSet&& other) {
    for (FeatureBase* feature : m_features) {
      delete feature;
    }
    m_features = std::move(other.m_features);
    return *this;
  }

  /// \brief Delete a feature set
  ~FeatureSet() {
    // Delete the features which we took ownership of
    for (FeatureBase* feature : m_features) {
      delete feature;
    }
  }

  /// \brief Add a new feature to a feature set
  ///
  /// The FeatureSet takes ownership of the provided feature
  ///
  /// \param feature  The feature to be added to the set
  void addFeature(std::unique_ptr<FeatureBase> feature) {
    assert (feature != nullptr && "Cannot add null feature to feature set");

    // The set takes ownership of the provided feature
    const auto& result = m_features.emplace(feature.release());
    assert (result.second && "Added feature already present in the set!");
  }

  /// \brief Get a constant iterator to the beginning of the feature set
  const_iterator cbegin() const { return m_features.cbegin(); }

  /// \brief Get a constant iterator to the end of the feature set
  const_iterator cend() const { return m_features.cend(); }

private:
  std::set<FeatureBase*, FeatureIDComparator> m_features;
};


} // end of namespace mageec


#endif // MAGEEC_FEATURE_SET_H
