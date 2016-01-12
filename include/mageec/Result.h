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

#include "mageec/FeatureSet.h"
#include "mageec/ParameterSet.h"
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

  /// \brief Get the initial features of the program unit
  FeatureSet getFeatures(void) const {
    return m_features;
  }

  /// \brief Get the global parameters of the compilation
  ParameterSet getParameters(void) const {
    return m_parameters;
  }

  /// \brief Get the name of the pass at a position in the pass sequence
  std::string getPassName(unsigned i) const {
    assert (i < m_pass_sequence.size() && "Index out of bounds!");
    return m_pass_sequence[i].name;
  }

  /// \brief Get the parameters for the pass at a position in the pass sequence
  ParameterSet getPassParameters(unsigned i) const {
    assert (i < m_pass_sequence.size() && "Index out of bounds!");
    return m_pass_sequence[i].parameters;
  }

  /// \brief Get the features extracted after the pass at the provided position
  /// in the pass sequence
  util::Option<FeatureSet> getFeaturesAfterPass(unsigned i) const {
    assert (i < m_pass_sequence.size() && "Index out of bounds!");
    if (i >= m_pass_features.size()) {
      return util::Option<FeatureSet>();
    }
    return m_pass_features[i];
  }

  /// \brief Get the numerical value of the result
  uint64_t getValue(void) const {
    return m_value;
  }

private:
  struct PassConfiguration {
    /// \brief Name used to identify the pass
    std::string name;

    /// \brief Parameters used by the pass
    ParameterSet parameters;
  };


  /// \brief Construct a new result with the provided initial features,
  /// compilation parameters and result value.
  ///
  /// This is restricted to be accessible by friend classes. User code should
  /// never be creating results
  Result(FeatureSet features, ParameterSet parameters, uint64_t value)
    : m_features(features), m_parameters(parameters),
      m_pass_sequence(), m_pass_features(),
      m_value(value)
  {}

  /// \brief Add a new pass to the pass sequence for the results.
  ///
  /// \param name  Identifier of the pass as used in the database
  /// \param parameters  Parameters for the provided pass
  /// \return The index of the newly added 
  unsigned addPass(std::string name, ParameterSet parameters)
  {
    unsigned pos = static_cast<unsigned>(m_pass_sequence.size());
    m_pass_sequence.push_back({name, parameters});
    return pos;
  }

  /// \brief Add features extracted after the provided pass in the pass
  /// sequence.
  ///
  /// \param pos  Position of the pass in the pass sequence
  /// \param features  Complete set of features extracted after the pass was
  /// run.
  void addFeatures(unsigned pos, FeatureSet features)
  {
    assert(pos < m_pass_sequence.size());
    m_pass_features.resize(pos + 1);
    m_pass_features[pos] = features;
  }

  /// \brief Initial features for the program unit
  FeatureSet m_features;

  /// \brief Parameters applicable to the overall compilation of the program
  /// unit.
  ParameterSet m_parameters;

  /// \brief Sequence of passes executed and their configurations
  std::vector<PassConfiguration> m_pass_sequence;

  /// \brief Features extracted after a pass in the pass configuration
  std::vector<util::Option<FeatureSet> > m_pass_features;

  /// \brief Value of the result
  uint64_t m_value;
};


} // end of namespace mageec


#endif // MAGEEC_RESULT_H
