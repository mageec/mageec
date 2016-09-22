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

//===------------------------ Trained Machine Learner ---------------------===//
//
// The contains the implementation of a trained machine learner, which
// provides access to training data held within the datbase, as well as th
// interface to the individual machine learner implementations.
//
//===----------------------------------------------------------------------===//

#include "mageec/AttributeSet.h"
#include "mageec/Decision.h"
#include "mageec/ML.h"
#include "mageec/TrainedML.h"
#include "mageec/Types.h"
#include "mageec/Util.h"

#include "sqlite3.h"

#include <memory>
#include <string>
#include <vector>

namespace mageec {

TrainedML::TrainedML(IMachineLearner &ml) : m_ml(ml), m_metric(), m_blob() {
  assert(!ml.requiresTraining() &&
         "Machine learner requires training, so it must be initialized with "
         "a metric and blob");
}

TrainedML::TrainedML(IMachineLearner &ml, std::string metric,
                     const std::vector<uint8_t> blob)
    : m_ml(ml), m_metric(metric), m_blob(blob) {
  assert(ml.requiresTraining() && "Machine learner does not require training, "
                                  "where did the metric and blob come from?");
}

util::UUID TrainedML::getUUID(void) const { return m_ml.getUUID(); }

std::string TrainedML::getName(void) const { return m_ml.getName(); }

std::string TrainedML::getMetric(void) const {
  assert(m_ml.requiresTraining() && "Machine learner does not require "
                                    "training. So it has no training metric");
  return m_metric.get();
}

bool TrainedML::requiresDecisionConfig() const {
  return m_ml.requiresDecisionConfig();
}

bool TrainedML::setDecisionConfig(std::string config_path) {
  assert(requiresDecisionConfig() &&
         "Cannot provide decision config to "
         "a machine learner which does not require one");
  return m_ml.setDecisionConfig(config_path);
}

std::unique_ptr<DecisionBase>
TrainedML::makeDecision(const DecisionRequestBase &request,
                        const FeatureSet &features) {
  return m_ml.makeDecision(request, features, m_blob);
}

void TrainedML::print(std::ostream &os) const {
  os << std::string(getUUID()) << " " << getName();
  if (m_metric) {
    os << " " << getMetric();
  }
  os << "\n";
}

} // end of namespace mageec
