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

#include "mageec/AttributeSet.h"
#include "mageec/Framework.h"
#include "mageec/ML.h"
#include "mageec/ML/C5.h"
#include "mageec/ML/FileML.h"
#include "mageec/TrainedML.h"
#include "mageec/Util.h"
#include "MAGEECPlugin.h"

#include "gcc-plugin.h"
#include "tree-pass.h"

#include <fstream>
#include <map>
#include <string>

/// Declared to allow GCC to load the plugin
int plugin_is_GPL_compatible;

/// Init version of the plugin
const mageec::util::Version
    mageec_plugin_version(MAGEEC_GCC_PLUGIN_VERSION_MAJOR,
                          MAGEEC_GCC_PLUGIN_VERSION_MINOR,
                          MAGEEC_GCC_PLUGIN_VERSION_PATCH);

/// Stored GCC Plugin name for future register_callbacks.
const char *mageec_gcc_plugin_name;

/// Context shared by all components of the plugin
MAGEECContext mageec_context;

/// Plugin information to pass to GCC
#define QUOTE(name) #name
#define STRINGIFY(macro) QUOTE(macro)

struct MAGEECPluginInfo {
  MAGEECPluginInfo() {}

  operator struct plugin_info() {
    struct plugin_info info;
    info.version = STRINGIFY(MAGEEC_GCC_PLUGIN_VERSION_MAJOR) "."
                   STRINGIFY(MAGEEC_GCC_PLUGIN_VERSION_MINOR) "."
                   STRINGIFY(MAGEEC_GCC_PLUGIN_VERSION_PATCH);
    info.help = "For help, pass -help as an argument to the plugin";
    return info;
  }
};
static struct plugin_info mageec_plugin_info = MAGEECPluginInfo();

#undef QUOTE
#undef STRINGIFY

/// \brief Print help information to stdout
static void printHelp()
{
  mageec::util::out() << 
"MAGEEC plugin for GCC. This allows GCC to interact with the MAGEEC\n"
"framework.\n"
"\n"
"options:\n"
"  -help                  Print this help information\n"
"  -plugin-info           Print information about the plugin\n"
"  -version               Print the version of the GCC plugin\n"
"  -framework-version     Print the version of the MAGEEC framework\n"
"  -debug                 Enable debug output from the plugin and framework\n"
"  -database=<arg>        Database to be used to store extracted features,\n"
"                         and which contains training data for machine\n"
"                         learners\n"
"  -database-version      Print the version of the provided database\n"
"  -print-ml-interfaces   Print the interfaces registered with the MAGEEC\n"
"                         framework\n"
"  -print-mls             Print the machine learners which are able to\n"
"                         make decisions about the compilation configuration\n"
"  -feature-extract       Features are extracted and stored in the provided\n"
"                         database\n"
"  -optimize              The provided machine learner will be used to make\n"
"                         decisions about the compiler configuration\n"
"  -ml=<arg>              UUID or shared object identifying a machine\n"
"                         learner interface to be used\n"
"  -ml-config=<arg>       Configuration to be provided to the machine learner\n"
"                         for decision making\n"
"  -metric=<arg>          Metric for the machine learner\n"
"  -out-file=<arg>        The output file records identifiers for each\n"
"                         element of the input program. These identifiers are\n"
"                         then provided to the framework when adding results\n"
"\n"
"examples:\n"
"  gcc -fplugin=libmageec_gcc.so -fplugin-libmageec_gcc-help foo.c\n"
"\n"
"  gcc -fplugin=libmageec_gcc.so -fplugin-libmageec_gcc-database=foo.db\n"
"      -fplugin-libmageec_gcc-feature-extract foo.c\n";
}

/// \brief Print information about the plugin to stdout
///
/// \param plugin_info  GCC plugin information
/// \param version      GCC version information
static void printPluginInfo(struct plugin_name_args *plugin_info,
                            struct plugin_gcc_version *version)
{
  mageec::util::out() <<
MAGEEC_PREFIX "MAGEEC plugin information\n"
"|- base_name: " << plugin_info->base_name << "\n"
"|- full_name: " << plugin_info->full_name << "\n"
"|- #args:     " << plugin_info->argc << "\n"
"|- version:   " << plugin_info->version << "\n"
"|- help:      " << plugin_info->help << "\n"
MAGEEC_PREFIX "GCC information\n"
"|- basever:   " << version->basever << "\n"
"|- datestamp: " << version->datestamp << "\n"
"|- devphase:  " << version->devphase << "\n"
"|- revision:  " << version->revision << "\n"
"|- confargs:  " << version->configuration_arguments << "\n";
}

