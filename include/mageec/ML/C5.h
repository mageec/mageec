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

//===------------------------ MAGEEC C5.0 Driver --------------------------===//
//
// This defines a machine learner interface which uses a driver to drive an
// external C5.0 machine learner. It calls out to the C5.0 command line in
// order to make decisions and train.
//
//===----------------------------------------------------------------------===//

#ifndef MAGEEC_C5_DRIVER_H
#define MAGEEC_C5_DRIVER_H

#include "mageec/AttributeSet.h"
#include "mageec/ML.h"
#include "mageec/Result.h"
#include "mageec/Types.h"
#include "mageec/Util.h"

#include <cstdint>
#include <set>
#include <string>
#include <vector>

namespace mageec {

class DecisionRequestBase;

/// \class C5Driver
///
/// \brief Machine learner which drives an external C5.0 classifier
class C5Driver : public IMachineLearner {
public:
  C5Driver();
  ~C5Driver() override;

  /// \brief Get the name of the machine learner
  ///
  /// Note that this is a driver in case a C5.0 classifier is later included
  /// with MAGEEC.
  std::string getName(void) const override { return "c50"; }

  bool requiresTraining(void) const override { return true; }

  bool requiresTrainingConfig(void) const override { return false; }
  bool setTrainingConfig(std::string) override {
    assert(0 && "C5.0 should not be provided a training config");
    return false;
  }
  bool requiresDecisionConfig(void) const override { return false; }
  bool setDecisionConfig(std::string) override {
    assert(0 && "C5.0 should not be provided a decision config");
    return false;
  }

  std::unique_ptr<DecisionBase>
  makeDecision(const DecisionRequestBase &request, const FeatureSet &features,
               const std::vector<uint8_t> &blob) const override;

  const std::vector<uint8_t> train(std::set<FeatureDesc> feature_descs,
                                   std::set<ParameterDesc> parameter_descs,
                                   std::set<std::string> passes,
                                   ResultIterator results) const override;
};

} // end of namespace mageec

#endif // MAGEEC_C5_DRIVER_H
