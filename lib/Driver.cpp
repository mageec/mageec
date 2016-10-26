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

//===---------------------------- MAGEEC driver ---------------------------===//
//
// This implements the standalone driver for the MAGEEC framework. This
// provides the ability to train the database to the user, as well as a
// number of other standalone utilites needed by MAGEEC
//
//===----------------------------------------------------------------------===//

#include "mageec/Database.h"
#include "mageec/Framework.h"
#include "mageec/ML/C5.h"
#include "mageec/Util.h"

#include <fstream>
#include <memory>
#include <set>
#include <sstream>

namespace mageec {

enum class DriverMode { kNone, kCreate, kTrain, kAddResults, kGarbageCollect };

} // end of namespace mageec

using namespace mageec;

/// \brief Print out the version of the MAGEEC framework
static void printVersion(const Framework &framework) {
  util::out() << MAGEEC_PREFIX "Framework version: "
              << static_cast<std::string>(framework.getVersion()) << '\n';
}

/// \brief Print out the version of the database
///
/// \return 0 on success, 1 if the database could not be opened.
static int printDatabaseVersion(Framework &framework,
                                const std::string &db_path) {
  std::unique_ptr<Database> db = framework.getDatabase(db_path, false);
  if (!db) {
    MAGEEC_ERR("Error retrieving database. The database may not exist, "
               "or you may not have sufficient permissions to read it");
    return -1;
  }
  util::out() << MAGEEC_PREFIX "Database version: "
              << static_cast<std::string>(db->getVersion()) << '\n';
  return 0;
}

/// \brief Print out a help string for the mageec tool
static void printHelp()
{
  util::out() <<
"Usage: mageec [options]\n"
"       mageec foo.db <mode> [options]\n"
"\n"
"Utility methods used alongside the MAGEEC framework. Used to create a new\n"
"database, train an existing database, add results, or access other\n"
"framework functionality.\n"
"\n"
"mode:\n"
"  --create                Create a new empty database.\n"
"  --train                 Train an existing database, using machine\n"
"                          learners provided via the --ml flag\n"
"  --garbage-collect       Delete anything from the database which is not\n"
"                          associated with a result\n"
"  --add-results <arg>     Add results from the provided file into the\n"
"                          database\n"
"\n"
"options:\n"
"  --help                  Print this help information\n"
"  --version               Print the version of the MAGEEC framework\n"
"  --debug                 Enable debug output in the framework\n"
"  --database-version      Print the version of the provided database\n"
"  --ml <arg>              UUID or shared object identifying a machine\n"
"                          learner interface to be used\n"
"  --print-ml-interfaces   Print the interfaces registered with the MAGEEC\n"
"                          framework, and therefore usable for training and\n"
"                          decision making\n"
"  --print-mls             Print information about the machine learners\n"
"                          available to make compiler configuration\n"
"                          decisions\n"
// FIXME: ml-config
"  --metric <arg>          Adds a new metric which the provided machine\n"
"                          learners should be trained with\n"
"\n"
"examples:\n"
"  mageec --help --version\n"
"  mageec foo.db --create\n"
"  mageec bar.db --train --ml path/to/ml_plugin.so\n"
"  mageec baz.db --train --ml deadbeef-ca75-4096-a935-15cabba9e5\n";
}

/// \brief Retrieve or load machine learners provided on the command line
///
/// \param framework  The framework to register the machine learners with
/// \param ml_strs  A list of strings from the command line to be parsed
/// and loaded into the framework.
///
/// \return A list of UUIDs of loaded machine learners, which may be empty
/// if none were successfully loaded.
static std::set<util::UUID>
getMachineLearners(Framework &framework, const std::set<std::string> &ml_strs) {
  // Load machine learners
  std::set<util::UUID> mls;
  for (const auto &str : ml_strs) {
    MAGEEC_DEBUG("Retrieving machine learner '" << str << "'");
    util::Option<util::UUID> ml_uuid;

    // Try and parse the argument as a UUID
    ml_uuid = util::UUID::parse(str);
    if (ml_uuid && !framework.hasMachineLearner(ml_uuid.get())) {
      MAGEEC_WARN("UUID '" << str << "' is not a register machine learner "
                                     "and will be ignored");
      continue;
    }
    if (ml_uuid) {
      MAGEEC_DEBUG("Found machine learner with UUID '" << str << "'");
    }

    // Not a UUID, try and load as a shared object
    if (!ml_uuid) {
      ml_uuid = framework.loadMachineLearner(str);
      if (ml_uuid) {
        MAGEEC_DEBUG("Loading machine learner from library '" << str << "'");
      }
    }
    if (!ml_uuid) {
      MAGEEC_WARN("Unable to load machine learner '"
                  << str << "'. This machine learner will be ignored");
      continue;
    }
    assert(ml_uuid);

    // add uuid of ml to be trained.
    mls.insert(ml_uuid.get());
  }
  if (mls.empty()) {
    MAGEEC_ERR("No machine learners were successfully loaded");
    return std::set<util::UUID>();
  }
  MAGEEC_DEBUG("Retrieved " << mls.size() << " machine learners");
  return mls;
}

