/*  MAGEEC GCC Plugin
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

//===-------------------- GCC Plugin Interface ----------------------------===//
//
// Interface between GCC and the MAGEEC framework, for feature extraction and
// the gating of passes.
//
//===----------------------------------------------------------------------===//

// We need to undefine these as gcc-plugin.h redefines them
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION

#include <map>
#include <string>

#include "mageec/FeatureSet.h"
#include "mageec/Framework.h"
#include "mageec/TrainedML.h"
#include "mageec/Util.h"
#include "MAGEECPlugin.h"

#include "gcc-plugin.h"
#include "tree-pass.h"


/// Declared to allow GCC to load the plugin
int plugin_is_GPL_compatible;


#if !defined(MAGEEC_GCC_PLUGIN_VERSION_MAJOR) || \
    !defined(MAGEEC_GCC_PLUGIN_VERSION_MINOR) || \
    !defined(MAGEEC_GCC_PLUGIN_VERSION_PATCH)
#error "MAGEEC GCC plugin version not defined by the build system"
#endif

static_assert(MAGEEC_GCC_PLUGIN_VERSION_MAJOR == 2 &&
              MAGEEC_GCC_PLUGIN_VERSION_MINOR == 0 &&
              MAGEEC_GCC_PLUGIN_VERSION_PATCH == 0,
              "MAGEEC GCC plugin version does not match");

/// Plugin information to pass to GCC
#define QUOTE(name) #name
#define STRINGIFY(macro) QUOTE(macro)
static struct plugin_info mageec_plugin_version =
{
  .version = STRINGIFY(MAGEEC_GCC_PLUGIN_VERSION_MAJOR) "."
             STRINGIFY(MAGEEC_GCC_PLUGIN_VERSION_MINOR) "."
             STRINGIFY(MAGEEC_GCC_PLUGIN_VERSION_PATCH),
  .help = NULL
};
#undef QUOTE
#undef STRINGIFY


/// Stored GCC Plugin name for future register_callbacks.
const char *mageec_gcc_plugin_name;

/// Context shared by all components of the plugin
MAGEECContext mageec_context;



/// \brief Parse arguments provided to the plugin
static bool parseArguments(int argc, struct plugin_argument *argv)
{
  assert(!mageec_context.is_init);
  assert(mageec_context.framework &&
         "Cannot parse plugin arguments with uninitialized framework");

  mageec::util::Option<std::string> ml_str;
  mageec::util::Option<std::string> db_str;

  bool with_plugin_info = false;
  bool with_print_machine_learners = false;
  bool with_feature_extract = false;
  bool with_optimize = false;
  bool with_save_features = false;

  bool seen_debug = false;
  bool seen_mode  = false;
  bool seen_machine_learner = false;
  bool seen_database = false;
  bool seen_save_features = false;

  for (int i = 0; i < argc; ++i) {
    std::string arg_str = argv[i].key;

    if (arg_str == "debug" || arg_str == "no_debug") {
      if (seen_debug) {
        MAGEEC_ERR("Plugin argument 'debug' already seen");
        return false;
      }

      if (arg_str == "debug") {
        if (argv[i].value) {
          std::string arg_value = argv[i].value;
          if (arg_value == "true") {
            mageec_context.with_debug = true;
          }
          else if (arg_value == "false") {
            mageec_context.with_debug = false;
          }
        }
        else {
          mageec_context.with_debug = true;
        }
      }
      else if (arg_str == "no_debug") {
        mageec_context.with_debug = false;
      }
      seen_debug = true;
    }

    else if (arg_str == "mode") {
      if (seen_mode) {
        MAGEEC_ERR("Plugin argument 'mode' already seen");
        return false;
      }
      if (!argv[i].value) {
        MAGEEC_ERR("No 'mode' argument value provided");
        return false;
      }

      std::string arg_value = argv[i].value;
      
      if (arg_value == "plugin_info") {
        with_plugin_info = true;
      }
      else if (arg_value == "print_machine_learners") {
        with_print_machine_learners = true;
      }
      else if (arg_value == "feature_extract") {
        with_feature_extract = true;
      }
      else if (arg_value == "optimize") {
        with_feature_extract = true;
        with_optimize = true;
      }
      else {
        MAGEEC_ERR("Unknown 'mode' for plugin: " << arg_value);
        return false;
      }
      seen_mode = true;
    }

    else if (arg_str == "machine_learner") {
      if (seen_machine_learner) {
        MAGEEC_ERR("Plugin argument 'machine_learner' already seen");
        return false;
      }
      if (!argv[i].value) {
        MAGEEC_ERR("Missing 'machine_learner' argument value");
        return false;
      }
      ml_str = std::string(argv[i].value);
      seen_machine_learner = true;
    }

    else if (arg_str == "database") {
      if (seen_database) {
        MAGEEC_ERR("Plugin argument 'database' already seen");
        return false;
      }
      if (!argv[i].value) {
        MAGEEC_ERR("Missing 'database' argument value");
        return false;
      }
      db_str = std::string(argv[i].value);
      seen_database = true;
    }

    else if (arg_str == "save_features" || arg_str == "no_save_features") {
      if (seen_save_features) {
        MAGEEC_ERR("Plugin argument 'save_features' already seen");
        return false;
      }

      if (arg_str == "save_features") {
        if (argv[i].value) {
          std::string arg_value = argv[i].value;
          if (arg_value == "true") {
            with_save_features = true;
          }
          else if (arg_value == "false") {
            with_save_features = false;
          }
        }
        else {
          with_save_features = true;
        }
      }
      if (arg_str == "no_save_features") {
        with_save_features = false;
      }
      seen_save_features = true;
    }
    else {
      MAGEEC_ERR("Unknown argument to plugin: " << arg_str);
      return false;
    }
  }

  // Default mode
  if (!seen_mode) {
    MAGEEC_WARN("No mode specified, defaulting to 'feature_extract'");
    with_feature_extract = true;
  }

  // Determine the mode we are running in
  if (with_plugin_info) {
    mageec_context.mode.reset(new MAGEECMode(MAGEECMode::kPluginInfo));
  }
  else if (with_print_machine_learners) {
    mageec_context.mode.reset(
        new MAGEECMode(MAGEECMode::kPrintMachineLearners));
  }
  else {
    if (with_feature_extract && !with_save_features && !with_optimize) {
      mageec_context.mode.reset(
          new MAGEECMode(MAGEECMode::kFeatureExtract));
    }
    else if (with_feature_extract && with_save_features && !with_optimize) {
      mageec_context.mode.reset(
          new MAGEECMode(MAGEECMode::kFeatureExtractAndSave));
    }
    else if (with_feature_extract && !with_save_features && with_optimize) {
      mageec_context.mode.reset(
          new MAGEECMode(MAGEECMode::kFeatureExtractAndOptimize));
    }
    else if (with_feature_extract && with_save_features && with_optimize) {
      mageec_context.mode.reset(
          new MAGEECMode(MAGEECMode::kFeatureExtractSaveAndOptimize));
    }
    else {
      assert(0 && "Invalid mode flags");
    }
  }

  // Sanity checks
  assert(mageec_context.mode);
  assert(seen_machine_learner == (ml_str == true));
  assert(seen_database == (db_str == true));

  // Warnings
  MAGEECMode mode = *mageec_context.mode;
  if (seen_save_features) {
    if ((mode == MAGEECMode::kPluginInfo) ||
        (mode == MAGEECMode::kPrintMachineLearners)) {
      MAGEEC_WARN("Ignored argument 'save_features'");
    }
  }
  if (seen_database) {
    if ((mode == MAGEECMode::kPluginInfo) ||
        (mode == MAGEECMode::kPrintMachineLearners) ||
        (mode == MAGEECMode::kFeatureExtract)) {
      MAGEEC_WARN("Ignored argument 'database'");
    }
  }
  if (seen_machine_learner) {
    if ((mode == MAGEECMode::kPluginInfo) ||
        (mode == MAGEECMode::kPrintMachineLearners) ||
        (mode == MAGEECMode::kFeatureExtract) ||
        (mode == MAGEECMode::kFeatureExtractAndSave)) {
      MAGEEC_WARN("Ignored argument 'machine_learner'");
    }
  }

  // Errors
  if ((mode == MAGEECMode::kFeatureExtractAndSave) ||
      (mode == MAGEECMode::kFeatureExtractSaveAndOptimize)) {
    if (!seen_database) {
      MAGEEC_ERR("Cannot save features without a database");
      return false;
    }
  }
  if ((mode == MAGEECMode::kFeatureExtractAndOptimize) ||
      (mode == MAGEECMode::kFeatureExtractSaveAndOptimize)) {
    if (!seen_machine_learner) {
      MAGEEC_ERR("Cannot optimize without a machine learner");
      return false;
    }
  }
  if (mode == MAGEECMode::kFeatureExtractAndOptimize) {
    if (!seen_database) {
      MAGEEC_ERR("Cannot optimize without a database");
      return false;
    }
  }


  // Load in the appropriate database if required.
  // TODO: Allow this to create a new database
  if ((mode == MAGEECMode::kFeatureExtractAndSave) ||
      (mode == MAGEECMode::kFeatureExtractAndOptimize) ||
      (mode == MAGEECMode::kFeatureExtractSaveAndOptimize)) {
    assert(seen_database);
    mageec_context.database =
        mageec_context.framework->getDatabase(db_str.get(), false);
    assert(mageec_context.database);
  }

  // If we are in a mode which requires a machine learner, try and parse the
  // machine learner string as a UUID.
  mageec::util::Option<mageec::util::UUID> ml_uuid;
  if ((mode == MAGEECMode::kFeatureExtractAndOptimize) ||
      (mode == MAGEECMode::kFeatureExtractSaveAndOptimize)) {
    assert(seen_machine_learner);
    assert(mageec_context.database);
    ml_uuid = mageec::util::UUID::parse(ml_str.get());

    // Not a UUID, try and load as a shared object
    if (!ml_uuid) {
      ml_uuid = mageec_context.framework->loadMachineLearner(ml_str.get());
    }

    if (!ml_uuid) {
      MAGEEC_ERR("Unable to load machine learner: " << ml_str);
      return false;
    }
    assert(ml_uuid);
    
    // Get all of the trained machine learners in the database
    // Find the machine learner we are interested in save it in the context
    std::vector<mageec::TrainedML> machine_learners =
        mageec_context.database->getTrainedMachineLearners();

    bool ml_found = false;
    for (auto &ml : machine_learners) {
      if (ml.getUUID() == ml_uuid.get()) {
        mageec_context.machine_learner.reset(new mageec::TrainedML(ml));
        ml_found = true;
        break;
      }
    }
    if (!ml_found) {
      MAGEEC_ERR("The provided machine learner has no training data in this "
                 "database");
      return false;
    }
    assert(mageec_context.machine_learner);
  }

  assert(mageec_context.mode);
  mageec_context.is_init = true;
  return true;
}


/// \brief Initialize the GCC Plugin
///
/// This initializes the call backs used by GCC to interact with the plugin
///
/// \param plugin_name_args GCC plugin information
/// \param plugin_gcc_version GCC version information
///
/// \return 0 if MAGEEC successfully set up, 1 otherwise.
int plugin_init (struct plugin_name_args *plugin_info,
                 struct plugin_gcc_version *version)
{
  // Initialize the MAGEEC framework
  assert(!mageec_context.framework);
  mageec_context.framework.reset(new mageec::Framework());

  // Register our information, parse arguments
  register_callback(plugin_info->base_name, PLUGIN_INFO, NULL,
                     &mageec_plugin_version);
  mageec_gcc_plugin_name = plugin_info->base_name;

  bool res = parseArguments(plugin_info->argc, plugin_info->argv);
  if (!res) {
    MAGEEC_ERR("Error when parsing plugin arguments");
    return 1;
  }

  if (*mageec_context.mode == MAGEECMode::kPluginInfo) {
    mageecPluginInfo(plugin_info, version);
    return 0;
  }

  register_callback(plugin_info->base_name, PLUGIN_START_UNIT,
                    mageecStartFile, NULL);
  register_callback(plugin_info->base_name, PLUGIN_FINISH_UNIT,
                    mageecFinishFile, NULL);
  register_callback(plugin_info->base_name, PLUGIN_FINISH,
                    mageecFinish, NULL);
  register_callback(plugin_info->base_name, PLUGIN_OVERRIDE_GATE,
                    mageecPassGate, NULL);

  mageecRegisterFeatureExtract();
  return 0;
}
