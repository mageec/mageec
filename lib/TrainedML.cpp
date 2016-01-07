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

#include "mageec/TrainedML.h"

#include <memory>
#include <string>
#include <vector>

#include "mageec/Feature.h"
#include "mageec/FeatureSet.h"
#include "mageec/Decision.h"
#include "mageec/ML.h"
#include "mageec/Types.h"
#include "mageec/Util.h"

#include "sqlite3.h"


namespace mageec {


TrainedML::TrainedML(sqlite3 &db,
                     IMachineLearner &ml,
                     Metric metric,
                     const std::vector<uint8_t> blob)
  : m_db(db), m_ml(ml), m_metric(metric), m_blob(blob)
{
  (void)m_db;
}

util::UUID TrainedML::getUUID(void) const
{
  return m_ml.getUUID();
}

std::string TrainedML::getName(void) const
{
  return m_ml.getName();
}

Metric TrainedML::getMetric(void) const
{
  return m_metric;
}

bool TrainedML::requiresDecisionConfig() const
{
  return m_ml.requiresDecisionConfig();
}

bool TrainedML::setDecisionConfig(std::string config_path)
{
  assert(requiresDecisionConfig() && "Cannot provide decision config to "
         "a machine learner which does not require one");
  return m_ml.setDecisionConfig(config_path);
}

std::unique_ptr<DecisionBase>
TrainedML::makeDecision(const DecisionRequestBase& request,
                        const FeatureSet& features)
{
  return m_ml.makeDecision(request, features, m_blob);
}


} // end of namespace mageec
