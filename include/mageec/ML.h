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

//===---------------------- Machine learner interface ---------------------===//
//
// This contains the interface for a machine learner which may be used by
// MAGEEC to make decisions about the compiler configuration, and train a
// machine learner using results data.
//
//===----------------------------------------------------------------------===//

#ifndef MAGEEC_ML_H
#define MAGEEC_ML_H

#include "mageec/Attribute.h"
#include "mageec/Decision.h"
#include "mageec/Result.h"
#include "mageec/Util.h"

#include <string>
#include <vector>

namespace mageec {

class DecisionRequestBase;

/// \class IMachineLearner
///
/// Abstract interface to a machine learner.
class IMachineLearner {
public:
  virtual ~IMachineLearner(void) = 0;

  /// \brief Get the unique identifier for this machine learner
  virtual util::UUID getUUID(void) const = 0;

  /// \brief Get the name of this machine learner as a string
  virtual std::string getName(void) const = 0;

  /// \brief Return whether this machine learner must be trained before
  /// making a decision.
  virtual bool requiresTraining(void) const = 0;

  /// \brief Return whether this machine learner should be provided with
  /// a path to a configuration file for training.
  virtual bool requiresTrainingConfig(void) const = 0;

  /// \brief Set the training configuration to be used by this machine
  /// learner.
  ///
  /// It is an error to call this method if requiresTrainingConfig
  /// returns false.
  ///
  /// \param config_path Path to the training configuration file
  ///
  /// \return True if the config was set successfully.
  virtual bool setTrainingConfig(std::string config_path) = 0;

  /// \brief Return whether this machine learner should be provided with
  /// a path to a configuration file for decision making
  virtual bool requiresDecisionConfig(void) const = 0;

  /// \brief Set the decision making configuration to be used by this
  /// machine learner.
  ///
  /// It is an error to call this method if requiresDecisionConfig
  /// returns false.
  ///
  /// \param config_path Path to the decision configuration file
  ///
  /// \return True if the config was set successfully.
  virtual bool setDecisionConfig(std::string config_path) = 0;

  /// \brief Get the type of decisions which can be requested of this machine
  /// learner.
  ///
  /// \return A set containing the supported types of decisions that may be
  /// requested.
  /// TODO: Support this
  // std::set<DecisionRequestType> getSupportedDecisions(void) const = 0;

  /// \brief Make a single decision based on a provided request, the features
  /// of a program unit, and a blob of training data for the machine learner.
  ///
  /// \param request  The decision to be made
  /// \param features  A set of features to be used by the machine learner to
  /// make the decision.
  /// \param blob  A blob of training data appropriate to the machine learner.
  /// this should never be nullptr
  ///
  /// \return The resultant decision, which is either the appropriate
  /// corresponding decision for the input request, or the native decision if
  /// a decision could not be made.
  virtual std::unique_ptr<DecisionBase>
  makeDecision(const DecisionRequestBase &request, const FeatureSet &features,
               const std::vector<uint8_t> &blob) const = 0;

  // TODO: machine learner training should be allowed to error

  /// \brief Train the machine learner using a complete set of provided
  /// results.
  ///
  /// This produces a training blob which can be subsequently used by the
  /// machine learner to make decisions.
  ///
  /// \param feature_descs  All of the feature ids and their types used in the
  /// results data
  /// \param parameter_descs  All of the parameter ids and their types used in
  /// the results data
  /// \param passes  All of the passes used in the results data
  /// \param results  Iterator to the results data
  ///
  /// \return A blob of training data.
  virtual const std::vector<uint8_t>
  train(std::set<FeatureDesc> feature_descs,
        std::set<ParameterDesc> parameter_descs, std::set<std::string> passes,
        ResultIterator results) const = 0;

  /// \brief Incrementally train the machine learner, using a partial set of
  /// results, and the previously constructed blob.
  ///
  /// This produces a new training blob which replaces the old and is used
  /// to make decisions.
  ///
  /// \param feature_descs  All of the feature ids and their types used in the
  /// results data
  /// \param parameter_descs  All of the parameter ids and their types used in
  /// the results data
  /// \param passes  All of the passes used in the results data
  /// \param results  Iterator to the new results data
  /// \param old_blob  The old blob of training data to be replaced
  ///
  /// \return A new blob of training data.
  virtual const std::vector<uint8_t>
  train(std::set<FeatureDesc> feature_descs,
        std::set<ParameterDesc> parameter_descs, std::set<std::string> passes,
        ResultIterator results, std::vector<uint8_t> old_blob) const = 0;
};

inline IMachineLearner::~IMachineLearner() {}

} // end of namespace MAGEEC

#endif // MAGEEC_ML_H
