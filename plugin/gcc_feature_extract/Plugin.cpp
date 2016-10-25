/*  MAGEEC GCC Feature Extraction Plugin
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

//===-------------- GCC Feature Extraction Plugin Interface ---------------===//
//
// Interface between GCC and the MAGEEC framework.
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
#include "mageec/Util.h"
#include "Plugin.h"

#ifndef BUILDING_GCC_VERSION
  #include "bversion.h"
  #define GCC_VERSION BUILDING_GCC_VERSION
#endif
#if (BUILDING_GCC_VERSION < 4009)
  #include "gcc-plugin.h"
  #include "tree-pass.h"
  #include "gimple.h"
  #include "function.h"
  #include "toplev.h"
#else
  #include "gcc-plugin.h"
  #include "config.h"
  #include "system.h"
  #include "coretypes.h"
  #include "tm.h"
  #include "tree.h"
  #include "stringpool.h"
  #include "toplev.h"
  #include "basic-block.h"
#if (BUILDING_GCC_VERSION < 5001)
  #include "pointer-set.h"
#endif // (BUILDING_GCC_VERSION < 5001)
  #include "hash-table.h"
  #include "vec.h"
  #include "ggc.h"
  #include "basic-block.h"
  #include "tree-ssa-alias.h"
  #include "internal-fn.h"
  #include "gimple-fold.h"
  #include "tree-eh.h"
  #include "gimple-expr.h"
  #include "is-a.h"
  #include "gimple.h"
  #include "gimple-iterator.h"
  #include "tree.h"
  #include "tree-pass.h"
  #include "intl.h"
  #include "diagnostic.h"
  #include "context.h"
#endif

#include <fstream>
#include <map>
#include <string>


/// Declared to allow GCC to load the plugin
int plugin_is_GPL_compatible;

/// Stored GCC Plugin name for future register_callbacks.
const char *feature_extract_plugin_name;

/// Init version of the plugin
const mageec::util::Version
    feature_extract_plugin_version(GCC_FEATURE_EXTRACT_PLUGIN_VERSION_MAJOR,
                                   GCC_FEATURE_EXTRACT_PLUGIN_VERSION_MINOR,
                                   GCC_FEATURE_EXTRACT_PLUGIN_VERSION_PATCH);

/// Helper to get hold of the singleton context used by the plugin
static FeatureExtractContext &getContext(void) {
  return FeatureExtractContext::getInstance();
}

/// Plugin information to pass to GCC
#define QUOTE(name) #name
#define STRINGIFY(macro) QUOTE(macro)
struct FeatureExtractPluginInfo {
  FeatureExtractPluginInfo() {}

  operator struct plugin_info() {
    struct plugin_info info;
    info.version = STRINGIFY(GCC_FEATURE_EXTRACT_PLUGIN_VERSION_MAJOR) "."
                   STRINGIFY(GCC_FEATURE_EXTRACT_PLUGIN_VERSION_MINOR) "."
                   STRINGIFY(GCC_FEATURE_EXTRACT_PLUGIN_VERSION_PATCH);
    info.help = "For help, pass -help as an argument to the plugin";
    return info;
  }
};
#undef QUOTE
#undef STRINGIFY

static struct plugin_info feature_extract_plugin_info =
    FeatureExtractPluginInfo();


/// \brief Print help information to stdout
static void printHelp()
{
  mageec::util::out() <<
"MAGEEC Feature Extraction plugin for GCC. This allows GCC to extract\n"
"features to be used by the MAGEEC framework.\n"
"\n"
"options:\n"
"  -help                Print this help information\n"
"  -plugin-info         Print information about the plugin\n"
"  -version             Print the version of the GCC plugin\n"
"  -framework-version   Print the version of the MAGEEC framework\n"
"  -debug               Enable debug output from the plugin and framework\n"
"  -database=<arg>      Database to be used to store extracted features\n"
"  -database-version    Print the version of the provided database\n"
"  -out=<arg>           The output file records identifiers of feature sets\n"
"                       in the database for each element of the program\n"
"\n"
"examples:\n"
"  gcc -fplugin=libfeature_extract_gcc.so\n"
"      -fplugin-libfeature_extract_gcc-help foo.c\n"
"\n"
"  gcc -fplugin=libfeature_extract_gcc.so\n"
"      -fplugin-libfeature_extract_gcc-database=foo.db\n";
}


/// \brief Print information about the plugin to stdout
///
/// \param plugin_info  GCC plugin information
/// \param version      GCC version information
static void printPluginInfo(struct plugin_name_args *plugin_info,
                            struct plugin_gcc_version *version)
{
  mageec::util::out() <<
MAGEEC_PREFIX "Feature Extraction plugin information\n"
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
      << static_cast<std::string>(feature_extract_plugin_version) << '\n';
}

/// \brief Print the framework version
static void printFrameworkVersion(const mageec::Framework &framework)
{
  mageec::util::out() << MAGEEC_PREFIX "Framework version: "
      << static_cast<std::string>(framework.getVersion()) << '\n';
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
  int argc = plugin_info->argc;
  struct plugin_argument *argv = plugin_info->argv;

  std::string db_str;
  std::string outfile_str;

  // Simple flags
  bool with_help                = false;
  bool with_plugin_info         = false;
  bool with_version             = false;
  bool with_framework_version   = false;
  bool with_debug               = false;
  bool with_db_version          = false;

  // Flags with arguments
  bool with_db       = false;
  bool with_outfile  = false;

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
    } else if (arg_str == "out") {
      if (with_outfile) {
        MAGEEC_ERR("Plugin argument 'out' already seen");
        return false;
      }
      if (!argv[i].value) {
        MAGEEC_ERR("No value provided to 'out' argument");
        return false;
      }
      outfile_str = std::string(argv[i].value);
      with_outfile = true;
    } else {
      MAGEEC_WARN("Unrecognized argument '" << arg_str << "' ignored");
    }
  }

  // Enable debug now that parsing arguments is finished.
  getContext().getFramework().setDebug(with_debug);


  // Print plugin information
  if (with_plugin_info)
    printPluginInfo(plugin_info, version);

  // Print help information
  if (with_help)
    printHelp();

  // Print plugin version
  if (with_version)
    printPluginVersion();

  // Print framework version
  if (with_framework_version)
    printFrameworkVersion(getContext().getFramework());

  // Errors
  if (!with_db) {
    MAGEEC_ERR("Cannot feature extract without a database to save features "
               "to");
    return false;
  }
  if (!with_outfile) {
    MAGEEC_ERR("Cannot feature extract without somewhere to output feature set "
               "ids");
    return false;
  }

  // Now we know whether a database is required we can load it.
  assert(db_str != "");
  getContext().loadDatabase(db_str);

  // Print the database version now that it is loaded
  if (with_db_version)
    printDatabaseVersion(getContext().getDatabase());

  getContext().openOutFile(outfile_str);
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
  feature_extract_plugin_name = plugin_info->base_name;

  MAGEEC_DEBUG("Initializing the MAGEEC framework");
  getContext().setFramework(
      std::unique_ptr<mageec::Framework>(new mageec::Framework()));

  // Parse command line arguments
  bool res = parseArguments(plugin_info, version);
  if (!res) {
    MAGEEC_ERR("Error when parsing plugin arguments");
    return 1;
  }

  // Register callbacks
  register_callback(feature_extract_plugin_name, PLUGIN_INFO, NULL,
                    &feature_extract_plugin_info);

  register_callback(feature_extract_plugin_name, PLUGIN_FINISH_UNIT,
                    featureExtractFinishUnit, NULL);

  // Register the feature extraction pass
  registerFeatureExtractPass();
  return 0;
}


//===------------------ Feature Extractor pass definition -----------------===//


#if (GCC_VERSION < 4009)
static struct gimple_opt_pass feature_extract_pass = {{
    GIMPLE_PASS,
    "feature-extractor",  // name
    OPTGROUP_NONE,        // optinfo_flags
    NULL,                 // gate
    featureExtract,       // execute
    NULL,                 // sub
    NULL,                 // next
    0,                    // static_pass_number
    TV_NONE,              // tv_id
    PROP_ssa,             // properties_required
    0,                    // properties_provided
    0,                    // properties_destroyed
    0,                    // todo_flags_start
    0                     // todo_flags_finish
}};
#elif(GCC_VERSION >= 4009)
namespace {
const pass_data pass_data_feature_extract = {
    GIMPLE_PASS,          // type
    "feature-extractor",  // name
    OPTGROUP_NONE,        // optinfo_flags
#if (GCC_VERSION < 5001)
    false,                // has_gate
    true,                 // has_execute
#endif // (GCC_VERSION < 5001)
    TV_NONE,              // tv_id
    PROP_ssa,             // properties_required
    0,                    // properties_provided
    0,                    // properties_destroyed
    0,                    // todo_flags_start
    0,                    // todo_flags_finish
};

class FeatureExtractPass : public gimple_opt_pass {
public:
  FeatureExtractPass(gcc::context *context)
      : gimple_opt_pass(pass_data_feature_extract, context)
  {}

// opt_pass methods
#if (GCC_VERSION < 5001)
  bool gate() override { return true; }
  unsigned execute() override { return featureExtractExecute(); }
#else
  bool gate(function *) override { return true; }
  unsigned execute(function *) override { return featureExtractExecute(); }
#endif
};
} // end of anonymous namespace

/// \brief Create a new feature extractor pass
static gimple_opt_pass *createFeatureExtractPass(gcc::context *context) {
  return new FeatureExtractPass(context);
}
#endif // (GCC_VERSION < 4009)


/// \brief Register the feature extractor pass in the pass list
///
/// Currently this runs after the CFG pass
void registerFeatureExtractPass(void) {
  struct register_pass_info pass;

#if (GCC_VERSION < 4009)
  pass.pass = &feature_extract_pass.pass;
#else
  pass.pass = createFeatureExtractPass(g);
#endif
  pass.reference_pass_name = "ssa";
  pass.ref_pass_instance_number = 1;
  pass.pos_op = PASS_POS_INSERT_AFTER;

  register_callback(feature_extract_plugin_name, PLUGIN_PASS_MANAGER_SETUP,
                    NULL, &pass);
}

unsigned featureExtractExecute() {
  std::string func_name = current_function_name();

  std::unique_ptr<FunctionFeatures> features = extractFunctionFeatures();  

  auto &func_features = getContext().getFunctionFeatures();
  assert(func_features.count(func_name) == 0);
  func_features[func_name] = std::move(features);

  return 0;
}

void featureExtractStartUnit(void *, void *) {
  getContext().getFunctionFeatures().clear();
}

void featureExtractFinishUnit(void *, void *) {
  std::vector<const FunctionFeatures *> func_features;
  for (auto &features : getContext().getFunctionFeatures())
    func_features.push_back(features.second.get());

  // Extract module features based on the function features, convert to a
  // mageec feature set
  std::string module_name = main_input_filename;

  std::unique_ptr<ModuleFeatures> module_features =
      extractModuleFeatures(func_features);
  std::unique_ptr<mageec::FeatureSet> module_feature_set =
      convertModuleFeatures(*module_features);

  mageec::FeatureSetID module_feature_set_id =
      getContext().getDatabase().newFeatureSet(*module_feature_set);

  getContext().getOutFile() << main_input_filename << ",module,"
                            << module_name << ",features,"
                            << (uint64_t)module_feature_set_id
                            << ",feature_class,"
                            << (uint64_t)mageec::FeatureClass::kModule << "\n";

  // Insert the features of each function into the database
  // Functions also inherit features from their encapsulating module
  for (auto &features : getContext().getFunctionFeatures()) {
    std::unique_ptr<mageec::FeatureSet> func_feature_set =
        convertFunctionFeatures(*features.second.get());

    mageec::FeatureSetID func_feature_set_id =
        getContext().getDatabase().newFeatureSet(*func_feature_set);

    getContext().getOutFile() << main_input_filename << ",function,"
                              << features.first << ",features,"
                              << (uint64_t)func_feature_set_id
                              << ",feature_class,"
                              << (uint64_t)mageec::FeatureClass::kFunction
                              << "\n";
  }
}
