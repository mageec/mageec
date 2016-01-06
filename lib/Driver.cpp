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

#include <memory>
#include <set>

#include "mageec/Database.h"
#include "mageec/Framework.h"
#include "mageec/ML/C5.h"
#include "mageec/Util.h"


namespace mageec {

enum class DriverMode {
  kNone,
  kCreateDatabase,
  kTrainDatabase,
};

} // end of namespace mageec


using namespace mageec;


/// \brief Convert from a metric to a string
static std::string metricToString(Metric metric) {
  switch (metric) {
  case Metric::kCodeSize:   return "size";
  case Metric::kTime:       return "time";
  case Metric::kEnergy:     return "energy";
  }
}

/// \brief Convert from a string to a metric if possible.
/// 
/// \return The metric if possible, or an empty Option type if not.
static util::Option<Metric> stringToMetric(std::string metric) {
  if (metric == "size") {
    return Metric::kCodeSize;
  }
  else if (metric == "time") {
    return Metric::kTime;
  }
  else if (metric == "energy") {
    return Metric::kEnergy;
  }
  else{
    return util::Option<Metric>();
  }
}


/// \brief Print out the version of the MAGEEC framework
static void printVersion(const Framework &framework)
{
  util::out() << static_cast<std::string>(framework.getVersion()) << '\n';
}

/// \brief Print out the version of the database
///
/// \return 0 on success, 1 if the database could not be opened.
static int printDatabaseVersion(Framework &framework, const std::string &db_path)
{
  std::unique_ptr<Database> db = framework.getDatabase(db_path, false);
  if (!db) {
    MAGEEC_ERR("Error retrieving database. The database may not exist, "
               "or you may not have sufficient permissions to read it");
    return -1;
  }
  util::out() << static_cast<std::string>(db->getVersion()) << '\n';
  return 0;
}