/// \brief Print out the plugin version
static void printPluginVersion()
{
  mageec::util::out() << MAGEEC_PREFIX "Plugin version: "
      << static_cast<std::string>(mageec_plugin_version) << '\n';
}

/// \brief Print the framework version
static void printFrameworkVersion(const mageec::Framework &framework)
{
  mageec::util::out() << MAGEEC_PREFIX "Framework version: "
      << static_cast<std::string>(framework.getVersion()) << '\n';
}

/// \brief Print information about the available machine learner
/// interfaces to stdout
///
/// \param mls The interfaces to be printed
static void printMLInterfaces(std::set<mageec::IMachineLearner *> mls) {
  for (const auto *ml : mls) {
    mageec::util::out() << ml->getName() << '\n'
                        << static_cast<std::string>(ml->getUUID()) << "\n\n";
  }
}

/// \brief Print information about usable machine learners to stdout
///
/// \param mls The machine learners which are trained and ready for use.
static void printMLs(std::vector<mageec::TrainedML> mls) {
  for (auto &ml : mls) {
    mageec::util::out() << ml.getName() << '\n'
                        << static_cast<std::string>(ml.getUUID()) << '\n'
                        << mageec::util::metricToString(ml.getMetric())
                        << "\n\n";
  }
}

/// \brief Print the database version number to stdout
static void printDatabaseVersion(mageec::Database &db) {
  mageec::util::out() << MAGEEC_PREFIX "Database version: "
      << static_cast<std::string>(db.getVersion()) << '\n';
}

