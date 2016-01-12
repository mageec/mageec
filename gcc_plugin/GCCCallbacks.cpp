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

#include "mageec/Decision.h"
#include "mageec/TrainedML.h"
#include "MAGEECPlugin.h"

#include "gcc-plugin.h"
#include "tree-pass.h"
#include "function.h"

#include <iostream>
#include <string>

/// \brief Get string representing type of pass
///
/// \param pass  Compiler pass
/// \return String corresponding to the type of the pass (GIMPLE, RTL, etc)
static std::string passTypeString(opt_pass *pass) {
  if (pass == NULL) {
    return "*NULL*";
  }
  switch (pass->type) {
  case GIMPLE_PASS:
    return "GIMPLE";
  case RTL_PASS:
    return "RTL";
  case SIMPLE_IPA_PASS:
    return "SIMPLE_IPA";
  case IPA_PASS:
    return "IPA";
  }
}

void mageecStartFile(void *, void *) {
  // FIXME: Output filename
  MAGEEC_DEBUG("Start file");

  // Check that the feature set and pass sequence is empty
  assert(mageec_context.func_features.size() == 0);
  assert(mageec_context.func_passes.size() == 0);
}

void mageecFinishFile(void *, void *) {
  // If necessary, store the features and pass sequences in the database
  if (mageec_context.with_feature_extract) {
    // For each function, save the functions features, compilation parameters,
    // and the passes executed.
    for (const auto &features : mageec_context.func_features) {
      // Features
      MAGEEC_DEBUG("Saving features for function '" << features.first
                                                    << "' in the database");
      mageec::FeatureSetID feature_set_id =
          mageec_context.db->newFeatureSet(*features.second);
      mageec::FeatureGroupID feature_group_id =
          mageec_context.db->newFeatureGroup({feature_set_id});

      // Parameters
      // FIXME: Using an empty parameter set for now
      MAGEEC_DEBUG("Saving parameters for function '" << features.first
                                                      << "' in the database");
      std::unique_ptr<mageec::ParameterSet> params(new mageec::ParameterSet());
      mageec::ParameterSetID parameter_set_id =
          mageec_context.db->newParameterSet(*params);

      // Pass sequence
      MAGEEC_DEBUG("Saving pass sequence for function '"
                   << features.first << "' in the database");
      assert(mageec_context.func_passes.count(features.first) &&
             "Function with no pass sequence");
      std::vector<std::string> pass_seq =
          mageec_context.func_passes[features.first];

      mageec::PassSequenceID pass_sequence_id =
          mageec_context.db->newPassSequence(pass_seq);

      // Create the  compilation for this execution run
      mageec_context.db->newCompilation(features.first, "function",
                                        feature_group_id, parameter_set_id,
                                        pass_sequence_id, nullptr /* parent */);
    }
  }

  // Discard the current feature set and pass sequence, ready for the next
  // file.
  mageec_context.func_features.clear();
  mageec_context.func_passes.clear();

  // FIXME: Output filename
  MAGEEC_DEBUG("End file");
}

void mageecFinish(void *gcc_data __attribute__((unused)),
                  void *user_data __attribute__((unused))) {
  MAGEEC_DEBUG("Finish");
}

/// \brief Use MAGEEC framework to decide whether to execute a pass
void mageecPassGate(void *gcc_data, void *user_data __attribute__((unused))) {
  if (mageec_context.with_optimize) {
    assert(mageec_context.ml && "Missing machine learner");
  }

  // Function whose passes we are gating
  std::string func_name = current_function_name();

  // early exit if the feature extractor has not yet run for this function.
  if (mageec_context.func_features.count(func_name) == 0) {
    return;
  }

  short *result = static_cast<short *>(gcc_data);
  if (mageec_context.with_optimize) {
    mageec::PassGateDecisionRequest request(current_pass->name);

    mageec::TrainedML &ml = *mageec_context.ml.get();
    std::unique_ptr<mageec::DecisionBase> decision =
        ml.makeDecision(request, *mageec_context.func_features[func_name]);

    const mageec::DecisionType decision_type = decision->getType();
    if (decision_type != mageec::DecisionType::kNative) {
      assert(decision_type == mageec::DecisionType::kBool);

      mageec::BoolDecision &should_run =
          *static_cast<mageec::BoolDecision *>(decision.get());

      // update the gate
      bool old_gate = *result;
      bool new_gate = !should_run.getValue();
      *result = new_gate;

      std::string pass_type_str = passTypeString(current_pass);

      MAGEEC_DEBUG("Updating pass '" << current_pass->name << "' for function '"
                                     << func_name << "'");
      if (mageec::util::withDebug()) {
        mageec::util::dbg() << " |- Old Gate: " << old_gate << '\n'
                            << " |- New Gate: " << new_gate << '\n';
      }
    }
  }

  // If we are saving features, then we also want to save the sequence of
  // passes for the function as they are run.
  if (mageec_context.with_feature_extract) {
    if (*result) {
      mageec_context.func_passes[func_name].push_back(current_pass->name);
    }
  }
}
