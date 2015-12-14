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

#include "mageec/Database.h"
#include "mageec/Framework.h"


// Utilies used for debugging the driver
static std::ostream &mageecDbg() { return std::cerr; }

#define MAGEEC_PREFIX "-- "
#define MAGEEC_ERR(msg) mageecDbg() << MAGEEC_PREFIX << "error: " << msg << std::endl
#define MAGEEC_WARN(msg) mageecDbg() << MAGEEC_PREFIX << "warning: " << msg << std::endl
#define MAGEEC_MSG(msg) mageecDbg() << MAGEEC_PREFIX << msg << std::endl


namespace mageec {


enum class DriverMode {
  kNone,
  kCreateDatabase
};


} // end of namespace mageec


using namespace mageec;


int createDatabase(const std::string &db_path)
{
  Framework framework;

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
                  const std::vector<std::string> &ml_strs,
                  const std::string &metric_str);
{
  Framework framework;

  // Parse the metric string
  Metric metric;
  if (metric_str == "size") {
    metric = kCodeSize;
  }
  else if (metric_str == "time") {
    metric = kTime;
  }
  else if (metric_str == "energy") {
    metric = kEnergy;
  }
  else {
    MAGEEC_ERR("Unknown metric specified '" << metric_str << "'");
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
  const std::vector<mageec::util::UUID> mls;
  for (auto str : ml_strs) {
    mageec::util::Option<mageec::util::UUID> ml_uuid;

    // A wildcard means all known machine learners should be trained
    if (str == "*") {
      mls = framework.getMachineLearners();
      continue;
    }

    // Try and parse the argument as a UUID
    ml_uuid = mageec::util::UUID::parse(str);
    if (ml_uuid && !framework.hasMachineLearner(str)) {
      MAGEEC_WARN("UUID '" << str << "' is not a register machine learner "
                  "and will be ignored");
      continue;
    }

    // Not a UUID, try and load as a shared object
    if (!ml_uuid) {
      ml_uuid = mageec_context.framework->loadMachineLearner(ml_str.get());
    }
    if (!ml_uuid) {
      MAGEEC_WARN("Unable to load machine learner '" << str << "'. This "
                  "machine learner will be ignored");
      continue;
    }
    assert(ml_uuid);

    // add uuid of ml to be trained.
    mls.push_back(ml_uuid);
  }
  if (mls.empty()) {
    MAGEEC_ERR("No machine learners were successfully loaded");
    return -1;
  }

  // Train. This will put the training blobs from the machine learners into
  // the database.
  for (auto ml : mls) {
    db->trainMachineLearner(ml, metric);
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
  std::vector<std::string>  mls;

  bool seen_database = false;
  bool seen_mls = false;

  bool with_help = false;
  bool with_version = false;
  bool with_database_version = false;
  bool with_debug = false;
  bool with_print_mls = false;

  for (int i = 0; i < argc; ++i) {
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
    else if (arg == "--print-machine-learner-interfaces")
      with_print_ml_interfaces = true;
    }
    else if (arg == "--print-trained-machine-learners") {
      with_print_trained_mls = true;
    }
    else if (arg == "--database-version") {
      with_database_version = true;
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
      mls.push_back(std::string(argv[i]));
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

  // Warnings
  if (mode == DriverMode::kCreateDatabase && seen_mls) {
    MAGEEC_WARN("Creation mode specified, '--machine-learner arguments "
                "will be ignored");
  }
  if (with_database_version && !seen_database) {
    MAGEEC_WARN("Cannot get database version, as no database was specified");
  }
  if (with_print_ml_interfaces && !seen_machine_learners) {
    MAGEEC_WARN("Cannot print machine learner interfaces with no specified "
                "machine learners");
  }
  if (with_print_trained_mls && !seen_database) {
    MAGEEC_WARN("Cannot print trained machine learners as no database was "
                "specified");
  }

  // Handle normal arguments

  // Handle modes
  switch (mode) {
    case DriverMode::kCreateDatabase:
      return createDatabase(db_str.get());
    case DriverMode::kTrainDatabase:
      return trainDatabase(db_str.get(), mls);
    default:
      break;
  }
  return 0;
}
