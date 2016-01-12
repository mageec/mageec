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

//===------------------------ File Machine Learner ------------------------===//
//
// This defines the file based machine learner for MAGEEC. FileML does
// not make any decisions of its own, instead relying on the user to
// predefine the decisions that should be made.
//
// As the user provides the decisions, it does not need to store anything
// in the database.
//
//===----------------------------------------------------------------------===//

#ifndef MAGEEC_FILE_ML_H
#define MAGEEC_FILE_ML_H

#include "mageec/ML.h"
#include "mageec/Result.h"
#include "mageec/Types.h"
#include "mageec/Util.h"

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace mageec {

class DecisionRequestBase;
class FeatureSet;

/// \class FileML
///
/// \brief Machine learner which makes decisions based on a predefined
/// file of user input features.
class FileML : public IMachineLearner {
private:
  static const util::UUID uuid;

public:
  FileML();
  ~FileML() override;

  /// \brief Get the unique identifier for this machine learner
  util::UUID getUUID(void) const override { return uuid; }

  /// \brief Get the name of the machine learner
  std::string getName(void) const override { return "File Machine Learner"; }

  bool requiresTraining(void) const override { return false; }

  bool requiresTrainingConfig(void) const override { return false; }
  bool setTrainingConfig(std::string) override {
    assert(0 && "FileML should not be provided a training config");
    return false;
  }

  bool requiresDecisionConfig(void) const override { return true; }
  bool setDecisionConfig(std::string) override;

  std::unique_ptr<DecisionBase>
  makeDecision(const DecisionRequestBase &request, const FeatureSet &features,
               const std::vector<uint8_t> &blob) const override;

  const std::vector<uint8_t> train(std::set<FeatureDesc> feature_descs,
                                   std::set<ParameterDesc> parameter_descs,
                                   std::set<std::string> passes,
                                   ResultIterator results) const override;

  const std::vector<uint8_t>
  train(std::set<FeatureDesc> feature_descs,
        std::set<ParameterDesc> parameter_descs, std::set<std::string> passes,
        ResultIterator results, std::vector<uint8_t> old_blob) const override;

private:
  /// Set when the a decision config has been provided to the machine
  /// learner. This is required before any decisions can be made.
  bool m_have_decision_config;

  /// Maps from a decision request identifier to the user provided result.
  /// This is parsed in from the decision making config file.
  std::map<std::string, std::string> m_decision_map;
};

} // end of namespace mageec

#endif // MAGEEC_FILE_ML_H
