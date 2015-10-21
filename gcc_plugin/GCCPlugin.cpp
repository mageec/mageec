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

#include <iostream>
#include <map>
#include <string>

#include "mageec/FeatureSet.h"
#include "mageec/Framework.h"
#include "mageec/TrainedML.h"
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

// Framework instance, and the trained machine learner to use when making
// decisions.
std::unique_ptr<mageec::Framework> mageec_framework;
std::unique_ptr<mageec::TrainedML> mageec_ml;

// Configuration of MAGEEC
std::map<std::string, int> mageec_config;

// Last feature set extracted
std::unique_ptr<mageec::FeatureSet> mageec_features;


/// \brief Parse arguments provided to the plugin
static void parseArguments(int argc, struct plugin_argument *argv)
{
  for (int i = 0; i < argc; ++i) {
    if (!strcmp(argv[i].key, "plugin_info")) {
      mageec_config["plugin_info"] = 1;
    }
    else if (!strcmp(argv[i].key, "debug")) {
      mageec_config["debug"] = 1;
    }
    else if (!strcmp(argv[i].key, "no_decision")) {
      mageec_config["no_decision"] = 1;
    }
    else {
      std::cerr << "MAGEEC Warning: Unknown option " << argv[i].key
                << std::endl;
    }
  }
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
  // Initialize options
  mageec_config["debug"] = 0;
  mageec_config["plugin_info"] = 0;
  mageec_config["no_decision"] = 0;

  // Register our information, parse arguments
  register_callback(plugin_info->base_name, PLUGIN_INFO, NULL,
                     &mageec_plugin_version);
  mageec_gcc_plugin_name = plugin_info->base_name;
  parseArguments(plugin_info->argc, plugin_info->argv);

  if (mageec_config["plugin_info"]) {
    mageecPluginInfo(plugin_info, version);
  }

  // Initialize the MAGEEC framework
  mageec_framework.reset(new mageec::Framework());

  // TODO: Get all trained machine learners
  // Select a machine learner based on plugin input
  // etc

  mageec_ml.reset(nullptr);

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
