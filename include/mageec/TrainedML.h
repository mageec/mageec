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


//===----------------------- Trained Machine Learner-----------------------===//
//
// This contains the implementation of a trained machine learner, which
// provides access to training data held within the database, as well as the
// interface to the individual machine learner implementations.
//
//===----------------------------------------------------------------------===//

#ifndef MAGEEC_TRAINED_ML_H
#define MAGEEC_TRAINED_ML_H

#include <memory>
#include <string>

#include "mageec/Decision.h"
#include "mageec/Types.h"

#include "sqlite3.h"


namespace mageec {


class FeatureBase;
class IMachineLearner;


/// \class TrainedML
///
/// Interface to a trained machine learner in a database. Used to make
/// decisions about the compiler configuration.
class TrainedML {
public:
  TrainedML() = delete;

  /// \brief Construct a trained machine learner based on an underlying
  /// machine learner implementation.
  ///
  /// \param db  A handle to the underlying sqlite database
  /// \param ml  Handle to the interface of the machine learner. There must be
  /// an entry for this machine learner in the database for the provided
  /// metric.
  /// \param metric  The metric this machine learner has been trained against
  /// \param blob  A blob of training data to be passed to the machine
  /// learner when making a decision
  TrainedML(sqlite3& db,
            const IMachineLearner& ml,
            Metric metric,
            const std::vector<uint8_t> blob);

  /// \brief Get the name of the underlying machine learner interface
  std::string getName(void) const;

  /// \brief Get the metric which this machine learner was trained against
  Metric getMetric(void) const;

  /// \brief Make a single decision using the machine learner interfaces
  ///
  /// This forwards a request to the underlying machine learner to make a
  /// decision, based on the input parameters, as well as the training blob
  /// stored in the database for this machine learner.
  ///
  /// \param request  The request made to the machine learner
  /// \param features  The features which the machine learner uses to make its
  /// decision.
  ///
  /// \return The decision made. If for any reason the machine learner cannot
  /// make a decision, this will be the native decision.
  std::unique_ptr<DecisionBase>
  makeDecision(const DecisionRequestBase& request,
               const std::vector<FeatureBase*> features);

private:
  /// Database containing this trained machine learner.
  sqlite3& m_db;

  /// Interface to the underlying machine learner.
  const IMachineLearner& m_ml;

  /// Metric which this machine learner is trained for.
  const Metric m_metric;

  /// Blob of training data for this machine learner
  const std::vector<uint8_t> m_blob;
};


} // end of namespace mageec


#endif // MAGEEC_TRAINED_ML_H