/// \brief Print a description of all of the machine learners trained
/// for this database.
///
/// If a database is not provided, then print the machine learners which
/// do not require training.
///
/// \return true on success
static bool printTrainedMLs(Framework &framework,
                            const util::Option<std::string> db_path) {
  // Add any machine learners which do not require training (and therefore
  // do not have any entry in the database)
  std::vector<TrainedML> trained_mls;
  for (const auto ml : framework.getMachineLearners()) {
    if (!ml->requiresTraining()) {
      trained_mls.push_back(TrainedML(*ml));
    }
  }

  if (db_path) {
    std::unique_ptr<Database> db = framework.getDatabase(db_path.get(), false);
    if (!db) {
      MAGEEC_ERR("Error retrieving database. The database may not exist, "
                 "or you may not have sufficient permissions to read it");
      return false;
    }
    // Add trained machine learners from the database
    for (const auto &ml : db->getTrainedMachineLearners()) {
      trained_mls.push_back(ml);
    }
  }

  // Print out the trained machine learners
  for (auto &ml : trained_mls) {
    util::out() << ml.getName() << '\n'
                << static_cast<std::string>(ml.getUUID()) << '\n'
                << ml.getMetric() << "\n\n";
  }
  return true;
}

/// \brief Print a description of all of the machine learner interfaces
/// known by the framework.
static void printMLInterfaces(Framework &framework) {
  std::set<IMachineLearner *> mls = framework.getMachineLearners();
  for (const auto *ml : mls) {
    util::out() << ml->getName() << '\n'
                << static_cast<std::string>(ml->getUUID()) << "\n\n";
  }
}

/// \brief Create a new database
///
/// \return true on success, false if the database could not be created.
static bool createDatabase(Framework &framework, const std::string &db_path) {
  std::unique_ptr<Database> db = framework.getDatabase(db_path, true);
  if (!db) {
    MAGEEC_ERR("Error creating new database. The database may already exist, "
               "or you may not have sufficient permissions to create the "
               "file");
    return false;
  }
  return true;
}

/// \brief Train a database
///
/// \return true on success, false if the database could not be trained.
static bool trainDatabase(Framework &framework, const std::string &db_path,
                          const std::set<util::UUID> mls,
                          const std::set<std::string> &metric_strs) {
  assert(metric_strs.size() > 0);

  // Parse the metrics we are training against.
  MAGEEC_DEBUG("Parsing training metrics");
  std::set<std::string> metrics;
  for (auto metric : metric_strs) {
    metrics.insert(metric);
  }
  if (metrics.size() == 0) {
    MAGEEC_ERR("No metrics specified");
    return false;
  }

  // The database to be trained.
  MAGEEC_DEBUG("Retrieving database '" << db_path << "' for training");
  std::unique_ptr<Database> db = framework.getDatabase(db_path, false);
  if (!db) {
    MAGEEC_ERR("Error retrieving database. The database may not exist, "
               "or you may not have sufficient permissions to read it");
    return false;
  }

  // Train against every combination of provided machine learner, metric, and
  // class of features. This will insert training blobs generated by the
  // machine learner into the database.
  for (auto ml : mls) {
    for (auto metric : metrics) {
      MAGEEC_DEBUG("Training for metric: " << metric);
      for (auto feature_class = FeatureClass::kFIRST_FEATURE_CLASS;
           feature_class <= FeatureClass::kLAST_FEATURE_CLASS; /*empty*/) {
        db->trainMachineLearner(ml, feature_class, metric);
        feature_class =
            static_cast<FeatureClass>(static_cast<TypeID>(feature_class) + 1);
      }
    }
  }
  return true;
}

