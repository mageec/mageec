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
#include "Parameters.h"

#include "gcc-plugin.h"
#include "tree-pass.h"
#include "function.h"

#include <cassert>
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
  assert(0 && "Unreachable");
  return std::string();
}


static std::unique_ptr<mageec::FeatureSet>
extractModuleFeatures(
    std::map<std::string, std::unique_ptr<mageec::FeatureSet>> &func_features) {

  std::unique_ptr<mageec::FeatureSet> module_features(new mageec::FeatureSet());
  for (const auto &feature_set : func_features) {
    (void)feature_set;
    // TODO extract some features
  }
  return std::move(module_features);
}

void mageecStartFile(void *, void *) {
  // FIXME: Output filename
  MAGEEC_DEBUG("Start file '" << main_input_filename << "'");

  // Check that the feature set and pass sequence is empty
  assert(mageec_context.func_features.size() == 0);
  assert(mageec_context.func_passes.size() == 0);
}

void mageecFinishFile(void *, void *) {
  // If necessary, store the features and pass sequences in the database
  if (mageec_context.with_feature_extract) {
    MAGEEC_DEBUG("Checking if a common configuration was used for all "
                 "functions");

    // Emit this filename into the compilation id file before any module or
    // function compilation ids. This means that we can associate any modules
    // or functions with the source file which they came from, as they will
    // always be preceded by a file.
    *mageec_context.out_file << "file," << main_input_filename << "\n";

    // If the same compiler configuration was used for every function in the
    // module, then module level features can be extracted.
    bool has_common_config = true;

    // FIXME: Check that the parameter sets are equal too
    std::vector<std::string> module_passes;
    for (const auto &features : mageec_context.func_features) {
      std::string func_name = features.first;

      if (module_passes.size() == 0) {
        module_passes = mageec_context.func_passes[func_name];
      } else {
        auto &func_passes = mageec_context.func_passes[func_name];
        if (module_passes.size() != func_passes.size()) {
          has_common_config = false;
          break;
        }
        if (!std::equal(module_passes.begin(), module_passes.end(),
                        func_passes.begin())) {
          has_common_config = false;
          break;
        }
      }
    }

    // Common config for all compiler configurations, so extract module
    // level features. Module features will be inherited by each of the
    // functions in the module.
    mageec::util::Option<std::string> module_name;
    mageec::util::Option<mageec::FeatureSetID>   module_feature_set_id;
    mageec::util::Option<mageec::ParameterSetID> module_parameter_set_id;
    mageec::util::Option<mageec::CompilationID>  module_compilation_id;
    if (has_common_config) {
      MAGEEC_DEBUG("All functions have a common configuration, so extracting "
                   "module level features");

      module_name = std::string(main_input_filename);

      // Add module level features and group to the database
      std::unique_ptr<mageec::FeatureSet> module_features =
          std::move(extractModuleFeatures(mageec_context.func_features));
      module_feature_set_id =
          mageec_context.db->newFeatureSet(*module_features);

      mageec::FeatureGroupID module_feature_group_id =
          mageec_context.db->newFeatureGroup({module_feature_set_id.get()});

      // Parameter set for the module
      std::unique_ptr<mageec::ParameterSet> module_params(new mageec::ParameterSet());
      module_params->add(std::make_shared<mageec::PassSeqParameter>(ParameterID::kPassSeq, module_passes, "Sequence of passes executed"));

      module_parameter_set_id =
          mageec_context.db->newParameterSet(*module_params);

      MAGEEC_DEBUG("Storing module configuration in the database");

      // Compilation id for the module
      module_compilation_id = mageec_context.db->newCompilation(
          module_name.get(), "module", module_feature_group_id,
          module_parameter_set_id.get(), nullptr /* parent */);

      // Emit the compilation id for the module
      *mageec_context.out_file << "module," << module_name.get() << ","
                               << static_cast<uint64_t>(
                                   module_compilation_id.get())
                               << "\n";
    }

    // For each function, save the functions features, compilation parameters,
    // and the passes executed.

    // For each function, save the functions features, compilation parameters,
    // and the passes executed.
    for (const auto &features : mageec_context.func_features) {
      std::string func_name = features.first;

      // Features
      MAGEEC_DEBUG("Saving features for function '" << func_name
                                                    << "' in the database");
      mageec::FeatureSetID feature_set_id =
          mageec_context.db->newFeatureSet(*features.second);

      // If available, inherit the module level features too
      mageec::FeatureGroupID feature_group_id;
      if (has_common_config) {
        feature_group_id = mageec_context.db->newFeatureGroup(
            {feature_set_id, module_feature_set_id.get()});
      } else {
        feature_group_id = mageec_context.db->newFeatureGroup({feature_set_id});
      }

      // Parameters
      // If available, use the module level parameters
      MAGEEC_DEBUG("Saving parameters for function '" << func_name
                                                      << "' in the database");
      mageec::ParameterSetID parameter_set_id;
      if (has_common_config) {
        MAGEEC_DEBUG("Parameters for function '" << func_name
                  << "' inherited from parent module '" << module_name.get()
                  << "'");
        parameter_set_id = module_parameter_set_id.get();
      } else {
        assert(mageec_context.func_passes.count(func_name) &&
               "Function with no pass sequence");
        auto passes = mageec_context.func_passes[func_name];

        std::unique_ptr<mageec::ParameterSet> params(new mageec::ParameterSet());
        params->add(std::make_shared<mageec::PassSeqParameter>(ParameterID::kPassSeq, passes, "Sequence of passes executed"));
        parameter_set_id = mageec_context.db->newParameterSet(*params);
      }

      // Create the compilation for this function
      // Add the module as a parent if it has a compilation
      MAGEEC_DEBUG("Creating compilation for function '" << func_name << "'");
      mageec::CompilationID compilation_id;
      if (has_common_config) {
        compilation_id = mageec_context.db->newCompilation(
            func_name, "function", feature_group_id, parameter_set_id,
            module_compilation_id.get());
      } else {
        compilation_id = mageec_context.db->newCompilation(
          func_name, "function", feature_group_id, parameter_set_id,
          nullptr /* parent */);
      }

      // Emit the compilation id for the function into the output file
      *mageec_context.out_file << "function," << func_name << ","
                               << static_cast<uint64_t>(compilation_id) << "\n";
    }
  }

  // Discard the current feature set and pass sequence, ready for the next
  // file.
  mageec_context.func_features.clear();
  mageec_context.func_passes.clear();

  // FIXME: Output filename
  MAGEEC_DEBUG("End file '" << main_input_filename << "'");
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
