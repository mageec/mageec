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

#include <iostream>
#include <memory>
#include <set>

#include "mageec/Database.h"
#include "mageec/Framework.h"
#include "mageec/ML/C5.h"


// Utilies used for debugging the driver
static std::ostream &mageecDbg() { return std::cerr; }

#define MAGEEC_PREFIX "-- "
#define MAGEEC_ERR(msg) mageecDbg() << MAGEEC_PREFIX << "error: " << msg << std::endl
#define MAGEEC_WARN(msg) mageecDbg() << MAGEEC_PREFIX << "warning: " << msg << std::endl
#define MAGEEC_MSG(msg) mageecDbg() << MAGEEC_PREFIX << msg << std::endl


namespace mageec {

enum class DriverMode {
  kNone,
  kCreateDatabase,
  kTrainDatabase,
};

} // end of namespace mageec


using namespace mageec;


int createDatabase(const std::string &db_path)
{
  Framework framework;

  // Register some machine learners that are part of MAGEEC
  std::unique_ptr<IMachineLearner> c5_ml(new C5Driver());
  framework.registerMachineLearner(std::move(c5_ml));


  std::unique_ptr<Database> db = framework.getDatabase(db_path, true);
  if (!db) {
    MAGEEC_ERR("Error creating new database. The database may already exist, "
               "or you may not have sufficient permissions to create the "
               "file");
    return -1;
  }
  return 0;
}

int trainDatabase(const std::string &db_path,
                  const std::set<std::string> &ml_strs,
                  const std::set<std::string> &metric_strs)
{
  assert(metric_strs.size() > 0);
  assert(ml_strs.size() > 0);

  Framework framework;

  // Parse the metrics we are training against.
  std::set<Metric> metrics;
  for (auto str : metric_strs) {
    util::Option<Metric> metric;
    if (str == "size") {
      metric = Metric::kCodeSize;
    }
    else if (str == "time") {
      metric = Metric::kTime;
    }
    else if (str == "energy") {
      metric = Metric::kEnergy;
    }
    else {
      MAGEEC_WARN("Unrecognized metric specified '" << str << "'");
    }
    if (metric) {
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

  // Load machine learners
  std::set<mageec::util::UUID> mls;
  for (auto str : ml_strs) {
    mageec::util::Option<mageec::util::UUID> ml_uuid;

    // A wildcard means all known machine learners should be trained
    if (str == "*") {
      mls = framework.getMachineLearners();
      continue;
    }

    // Try and parse the argument as a UUID
    ml_uuid = mageec::util::UUID::parse(str);
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
  std::set<std::string> mls;
  // Metrics to train the machine learners against
  std::set<std::string> metrics;

  bool seen_database = false;
  bool seen_mls = false;
  bool seen_metrics = false;

  bool with_help = false;
  bool with_version = false;
  bool with_database_version = false;
  bool with_debug = false;
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
    else if (arg == "--debug") {
      with_debug = true;
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
      mls.insert(std::string(argv[i]));
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
  if (with_print_ml_interfaces && !seen_mls) {
    MAGEEC_WARN("Cannot print machine learner interfaces with no specified "
                "machine learners");
  }
  if (with_print_trained_mls && !seen_database) {
    MAGEEC_WARN("Cannot print trained machine learners as no database was "
                "specified");
  }

  // Handle normal arguments
  (void)with_help;
  (void)with_version;
  (void)with_debug;

  // Handle modes
  switch (mode) {
    case DriverMode::kCreateDatabase:
      return createDatabase(db_str.get());
    case DriverMode::kTrainDatabase:
      return trainDatabase(db_str.get(), mls, metrics);
    default:
      break;
  }
  return 0;
}