/// \brief Parse arguments provided to the plugin
///
/// FIXME: This method is rather large an unweildy as we mutate
/// the configuration of the framework and context all over the place.
/// It needs to be factored out to make it more manageable.
static bool parseArguments(struct plugin_name_args *plugin_info,
                           struct plugin_gcc_version *version) {
  assert(!mageec_context.is_init);
  assert(mageec_context.framework &&
         "Cannot parse plugin arguments with uninitialized framework");

  int argc = plugin_info->argc;
  struct plugin_argument *argv = plugin_info->argv;

  mageec::util::Option<std::string> db_str;
  mageec::util::Option<std::string> ml_str;
  mageec::util::Option<std::string> ml_config_str;
  mageec::util::Option<mageec::Metric> metric;
  mageec::util::Option<std::string> out_file_str;

  // Simple flags
  bool with_help                = false;
  bool with_plugin_info         = false;
  bool with_version             = false;
  bool with_framework_version   = false;
  bool with_debug               = false;
  bool with_db_version          = false;
  bool with_print_ml_interfaces = false;
  bool with_print_mls           = false;
  bool with_feature_extract     = false;
  bool with_optimize            = false;

  // Flags with arguments
  bool with_db        = false;
  bool with_ml        = false;
  bool with_ml_config = false;
  bool with_metric    = false;
  bool with_out_file  = false;

  for (int i = 0; i < argc; ++i) {
    std::string arg_str = argv[i].key;

    // Simple flags
    if (arg_str == "help") {
      if (argv[i].value) {
        MAGEEC_ERR("Plugin argument 'help' does not take a value");
        return false;
      }
      with_help = true;
    } else if (arg_str == "plugin-info") {
      if (argv[i].value) {
        MAGEEC_ERR("Plugin argument 'plugin-info' does not take a value");
        return false;
      }
      with_plugin_info = true;
    } else if (arg_str == "version") {
      if (argv[i].value) {
        MAGEEC_ERR("Plugin argument 'version' does not take a value");
        return false;
      }
      with_version = true;
    } else if (arg_str == "framework-version") {
      if (argv[i].value) {
        MAGEEC_ERR("Plugin argument 'framework-version' does not take a value");
        return false;
      }
      with_framework_version = true;
    } else if (arg_str == "debug") {
      if (argv[i].value) {
        MAGEEC_ERR("Plugin argument 'debug' does not take a value");
        return false;
      }
      with_debug = true;
    } else if (arg_str == "database-version") {
      if (argv[i].value) {
        MAGEEC_ERR("Plugin argument 'database-version' does not take a value");
        return false;
      }
      with_db_version = true;
    } else if (arg_str == "print-ml-interfaces") {
      if (argv[i].value) {
        MAGEEC_ERR("Plugin argument 'print-ml-interfaces' does not take a "
                   "value");
        return false;
      }
      with_print_ml_interfaces = true;
    } else if (arg_str == "print-mls") {
      if (argv[i].value) {
        MAGEEC_ERR("Plugin argument 'print-mls' does not take a value");
        return false;
      }
      with_print_mls = true;
    } else if (arg_str == "feature-extract") {
      if (argv[i].value) {
        MAGEEC_ERR("Plugin argument 'feature-extract' does not take a value");
        return false;
      }
      with_feature_extract = true;
    } else if (arg_str == "optimize") {
      if (argv[i].value) {
        MAGEEC_ERR("Plugin argument 'optimize' does not take a value");
        return false;
      }
      with_optimize = true;
    }

    // Flags with arguments
    else if (arg_str == "database") {
      if (with_db) {
        MAGEEC_ERR("Plugin argument 'database' already seen");
        return false;
      }
      if (!argv[i].value) {
        MAGEEC_ERR("No value provided to 'database' argument");
        return false;
      }
      db_str = std::string(argv[i].value);
      with_db = true;
    } else if (arg_str == "ml") {
      if (with_ml) {
        MAGEEC_ERR("Plugin argument 'ml' already seen");
        return false;
      }
      if (!argv[i].value) {
        MAGEEC_ERR("No value provided to 'ml' argument");
        return false;
      }
      ml_str = std::string(argv[i].value);
      with_ml = true;
    } else if (arg_str == "ml-config") {
      if (with_ml_config) {
        MAGEEC_ERR("Plugin argument 'ml-config' already seen");
        return false;
      }
      if (!argv[i].value) {
        MAGEEC_ERR("No value provided to 'ml-config' argument");
        return false;
      }
      ml_config_str = std::string(argv[i].value);
      with_ml_config = true;
    } else if (arg_str == "metric") {
      if (with_metric) {
        MAGEEC_ERR("Plugin argument 'metric' already seen");
        return false;
      }
      if (!argv[i].value) {
        MAGEEC_ERR("No value provided to 'metric' argument");
        return false;
      }
      std::string metric_str = argv[i].value;
      metric = mageec::util::stringToMetric(argv[i].value);
      if (!metric) {
        MAGEEC_ERR("Invalid metric value specified '" << metric_str << "'");
        return false;
      }
      with_metric = true;
    } else if (arg_str == "out-file") {
      if (with_out_file) {
        MAGEEC_ERR("Plugin argument 'out-file' already seen");
        return false;
      }
      if (!argv[i].value) {
        MAGEEC_ERR("No value provided to 'out-file' argument");
        return false;
      }
      out_file_str = std::string(argv[i].value);
      with_out_file = true;
    } else {
      MAGEEC_WARN("Unrecognized argument '" << arg_str << "' ignored");
    }
  }

  // Enable debug now that parsing arguments is finished.
  mageec_context.framework->setDebug(with_debug);

  // Sanity checks
  assert(with_ml == (ml_str == true));
  assert(with_db == (db_str == true));
  assert(with_ml_config == (ml_config_str == true));

  bool may_require_db = with_feature_extract || with_print_mls ||
                        with_optimize || with_db_version;
  bool may_use_ml = with_print_mls || with_print_ml_interfaces || with_optimize;

  // Warnings
  if (with_db && !may_require_db) {
    MAGEEC_WARN("Database provided but no option specified which requires "
                "a database");
  }
  if (with_ml_config && !with_ml) {
    MAGEEC_WARN("Configuration for machine learner provided but no machine "
                "learner was specified");
  }
  if (with_out_file && !with_feature_extract) {
    MAGEEC_WARN("Output file for compilation ids specified, but no features "
                "will be saved to the database");
  }

  // FIXME: ignored arguments?

  // Errors
  if (with_db_version && !with_db) {
    MAGEEC_ERR("Cannot version for database without a provided database");
    return false;
  }
  if (with_feature_extract && !with_db) {
    MAGEEC_ERR("Cannot feature extract without a database to save features "
               "to");
    return false;
  }
  if (with_optimize && !with_ml) {
    MAGEEC_ERR("Cannot optimize without specifying a machine learner");
    return false;
  }
  if (with_feature_extract && !with_out_file) {
    MAGEEC_ERR("Cannot feature extract without somewhere to output compilation"
               "ids");
    return false;
  }

  // Print plugin information
  if (with_plugin_info) {
    printPluginInfo(plugin_info, version);
  }

  // Print help information
  if (with_help) {
    printHelp();
  }

  // Print plugin version
  if (with_version) {
    printPluginVersion();
  }

  // Print framework version
  if (with_framework_version) {
    printFrameworkVersion(*mageec_context.framework.get());
  }

  // If we are optimizing, or querying machine learner interfaces or
  // trained machine learners, and we have a machine learner argument, try
  // and parse it as a UUID.
  mageec::util::Option<mageec::util::UUID> ml_uuid;
  if (may_use_ml && with_ml) {
    assert(ml_str);

    ml_uuid = mageec::util::UUID::parse(ml_str.get());
    if (!ml_uuid) {
      // Not a UUID, try and load as a shared object
      ml_uuid = mageec_context.framework->loadMachineLearner(ml_str.get());
    }
    if (!ml_uuid) {
      MAGEEC_ERR("Unable to load provided machine learner: " << ml_str);
      return false;
    }
    assert(ml_uuid);
  }

  // If printing the interfaces or optimizing, get the machine learners.
  // For optimizing, we need to know whether the user needs to specify
  // a metric to the machine learner. We also need to know whether the
  // machine learner needs the database to make decisions.
  bool ml_requires_db = false;
  bool ml_requires_metric = false;
  if (with_print_ml_interfaces || with_optimize) {
    std::set<mageec::IMachineLearner *> ml_interfaces =
        mageec_context.framework->getMachineLearners();

    // Print the machine learner interfaces
    if (with_print_ml_interfaces) {
      printMLInterfaces(ml_interfaces);
    }

    // Check if the user must specify a metric to the machine learner
    // An interface which requires training also requires a database,
    // and a metric to be specified by the user.
    for (auto ml : ml_interfaces) {
      if ((ml->getUUID() == ml_uuid.get()) && ml->requiresTraining()) {
        ml_requires_metric = true;
        ml_requires_db = true;
      }
    }
    if (ml_requires_metric && !with_metric) {
      MAGEEC_ERR("The provided machine learner requires a metric, but "
                 "one was not specified");
      return false;
    } else if (!ml_requires_metric && with_metric) {
      MAGEEC_ERR("Machine learner does not require a metric, but one was "
                 "specified");
      return false;
    }
  }

  if (ml_requires_db && !with_db) {
    MAGEEC_ERR("Machine learners requires database to make decisions, but "
               "a database was not provided");
    return false;
  }

  // Now we know whether a database is required we can load it.
  if ((with_feature_extract || ml_requires_db || with_print_mls) && with_db) {
    assert(db_str);
    mageec_context.db =
        mageec_context.framework->getDatabase(db_str.get(), false);
    assert(mageec_context.db);

    // Print the database version now that it is loaded
    if (with_db_version) {
      printDatabaseVersion(*mageec_context.db.get());
    }
  }

  // If we are optimizing, or querying for trained machine learners, then
  // retrieve them from the database if it is available.
  std::vector<mageec::TrainedML> trained_mls;
  if ((with_print_mls || with_optimize) && with_db) {
    trained_mls = mageec_context.db->getTrainedMachineLearners();
  }
  // There are also some machine learners which don't need training, and for
  // these we can create a trained machine learner directly.
  // FIXME: May be better for this to be in the framework.
  for (const auto ml : mageec_context.framework->getMachineLearners()) {
    if (!ml->requiresTraining()) {
      trained_mls.push_back(mageec::TrainedML(*ml));
    }
  }

  // Print out the trained machine learners if desired
  if (with_print_mls) {
    printMLs(trained_mls);
  }

  // If we are optimizing, find the trained machine learner specified earlier
  // which has the desired UUID and (if required) metric.
  if (with_optimize) {
    bool ml_found = false;
    for (auto &ml : trained_mls) {
      // UUID or metric doesn't match, ignore
      if (ml.getUUID() != ml_uuid.get()) {
        continue;
      }
      if (ml_requires_metric && (ml.getMetric() != metric.get())) {
        continue;
      }

      // We've found the right trained machine learner
      mageec_context.ml.reset(new mageec::TrainedML(ml));

      // Check that a configuration was provided by the user if it is
      // required by the ml.
      if (mageec_context.ml->requiresDecisionConfig()) {
        if (with_ml_config) {
          mageec_context.ml->setDecisionConfig(ml_config_str.get());
        } else {
          MAGEEC_ERR("The provided machine learner needs a configuration, "
                     "but one was not provided");
          return false;
        }
      } else {
        if (with_ml_config) {
          MAGEEC_ERR("Machine learner does not require a configuration, "
                     "but one was provided");
          return false;
        }
      }
      ml_found = true;
      break;
    }
    if (!ml_found) {
      MAGEEC_ERR("Unable to optimize with the provided machine learner. "
                 "The machine learner may need to be trained before use");
      return false;
    }
    assert(mageec_context.ml);
  }

  // If feature extraction is enabled, open the output file
  if (with_feature_extract) {
    mageec_context.out_file.reset(new std::ofstream(out_file_str.get()));
  } else {
    mageec_context.out_file = nullptr;
  }
  mageec_context.with_feature_extract = with_feature_extract;
  mageec_context.with_optimize = with_optimize;
  mageec_context.is_init = true;

  // Do some sanity checks to validate the mess I have written above.
  // FIXME

  return true;
}