/// \brief Print out a help string for the mageec tool
static void printHelp()
{
  util::out() <<
"Usage: mageec [options]\n"
"       mageec (create | train) [options]\n"
"\n"
"Utility methods used alongside the MAGEEC frame work. Used to create a new \n"
"empty database, train using an existing database, or access other framework \n"
"functionality.\n"
"\n"
"Options:\n"
"  --help                   Print this help information\n"
"  --version                Print the version of the MAGEEC framework.\n"
"  --database <path>        Name of the database to be created, trained or\n"
"                           queried.\n"
"  --database-version       Print the version of the provided database.\n"
"  --machine-learner <arg>  UUID or shared object identifing the machine\n"
"                           learner to be used for training. A wildcard '*'\n"
"                           argument corresponds to all available machine\n"
"                           learners\n"
"  --print-trained-machine-learners    Print information about the machine\n"
"                           learners which are trained in the provided\n"
"                           database\n"
"  --print-machine-learner-interfaces  Print the interfaces registers with\n"
"                           the framework, and therefore usable for training\n"
"                           and decision making.\n"
"Examples:\n"
"  mageec --help\n"
"  mageec create --database foo.db\n"
"  mageec train --database bar.db --machine-learner my_ml.so\n"
"  mageec train --database foo.db\n"
"               --machine-learner ccf7593c-78d9-429a-b544-2a7339d4325e\n";
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
getMachineLearners(Framework &framework, const std::set<std::string> &ml_strs)
{
  // Load machine learners
  std::set<util::UUID> mls;
  for (const auto &str : ml_strs) {
    util::Option<util::UUID> ml_uuid;

    // A wildcard means all known machine learners should be trained
    if (str == "*") {
      for (const auto *ml : framework.getMachineLearners()) {
        mls.emplace(ml->getUUID());
      }
      continue;
    }

    // Try and parse the argument as a UUID
    ml_uuid = util::UUID::parse(str);
    if (ml_uuid && !framework.hasMachineLearner(ml_uuid.get())) {
      MAGEEC_WARN("UUID '" << str << "' is not a register machine learner "
                  "and will be ignored");
      continue;
    }

    // Not a UUID, try and load as a shared object
    if (!ml_uuid) {
      ml_uuid = framework.loadMachineLearner(str);
    }
    if (!ml_uuid) {
      MAGEEC_WARN("Unable to load machine learner '" << str << "'. This "
                  "machine learner will be ignored");
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
  return mls;
}


/// \brief Print a description of all of the machine learners trained
/// for this database.
///
/// \return 0 on success, 1 if the database could not be opened.
static int printTrainedMLs(Framework &framework, const std::string &db_path)
{
  std::unique_ptr<Database> db = framework.getDatabase(db_path, false);
  if (!db) {
    MAGEEC_ERR("Error retrieving database. The database may not exist, "
               "or you may not have sufficient permissions to read it");
    return -1;
  }

  std::vector<TrainedML> trained_mls = db->getTrainedMachineLearners();
  for (auto &ml : trained_mls) {
    util::out() << ml.getName() << '\n'
                << static_cast<std::string>(ml.getUUID()) << '\n'
                << metricToString(ml.getMetric()) << "\n\n";
  }
  return 0;
}

/// \brief Print a description of all of the machine learner interfaces
/// known by the framework.
static void printMLInterfaces(Framework &framework)
{
  std::set<IMachineLearner *> mls = framework.getMachineLearners();
  for (const auto *ml : mls) {
    util::out() << ml->getName() << '\n'
                << static_cast<std::string>(ml->getUUID()) << "\n\n";
  }
}

/// \brief Create a new database
///
/// \return 0 on success, 1 if the database could not be created.
static int createDatabase(Framework &framework, const std::string &db_path)
{
  std::unique_ptr<Database> db = framework.getDatabase(db_path, true);
  if (!db) {
    MAGEEC_ERR("Error creating new database. The database may already exist, "
               "or you may not have sufficient permissions to create the "
               "file");
    return -1;
  }
  return 0;
}

/// \brief Train a database
///
/// \return 0 on success, 1 if the database could not be trained.
static int trainDatabase(Framework &framework,
                         const std::string &db_path,
                         const std::set<util::UUID> mls,
                         const std::set<std::string> &metric_strs)
{
  assert(metric_strs.size() > 0);

  // Parse the metrics we are training against.
  std::set<Metric> metrics;
  for (auto str : metric_strs) {
    util::Option<Metric> metric = stringToMetric(str);
    if (!metric) {
      MAGEEC_WARN("Unrecognized metric specified '" << str << "'");
    }
    else {
      metrics.insert(metric.get());
    }
  }
  if (metrics.size() == 0) {
    MAGEEC_ERR("No recognized metrics specified");
    return -1;
  }

  // The database to be trained.
  std::unique_ptr<Database> db = framework.getDatabase(db_path, false);
  if (!db) {
    MAGEEC_ERR("Error retrieving database. The database may not exist, "
               "or you may not have sufficient permissions to read it");
    return -1;
  }

  // Train. This will put the training blobs from the machine learners into
  // the database.
  for (auto metric : metrics) {
    for (auto ml : mls) {
      db->trainMachineLearner(ml, metric);
    }
  }
  return 0;
}


/// \brief Entry point for the MAGEEC driver
int main(int argc, const char *argv[])
{
  DriverMode mode = DriverMode::kNone;

  // The database to be created or trained
  util::Option<std::string> db_str;
  // The machine learners to train
  std::set<std::string> ml_strs;
  // Metrics to train the machine learners against
  std::set<std::string> metrics;

  bool seen_database = false;
  bool seen_mls = false;
  bool seen_metrics = false;

  bool with_help = false;
  bool with_version = false;
  bool with_database_version = false;
  bool with_print_trained_mls = false;
  bool with_print_ml_interfaces = false;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    // If this is the first argument, it may specify the mode which the tool
    // is running in.
    if (i == 1) {
      if (arg == "create") {
        mode = DriverMode::kCreateDatabase;
        continue;
      }
      if (arg == "train") {
        mode = DriverMode::kTrainDatabase;
        continue;
      }
    }

    // Common flags
    if (arg == "--help") {
      with_help = true;
    }
    else if (arg == "--version") {
      with_version = true;
    }
    else if (arg == "--print-machine-learner-interfaces") {
      with_print_ml_interfaces = true;
    }
    else if (arg == "--print-trained-machine-learners") {
      with_print_trained_mls = true;
    }
    else if (arg == "--database-version") {
      with_database_version = true;
    }

    else if (arg == "--metric") {
      ++i;
      if (i >= argc) {
        MAGEEC_ERR("No '--metric' value provided");
        return -1;
      }
      metrics.insert(std::string(argv[i]));
      seen_metrics = true;
    }
    else if (arg == "--database") {
      if (seen_database) {
        MAGEEC_ERR("Argument '--database' already seen");
        return -1;
      }
      ++i;
      if (i >= argc) {
        MAGEEC_ERR("No '--database' value provided");
      }
      db_str = std::string(argv[i]);
      seen_database = true;
    }
    else if (arg == "--machine-learner") {
      ++i;
      if (i >= argc) {
        MAGEEC_ERR("No '--machine-learner' value provided");
        return -1;
      }
      ml_strs.insert(std::string(argv[i]));
      seen_mls = true;
    }
    else {
      MAGEEC_ERR("Unrecognized argument: " << arg);
      return -1;
    }
  }

  // Errors
  if (mode == DriverMode::kTrainDatabase && !seen_mls) {
    MAGEEC_ERR("Training mode specified without machine learners");
    return -1;
  }
  if (mode == DriverMode::kTrainDatabase && !seen_metrics) {
    MAGEEC_ERR("Training mode specified without any metrics to train for");
    return -1;
  }
  if (mode == DriverMode::kCreateDatabase && !seen_database) {
    MAGEEC_ERR("Create mode specified without any database to create");
    return -1;
  }

  // Warnings
  if (mode == DriverMode::kCreateDatabase && seen_mls) {
    MAGEEC_WARN("Creation mode specified, '--machine-learner arguments "
                "will be ignored");
  }
  if (with_database_version && !seen_database) {
    MAGEEC_WARN("Cannot get database version, as no database was specified");
  }
  if (with_print_trained_mls && !seen_database) {
    MAGEEC_WARN("Cannot print trained machine learners as no database was "
                "specified");
  }

  // Initialize the framework, and register some builtin machine learner
  // interfaces so that they can be selected by UUID by the user.
  Framework framework;
  // C5 classifier
  std::unique_ptr<IMachineLearner> c5_ml(new C5Driver());
  framework.registerMachineLearner(std::move(c5_ml));

  // Get the UUIDs of the available machine learners provided on the command
  // line.
  std::set<util::UUID> mls;
  if (seen_mls) {
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
  if (with_database_version) {
    if (!printDatabaseVersion(framework, db_str.get())) {
      return -1;
    }
  }
  if (with_print_trained_mls) {
    if (!printTrainedMLs(framework, db_str.get())) {
      return -1;
    }
  }
  if (with_print_ml_interfaces) {
    printMLInterfaces(framework);
  }

  // Handle modes
  switch (mode) {
    case DriverMode::kCreateDatabase:
      return createDatabase(framework, db_str.get());
    case DriverMode::kTrainDatabase:
      return trainDatabase(framework, db_str.get(), mls, metrics);
    default:
      break;
  }
  return 0;
}