static util::Option<std::set<Database::InputResult>>
parseResults(const std::string &result_path) {
  std::set<Database::InputResult> results;

  MAGEEC_DEBUG("Opening file '" << result_path << "' to parse results");
  std::ifstream result_file(result_path);
  if (!result_file) {
    MAGEEC_ERR("Could not open results file '"
               << result_path
               << "', the "
                  "file may not exist, or you may not have permissions to "
                  "read it");
    return nullptr;
  }

  std::string line;
  while (std::getline(result_file, line)) {
    std::string id_str;
    std::string metric_str;
    std::string value_str;

    // read the id
    auto line_it = line.begin();
    for (; line_it != line.end() && *line_it != ','; line_it++) {
      id_str.push_back(*line_it);
    }
    if (line_it == line.end() || id_str.size() == 0) {
      MAGEEC_ERR("Malformed results file line\n" << line);
      continue;
    }
    assert(*line_it == ',');
    line_it++;

    // metric string
    for (; line_it != line.end() && *line_it != ','; line_it++) {
      metric_str.push_back(*line_it);
    }
    if (line_it == line.end() || metric_str.size() == 0) {
      MAGEEC_WARN("Malformed results file line\n" << line);
      continue;
    }
    assert(*line_it == ',');
    line_it++;

    // value string
    for (; line_it != line.end() && *line_it != ','; line_it++) {
      value_str.push_back(*line_it);
    }
    if (line_it != line.end() || value_str.size() == 0) {
      // junk on the end of the line
      MAGEEC_WARN("Malformed results file line\n" << line);
      continue;
    }

    // Convert each field to its expected type.
    uint64_t tmp;

    std::istringstream id_stream(id_str);
    id_stream >> tmp;
    if (id_stream.fail()) {
      MAGEEC_ERR("Malformed compilation id in results file line:\n" << line);
      return nullptr;
    }
    CompilationID compilation_id = static_cast<CompilationID>(tmp);

    uint64_t value;
    std::istringstream value_stream(value_str);
    value_stream >> value;
    if (id_stream.fail()) {
      MAGEEC_ERR("Malformed result value '" << value_str << "' in result "
                                                            "file line:\n"
                                            << line);
      return nullptr;
    }

    // Add the now parsed results into the dataset
    results.insert({compilation_id, metric_str, value});
  }
  return results;
}

static bool addResults(Framework &framework, const std::string &db_path,
                       const std::string &results_path) {
  std::unique_ptr<Database> db = framework.getDatabase(db_path, false);
  if (!db) {
    MAGEEC_ERR("Error retrieving database. The database may not exist, "
               "or you may not have sufficient permissions to read it");
    return false;
  }

  // Parse the results file, then add them to the database.
  auto results = parseResults(results_path);
  if (!results) {
    MAGEEC_ERR("Error parsing results file");
    return false;
  }
  if (results.get().size() == 0) {
    MAGEEC_WARN("No results found in the provided file, nothing will be "
                "added to the database");
    return true;
  }
  MAGEEC_DEBUG("Adding parsed results to the database");
  db->addResults(results.get());
  return true;
}

static bool garbageCollect(Framework &framework, const std::string &db_path) {
  std::unique_ptr<Database> db = framework.getDatabase(db_path, false);
  if (!db) {
    MAGEEC_ERR("Error retrieving database. The database may not exist, "
               "or you may not have sufficient permissions to read it");
    return false;
  }
  MAGEEC_DEBUG("Garbage collecting unreachable values from the database");
  db->garbageCollect();
  return true;
}