/// \brief Initialize the GCC Plugin
///
/// This initializes the call backs used by GCC to interact with the plugin
///
/// \param plugin_info GCC plugin information
/// \param version GCC version information
///
/// \return 0 if MAGEEC successfully set up, 1 otherwise.
int plugin_init(struct plugin_name_args *plugin_info,
                struct plugin_gcc_version *version) {
  // Initialize the MAGEEC framework
  MAGEEC_DEBUG("Initializing the MAGEEC framework");
  assert(!mageec_context.framework);
  mageec_context.framework.reset(new mageec::Framework());

  // Load built in machine learners into the framework.
  // C5 classifier
  MAGEEC_DEBUG("Registering C5 machine learner");
  std::unique_ptr<mageec::IMachineLearner> c5_ml(new mageec::C5Driver());
  mageec_context.framework->registerMachineLearner(std::move(c5_ml));
  // FileML
  MAGEEC_DEBUG("Registering FileML machine learner");
  std::unique_ptr<mageec::IMachineLearner> file_ml(new mageec::FileML());
  mageec_context.framework->registerMachineLearner(std::move(file_ml));

  // Register our information, parse arguments
  register_callback(plugin_info->base_name, PLUGIN_INFO, NULL,
                    &mageec_plugin_info);
  mageec_gcc_plugin_name = plugin_info->base_name;

  bool res = parseArguments(plugin_info, version);
  if (!res) {
    MAGEEC_ERR("Error when parsing plugin arguments");
    return 1;
  }

  register_callback(plugin_info->base_name, PLUGIN_START_UNIT, mageecStartFile,
                    NULL);
  register_callback(plugin_info->base_name, PLUGIN_FINISH_UNIT,
                    mageecFinishFile, NULL);
  register_callback(plugin_info->base_name, PLUGIN_FINISH, mageecFinish, NULL);
  register_callback(plugin_info->base_name, PLUGIN_OVERRIDE_GATE,
                    mageecPassGate, NULL);

  MAGEEC_DEBUG("Registering feature extraction pass");
  mageecRegisterFeatureExtract();
  return 0;
}
