/*  MAGEEC GCC Plugin Callbacks
    Copyright (C) 2013-2015 Embecosm Limited and University of Bristol

    This file is part of MAGEEC.

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

//===----------------------- GCC Plugin Callbacks -------------------------===//
//
// This defines the callbacks for the GCC plugin interface
//
//===----------------------------------------------------------------------===//

// We need to undefine these as gcc-plugin.h redefines them
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION

#include <iostream>
#include <string>

#include "mageec/Decision.h"
#include "mageec/TrainedML.h"
#include "MAGEECPlugin.h"

#include "gcc-plugin.h"
#include "tree-pass.h"
#include "function.h"


using namespace mageec;


/// \grief Get string representing type of pass
///
/// \param pass  Compiler pass
/// \return String corresponding to the type of the pass (GIMPLE, RTL, etc)
static std::string passTypeString(opt_pass *pass)
{
  if (pass == NULL) {
    return "*NULL*";
  }
  switch (pass->type) {
  case GIMPLE_PASS:     return "GIMPLE";
  case RTL_PASS:        return "RTL";
  case SIMPLE_IPA_PASS: return "SIMPLE_IPA";
  case IPA_PASS:        return "IPA";
  default:
    return "*UNKNOWN*";
  }
  return "*UNKNOWN*";
}

void mageecStartFile(void *, void *)
{
  if (mageec_config["debug"]) {
    std::cerr << "-- MAGEEC: Start file" << std::endl;
  }
}

void mageecFinishFile(void *, void *)
{
  mageec_features.release();

  if (mageec_config["debug"]) {
    std::cerr << "-- MAGEEC: End file" << std::endl;
  }
}

void mageecFinish(void *gcc_data __attribute__((unused)),
                     void *user_data __attribute__((unused)))
{
  if (mageec_config["debug"]) {
    std::cerr << "-- MAGEEC: Finish" << std::endl;
  }
}

/// \brief Use MAGEEC framework to decide whether to execute a pass
void mageecPassGate(void *gcc_data, void *user_data __attribute__((unused)))
{
  if (!mageec_config["no_decision"]) {
    assert(mageec_ml && "No machine learner");
  }

  // early exit if the feature extractor has not yet run
  if (!mageec_features) {
    return;
  }

  short *result = static_cast<short *>(gcc_data);
  if (!mageec_config["no_decision"]) {
    PassGateDecisionRequest request(current_pass->name);
    std::unique_ptr<DecisionBase> decision =
        mageec_ml->makeDecision(request, *mageec_features.get());
    
    const DecisionType decision_type = decision->getType();
    if (decision_type == DecisionType::kNative) {
      return;
    }
    assert(decision_type == DecisionType::kBool);

    BoolDecision &should_run = *static_cast<BoolDecision*>(decision.get());

    // update the gate
    bool old_gate = *result;
    bool new_gate = !should_run.getValue();
    *result = new_gate;

    if (mageec_config["debug"]) {
      std::string pass_type_str = passTypeString(current_pass);
      std::cerr << "-- Updating pass '" << current_pass->name << "'" << std::endl
                << "  |- Old Gate: " << old_gate << std::endl
                << "  |- New Gate: " << new_gate << std::endl;
    }
  }
}