/// \brief Entry point for the MAGEEC driver
int main(int argc, const char *argv[]) {
  DriverMode mode = DriverMode::kNone;

  // The database to be created or trained
  util::Option<std::string> db_str;
  // Metrics to train the machine learners
  std::set<std::string> metric_strs;
  // Machine learners to train
  std::set<std::string> ml_strs;
  // The path to the results to be inserted into the database
  util::Option<std::string> results_path;

  bool with_db      = false;
  bool with_metric  = false;
  bool with_ml      = false;

  bool with_db_version          = false;
  bool with_debug               = false;
  bool with_help                = false;
  bool with_print_ml_interfaces = false;
  bool with_print_mls           = false;
  bool with_version             = false;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    // If this is the first argument, it may specify the database which the
    // tool is to use.
    if (i == 1) {
      if (arg[0] != '-') {
        db_str = arg;
        with_db = true;
        continue;
      }
    }

    // If a database is specified, then the second argument *might* be the
    // mode
    if ((i == 2) && with_db) {
      if (arg == "--create") {
        mode = DriverMode::kCreate;
        continue;
      } else if (arg == "--train") {
        mode = DriverMode::kTrain;
        continue;
      } else if (arg == "--garbage-collect") {
        mode = DriverMode::kGarbageCollect;
        continue;
      }
    }

    // Common flags
    if (arg == "--help") {
      with_help = true;
    } else if (arg == "--version") {
      with_version = true;
    } else if (arg == "--debug") {
      with_debug = true;
    } else if (arg == "--print-ml-interfaces") {
      with_print_ml_interfaces = true;
    } else if (arg == "--print-mls") {
      with_print_mls = true;
    } else if (arg == "--database-version") {
      with_db_version = true;
    }

    else if (arg == "--metric") {
      ++i;
      if (i >= argc) {
        MAGEEC_ERR("No '--metric' value provided");
        return -1;
      }
      metric_strs.insert(std::string(argv[i]));
      with_metric = true;
    } else if (arg == "--ml") {
      ++i;
      if (i >= argc) {
        MAGEEC_ERR("No '--ml' value provided");
        return -1;
      }
      ml_strs.insert(std::string(argv[i]));
      with_ml = true;
    } else if (arg == "--add-results") {
      ++i;
      if (i >= argc) {
        MAGEEC_ERR("No '--add-results' value provided");
        return -1;
      }
      results_path = std::string(argv[i]);
      mode = DriverMode::kAddResults;
    } else {
      MAGEEC_ERR("Unrecognized argument: '" << arg << "'");
      return -1;
    }
  }

  // Errors
  if (mode == DriverMode::kTrain && !with_ml) {
    MAGEEC_ERR("Training mode specified without machine learners");
    return -1;
  }
  if (mode == DriverMode::kTrain && !with_metric) {
    MAGEEC_ERR("Training mode specified without any metric to train for");
    return -1;
  }

  // Warnings
  if (with_db_version && !with_db) {
    MAGEEC_WARN("Cannot get database version as no database was specified");
  }

  // Unused arguments
  if ((mode == DriverMode::kNone) ||
      (mode == DriverMode::kCreate) ||
      (mode == DriverMode::kAddResults) ||
      (mode == DriverMode::kGarbageCollect)) {
    if (with_metric) {
      MAGEEC_WARN("--metric arguments will be ignored for the specified mode");
    }
    if (with_ml) {
      MAGEEC_WARN("--ml arguments will be ignored for the specified mode");
    }
  }

  // Initialize the framework, and register some built in machine learners
  // so that they can be selected by UUID by the user.
  Framework framework(with_debug);

  // C5 classifier
  MAGEEC_DEBUG("Registering C5.0 machine learner interface");
  std::unique_ptr<IMachineLearner> c5_ml(new C5Driver());
  framework.registerMachineLearner(std::move(c5_ml));

  // Get the machine learners provided on the command line
  std::set<util::UUID> mls;
  if (with_ml) {
    assert(ml_strs.size() != 0);
    mls = getMachineLearners(framework, ml_strs);
    if (mls.size() <= 0) {
      return -1;
    }
  }

  // Handle common arguments
  if (with_version) {
    printVersion(framework);
  }
  if (with_help) {
    printHelp();
  }
  if (with_db_version) {
    if (!printDatabaseVersion(framework, db_str.get())) {
      return -1;
    }
  }
  if (with_print_mls) {
    if (!printTrainedMLs(framework, db_str)) {
      return -1;
    }
  }
  if (with_print_ml_interfaces) {
    printMLInterfaces(framework);
  }

  // Handle modes
  switch (mode) {
  case DriverMode::kNone:
    return 0;
  case DriverMode::kCreate:
    if (!createDatabase(framework, db_str.get())) {
      return -1;
    }
    return 0;
  case DriverMode::kTrain:
    if (!trainDatabase(framework, db_str.get(), mls, metric_strs)) {
      return -1;
    }
    return 0;
  case DriverMode::kAddResults:
    if (!addResults(framework, db_str.get(), results_path.get())) {
      return -1;
    }
    return 0;
  case DriverMode::kGarbageCollect:
    if (!garbageCollect(framework, db_str.get())) {
      return -1;
    }
    return 0;
  }
  return 0;
}
