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

//===--------------------------- MAGEEC database --------------------------===//
//
// This file defines the main interface to the MAGEEC database. This includes
// the interfaces for feature extraction, compilation and access to the
// machine learners. It also provides methods to query an existing database,
// as well as create a new one.
//
//===----------------------------------------------------------------------===//

#include "mageec/Database.h"
#include "mageec/DatabaseQuery.h"
#include "mageec/ML.h"
#include "mageec/TrainedML.h"
#include "mageec/Types.h"
#include "mageec/Util.h"

#include "sqlite3.h"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace mageec {

// Init the version of the database
const util::Version Database::version(MAGEEC_DATABASE_VERSION_MAJOR,
                                      MAGEEC_DATABASE_VERSION_MINOR,
                                      MAGEEC_DATABASE_VERSION_PATCH);

//===------------------- Database table creation queries ------------------===//

// metadata table creation
static const char *const create_metadata_table = "CREATE TABLE Metadata("
                                                 "field INTEGER PRIMARY KEY, "
                                                 "value TEXT NOT NULL"
                                                 ")";

// database feature table creation strings
static const char *const create_feature_type_table =
    "CREATE TABLE FeatureType("
    "feature_type_id INTEGER PRIMARY KEY, "
    "feature_type    INTEGER NOT NULL"
    ")";

static const char *const create_feature_instance_table =
    "CREATE TABLE FeatureInstance("
    "feature_id      INTEGER PRIMARY KEY, "
    "feature_type_id INTEGER NOT NULL, "
    "value           BLOB NOT NULL, "
    "FOREIGN KEY(feature_type_id) REFERENCES FeatureType(feature_type_id)"
    ")";

static const char *const create_feature_set_feature_table =
    "CREATE TABLE FeatureSetFeature("
    "feature_set_id INTEGER NOT NULL, "
    "feature_id     INTEGER NOT NULL, "
    "UNIQUE(feature_set_id, feature_id), "
    "FOREIGN KEY(feature_id) REFERENCES FeatureInstance(feature_id)"
    ")";

static const char *const create_feature_group_set_table =
    "CREATE TABLE FeatureGroupSet("
    "feature_group_id INTEGER NOT NULL, "
    "feature_set_id   INTEGER NOT NULL, "
    "UNIQUE(feature_group_id, feature_set_id)"
    ")";

// database parameter table creation strings
static const char *const create_parameter_type_table =
    "CREATE TABLE ParameterType("
    "parameter_type_id INTEGER PRIMARY KEY, "
    "parameter_type    INTEGER NOT NULL"
    ")";

static const char *const create_parameter_instance_table =
    "CREATE TABLE ParameterInstance("
    "parameter_id      INTEGER PRIMARY KEY, "
    "parameter_type_id INTEGER NOT NULL, "
    "value             BLOB NOT NULL, "
    "FOREIGN KEY(parameter_type_id) REFERENCES ParameterType(parameter_type_id)"
    ")";

static const char *const create_parameter_set_parameter_table =
    "CREATE TABLE ParameterSetParameter("
    "parameter_set_id INTEGER NOT NULL, "
    "parameter_id     INTEGER NOT NULL, "
    "UNIQUE(parameter_set_id, parameter_id), "
    "FOREIGN KEY(parameter_id) REFERENCES ParameterInstance(parameter_id)"
    ")";

// pass sequence table creation strings
static const char *const create_pass_sequence_pass_table =
    "CREATE TABLE PassSequencePass("
    "pass_id          INTEGER PRIMARY KEY, "
    "pass_sequence_id INTEGER NOT NULL, "
    "pass_pos         INTEGER NOT NULL, "
    "pass_name_id     TEXT NOT NULL, "
    "parameter_set_id INTEGER, "
    "UNIQUE(pass_sequence_id, pass_id), "
    "UNIQUE(pass_id, pass_pos)"
    ")";

// compilation table creation strings
static const char *const create_compilation_table =
    "CREATE TABLE Compilation("
    "compilation_id   INTEGER PRIMARY KEY, "
    "feature_group_id INTEGER NOT NULL, "
    "parameter_set_id INTEGER, "
    "pass_sequence_id INTEGER"
    ")";

static const char *const create_compilation_feature_table =
    "CREATE TABLE CompilationFeature("
    "compilation_id   INTEGER NOT NULL, "
    "pass_id          INTEGER NOT NULL, "
    "feature_group_id INTEGER NOT NULL, "
    "UNIQUE(compilation_id, pass_id), "
    "FOREIGN KEY(compilation_id) REFERENCES Compilation(compilation_id)"
    ")";

// result table creation strings
static const char *const create_result_table =
    "CREATE TABLE Result("
    "compilation_id INTEGER NOT NULL, "
    "metric         INTEGER NOT NULL, "
    "result         INTEGER NOT NULL, "
    "UNIQUE(compilation_id, metric), "
    "FOREIGN KEY(compilation_id) REFERENCES Compilation(compilation_id)"
    ")";

// machine learner table creation strings
static const char *const create_machine_learner_table =
    "CREATE TABLE MachineLearner("
    "ml_id    BLOB, "
    "metric   INTEGER, "
    "ml_blob  BLOB NOT NULL, "
    "UNIQUE(ml_id, metric)"
    ")";

// debug table creation
static const char *const create_program_unit_debug_table =
    "CREATE TABLE ProgramUnitDebug("
    "compilation_id INTEGER PRIMARY KEY, "
    "name           TEXT NOT NULL, "
    "type           TEXT NOT NULL, "
    "parent_id      INTEGER, "
    "FOREIGN KEY(compilation_id) REFERENCES Compilation(compilation_id), "
    "FOREIGN KEY(parent_id) REFERENCES Compilation(compilation_id)"
    ")";

static const char *const create_feature_debug_table =
    "CREATE TABLE FeatureDebug("
    "feature_type_id INTEGER PRIMARY KEY, "
    "name            TEXT NOT NULL, "
    "FOREIGN KEY(feature_type_id) REFERENCES FeatureType(feature_type_id)"
    ")";

static const char *const create_parameter_debug_table =
    "CREATE TABLE ParameterDebug("
    "parameter_type_id INTEGER PRIMARY KEY, "
    "name              TEXT NOT NULL, "
    "FOREIGN KEY(parameter_type_id) REFERENCES ParameterType(parameter_type_id)"
    ")";

//===-------------------- Database implementation -------------------------===//

std::unique_ptr<Database>
Database::createDatabase(std::string db_path,
                         std::map<util::UUID, IMachineLearner *> mls) {
  // Fail if the file already exists
  std::ifstream f(db_path.c_str());
  if (f.good()) {
    return nullptr;
  }
  f.close();

  sqlite3 *db;
  int res = sqlite3_open(db_path.c_str(), &db);
  if (db == nullptr) {
    return nullptr;
  }
  if (res != SQLITE_OK) {
    sqlite3_close(db);
    return nullptr;
  }
  return std::unique_ptr<Database>(new Database(*db, mls, true));
}

std::unique_ptr<Database>
Database::loadDatabase(std::string db_path,
                       std::map<util::UUID, IMachineLearner *> mls) {
  // Fail if the file does not already exist
  std::ifstream f(db_path.c_str());
  if (!f.good()) {
    return nullptr;
  }
  f.close();

  sqlite3 *db;
  int res = sqlite3_open(db_path.c_str(), &db);
  if (db == nullptr) {
    return nullptr;
  }
  if (res != SQLITE_OK) {
    sqlite3_close(db);
    return nullptr;
  }
  return std::unique_ptr<Database>(new Database(*db, mls, false));
}

std::unique_ptr<Database>
Database::getDatabase(std::string db_path,
                      std::map<util::UUID, IMachineLearner *> mls) {
  // First try and load the database, if that fails try and create it
  std::unique_ptr<Database> db;
  db = loadDatabase(db_path, mls);
  if (db) {
    return db;
  }
  db = createDatabase(db_path, mls);
  return db;
}

Database::Database(sqlite3 &db, std::map<util::UUID, IMachineLearner *> mls,
                   bool create)
    : m_db(&db), m_mls(mls) {
  // Enable foreign keys (requires sqlite 3.6.19 or above)
  // If foreign keys are not available the database is still usable, but no
  // foreign key checking will do done.
  DatabaseQuery(*m_db, "PRAGMA foreign_keys = ON").execute().assertDone();

  if (create) {
    init_db(*m_db);
    validate();
  } else {
    if (!isCompatible()) {
      // TODO: trigger exception
      assert(0 && "Loaded incompatible database");
    }
    validate();
  }
}

Database::~Database(void) {
  int res = sqlite3_close(m_db);
  if (res != SQLITE_OK) {
    MAGEEC_DEBUG("Unable to close mageec database:\n" << sqlite3_errmsg(m_db));
  }
  assert(res == SQLITE_OK && "Unable to close mageec database!");
}

void Database::init_db(sqlite3 &db) {
  // Create the entire database in a single transaction
  DatabaseTransaction db_transaction(&db);

  // Create table to hold database metadata
  DatabaseQuery(db, create_metadata_table).execute().assertDone();

  // Create tables to hold features
  DatabaseQuery(db, create_feature_type_table).execute().assertDone();
  DatabaseQuery(db, create_feature_instance_table).execute().assertDone();
  DatabaseQuery(db, create_feature_set_feature_table).execute().assertDone();
  DatabaseQuery(db, create_feature_group_set_table).execute().assertDone();

  // Tables to hold parameters
  DatabaseQuery(db, create_parameter_type_table).execute().assertDone();
  DatabaseQuery(db, create_parameter_instance_table).execute().assertDone();
  DatabaseQuery(db, create_parameter_set_parameter_table)
      .execute()
      .assertDone();

  // Pass sequences
  DatabaseQuery(db, create_pass_sequence_pass_table).execute().assertDone();

  // Compilation
  DatabaseQuery(db, create_compilation_table).execute().assertDone();
  DatabaseQuery(db, create_compilation_feature_table).execute().assertDone();

  // Results
  DatabaseQuery(db, create_result_table).execute().assertDone();

  // Machine learner
  DatabaseQuery(db, create_machine_learner_table).execute().assertDone();

  // Debug tables
  DatabaseQuery(db, create_program_unit_debug_table).execute().assertDone();
  DatabaseQuery(db, create_feature_debug_table).execute().assertDone();
  DatabaseQuery(db, create_parameter_debug_table).execute().assertDone();

  // Manually insert the version into the metadata table
  DatabaseQuery query = DatabaseQueryBuilder(db)
                        << "INSERT INTO Metadata(field, value) VALUES("
                        << QueryParamType::kInteger << ", "
                        << QueryParamType::kText << ")";
  query << static_cast<int64_t>(MetadataField::kDatabaseVersion);
  query << std::string(Database::version);
  query.execute().assertDone();

  // Finish this transaction before setting metadata (which may create its
  // own transactions.
  db_transaction.commit();

  // Add other metadata now that the database is in a valid state
  // setMetadata();
}

void Database::validate(void) {
  assert(isCompatible() && "Cannot validate incompatible database!");
}

bool Database::isCompatible(void) {
  util::Version db_version = getVersion();
  return (db_version == Database::version);
}

util::Version Database::getVersion(void) {
  std::string str = getMetadata(MetadataField::kDatabaseVersion);

  unsigned major, minor, patch;
  int res = sscanf(str.c_str(), "%u.%u.%u", &major, &minor, &patch);

  assert(res == 3 && "Database has a malformed version number");
  return util::Version(major, minor, patch);
}

std::vector<TrainedML> Database::getTrainedMachineLearners(void) {
  assert(isCompatible());

  // Store for each of the trained machine learners as they are extracted
  // from the database.
  std::vector<TrainedML> trained_mls;

  DatabaseQuery query = DatabaseQueryBuilder(*m_db)
                        << "SELECT metric, ml_blob FROM MachineLearner WHERE "
                        << "ml_id = " << QueryParamType::kBlob;

  for (auto I : m_mls) {
    const util::UUID uuid = I.first;
    IMachineLearner &ml = *I.second;

    query.clearAllBindings();
    query << std::vector<uint8_t>(uuid.data().begin(), uuid.data().end());

    auto res = query.execute();
    res.assertDone();
    if (res.numColumns() == 2) {
      Metric metric = static_cast<Metric>(res.getInteger(0));
      std::vector<uint8_t> ml_blob = res.getBlob(1);

      TrainedML trained_ml(ml, metric, ml_blob);
      trained_mls.push_back(trained_ml);
    } else {
      assert(res.numColumns() == 0);
    }
  }

  return trained_mls;
}

std::string Database::getMetadata(MetadataField field) {
  std::string value;

  DatabaseQuery query =
      DatabaseQueryBuilder(*m_db)
      << "SELECT value FROM Metadata WHERE field=" << QueryParamType::kInteger;
  query << static_cast<int64_t>(field);

  DatabaseQueryIterator res = query.execute();
  if (!res.done()) {
    assert(res.numColumns() == 1);
    value = res.getText(0);

    res.next().assertDone();
  }
  return value;
}

void Database::setMetadata(MetadataField field, std::string value) {
  assert(isCompatible());

  DatabaseQuery query = DatabaseQueryBuilder(*m_db)
                        << "INSERT INTO Metadata(field, value) VALUES("
                        << QueryParamType::kText << ", "
                        << QueryParamType::kInteger << ")";
  query << static_cast<int64_t>(field) << value;

  query.execute().assertDone();
}

//===------------------- Feature extractor interface-----------------------===//

FeatureSetID Database::newFeatureSet(FeatureSet features) {
  DatabaseQuery get_features =
      DatabaseQueryBuilder(*m_db)
      << "SELECT FeatureInstance.feature_type_id, FeatureInstance.value "
      << "FROM FeatureSetFeature, FeatureInstance "
      << "WHERE "
      << "FeatureSetFeature.feature_id = FeatureInstance.feature_id AND "
      << "FeatureSetFeature.feature_set_id = " << QueryParamType::kInteger;

  // FIXME: This should check that the keys are identical if a conflict
  // arises.
  DatabaseQuery insert_feature_type =
      DatabaseQueryBuilder(*m_db)
      << "INSERT OR IGNORE INTO FeatureType(feature_type_id, feature_type) "
      << "VALUES (" << QueryParamType::kInteger << ", "
      << QueryParamType::kInteger << ")";

  DatabaseQuery insert_feature =
      DatabaseQueryBuilder(*m_db)
      << "INSERT INTO FeatureInstance(feature_type_id, value) VALUES ("
      << QueryParamType::kInteger << ", " << QueryParamType::kBlob << ")";

  DatabaseQuery insert_into_feature_set =
      DatabaseQueryBuilder(*m_db)
      << "INSERT INTO FeatureSetFeature(feature_set_id, feature_id) VALUES ("
      << QueryParamType::kInteger << ", " << QueryParamType::kInteger << ")";

  // FIXME: This should check that the keys are identical if a conflict
  // arises.
  DatabaseQuery insert_feature_debug =
      DatabaseQueryBuilder(*m_db)
      << "INSERT OR IGNORE INTO FeatureDebug(feature_type_id, name) VALUES ("
      << QueryParamType::kInteger << ", " << QueryParamType::kText << ")";

  // Sort the feature in the feature set using a map, then serialize to a
  // a blob and hash. The hash forms the identifier for the feature set
  // in the database.
  std::map<unsigned, std::vector<uint8_t>> feature_map;
  for (auto I : features) {
    feature_map[I->getFeatureID()] = I->toBlob();
  }
  std::vector<uint8_t> blob;
  for (auto I : feature_map) {
    util::write16LE(blob, I.first);
    blob.insert(blob.end(), I.second.begin(), I.second.end());
  }
  FeatureSetID feature_set_id = static_cast<FeatureSetID>(
      util::crc64(blob.data(), static_cast<unsigned>(blob.size())));

  // Whole operation in a single transaction
  DatabaseTransaction db_transaction(m_db);

  // Check if there is already a feature set with this id in the database.
  // If there is, but the feature sets don't actually match, then keep trying
  // the next consecutive id until either the matching feature set is found,
  // or no set is found (signifying that it needs to be created)
  bool feature_id_found = false;
  while (!feature_id_found) {
    // Check if there is already a feature set with this id in the database.
    get_features.clearAllBindings();
    get_features << static_cast<int64_t>(feature_set_id);

    auto feat_iter = get_features.execute();
    if (!feat_iter.done()) {
      // The feature set already exists. Extract the set into a map to
      // compare to the one we hashed.
      std::map<unsigned, std::vector<uint8_t>> db_feature_map;
      for (; !feat_iter.done(); feat_iter = feat_iter.next()) {
        unsigned feat_id = static_cast<unsigned>(feat_iter.getInteger(0));
        std::vector<uint8_t> feat_blob = feat_iter.getBlob(1);

        db_feature_map[feat_id] = feat_blob;
      }
      // Compare the sets
      if ((db_feature_map.size() != feature_map.size()) ||
          !std::equal(feature_map.begin(), feature_map.end(),
                      db_feature_map.begin())) {
        // We have a hash collision, compare against the next consecutive
        // feature set
        uint64_t tmp = static_cast<uint64_t>(feature_set_id);
        feature_set_id = static_cast<FeatureSetID>(tmp + 1);
      } else {
        feature_id_found = true;
      }
    } else {
      // No feature set matching this id has been found, so insert the
      // features into the database.
      for (auto I : features) {
        // clear parameters bindings for all queries
        insert_feature_type.clearAllBindings();
        insert_feature.clearAllBindings();
        insert_into_feature_set.clearAllBindings();
        insert_feature_debug.clearAllBindings();

        // add feature type first if not present
        insert_feature_type << static_cast<int64_t>(I->getFeatureID())
                            << static_cast<int64_t>(I->getType());
        insert_feature_type.execute().assertDone();

        // feature insertion
        insert_feature << static_cast<int64_t>(I->getFeatureID())
                       << I->toBlob();
        insert_feature.execute().assertDone();

        // The feature_id is an integer primary key, and so it is equal to the
        // rowid. We use this to add the feature to the feature set
        int64_t feature_id = sqlite3_last_insert_rowid(m_db);

        // feature set
        insert_into_feature_set << static_cast<int64_t>(feature_set_id)
                                << static_cast<int64_t>(feature_id);
        insert_into_feature_set.execute().assertDone();

        // debug table
        insert_feature_debug << static_cast<int64_t>(I->getFeatureID())
                             << I->getName();
        insert_feature_debug.execute().assertDone();
      }
      feature_id_found = true;
    }
  }
  db_transaction.commit();
  return feature_set_id;
}

FeatureGroupID Database::newFeatureGroup(std::set<FeatureSetID> group) {
  DatabaseQuery get_feature_ids =
      DatabaseQueryBuilder(*m_db)
      << "SELECT feature_set_id FROM FeatureGroupSet "
      << "WHERE feature_group_id = " << QueryParamType::kInteger;

  DatabaseQuery insert_into_feature_group =
      DatabaseQueryBuilder(*m_db)
      << "INSERT INTO FeatureGroupSet(feature_group_id, feature_set_id) "
      << "VALUES (" << QueryParamType::kInteger << ", "
      << QueryParamType::kInteger << ")";

  // Hash the feature ids in the group to produce the group id.
  std::vector<uint8_t> blob;
  for (auto I : group) {
    util::write64LE(blob, static_cast<uint64_t>(I));
  }
  FeatureGroupID group_id = static_cast<FeatureGroupID>(
      util::crc64(blob.data(), static_cast<unsigned>(blob.size())));

  // Whole operation in a single transaction
  DatabaseTransaction db_transaction(m_db);

  // Check for a feature group with this id in the database.
  // If there is, but the feature set ids don't match, then keep trying the
  // next consecutive id until either a matching group is found, or none is.
  bool group_id_found = false;
  while (!group_id_found) {
    // Check for an already existing group
    get_feature_ids.clearAllBindings();
    get_feature_ids << static_cast<int64_t>(group_id);

    auto group_iter = get_feature_ids.execute();
    if (!group_iter.done()) {
      // Group already exists. Extract the group to compare against the one we
      // hashed.
      std::set<FeatureSetID> db_group;
      for (; !group_iter.done(); group_iter = group_iter.next()) {
        FeatureSetID feat_set_id =
            static_cast<FeatureSetID>(group_iter.getInteger(0));
        db_group.insert(feat_set_id);
      }
      if ((db_group.size() != group.size()) ||
          !std::equal(group.begin(), group.end(), db_group.begin())) {
        // Hash collision, compare against the next consecutive group
        uint64_t tmp = static_cast<uint64_t>(group_id);
        group_id = static_cast<FeatureGroupID>(tmp + 1);
      } else {
        group_id_found = true;
      }
    } else {
      // Add all of the feature sets to the group
      for (auto I : group) {
        insert_into_feature_group.clearAllBindings();
        insert_into_feature_group << static_cast<int64_t>(group_id)
                                  << static_cast<int64_t>(I);
        insert_into_feature_group.execute().assertDone();
      }
      group_id_found = true;
    }
  }
  db_transaction.commit();
  return group_id;
}

void Database::addFeaturesAfterPass(FeatureGroupID features,
                                    CompilationID compilation,
                                    PassID after_pass) {
  DatabaseQuery add_feature_group_after_pass =
      DatabaseQueryBuilder(*m_db)
      << "INSERT INTO CompilationFeature("
      << "compilation_id, pass_id, feature_group_id) "
      << "VALUES (" << QueryParamType::kInteger << ", "
      << QueryParamType::kInteger << ", " << QueryParamType::kInteger << ")";

  add_feature_group_after_pass << static_cast<int64_t>(compilation)
                               << static_cast<int64_t>(after_pass)
                               << static_cast<int64_t>(features);
  add_feature_group_after_pass.execute().assertDone();
}

//===----------------------- Compiler interface ---------------------------===//

CompilationID
Database::newCompilation(std::string name, std::string type,
                         FeatureGroupID features, ParameterSetID parameters,
                         util::Option<PassSequenceID> pass_sequence,
                         util::Option<CompilationID> parent) {
  DatabaseQuery insert_into_compilation =
      DatabaseQueryBuilder(*m_db)
      << "INSERT INTO Compilation("
      << "feature_group_id, parameter_set_id, pass_sequence_id) "
      << "VALUES (" << QueryParamType::kInteger << ", "
      << QueryParamType::kInteger << ", " << QueryParamType::kInteger << ")";

  DatabaseQuery insert_compilation_debug =
      DatabaseQueryBuilder(*m_db)
      << "INSERT INTO ProgramUnitDebug("
      << "compilation_id, name, type, parent_id) VALUES("
      << QueryParamType::kInteger << ", " << QueryParamType::kText << ", "
      << QueryParamType::kText << ", " << QueryParamType::kInteger << ")";

  // add in a single transaction
  DatabaseTransaction db_transaction(m_db);

  // Add the compilation
  insert_into_compilation << static_cast<int64_t>(features)
                          << static_cast<int64_t>(parameters);
  if (pass_sequence) {
    insert_into_compilation << static_cast<int64_t>(pass_sequence.get());
  } else {
    insert_into_compilation << nullptr;
  }
  insert_into_compilation.execute().assertDone();

  // The rowid of the insert is the compilation_id, retrieve it
  int64_t row_id = sqlite3_last_insert_rowid(m_db);
  CompilationID compilation_id = static_cast<CompilationID>(row_id);
  assert(row_id != 0 && "compilation_id overflow");

  // Add debug information
  insert_compilation_debug << static_cast<int64_t>(compilation_id) << name
                           << type;
  if (parent) {
    insert_compilation_debug << static_cast<int64_t>(parent.get());
  } else {
    insert_compilation_debug << nullptr;
  }
  insert_compilation_debug.execute().assertDone();

  db_transaction.commit();
  return compilation_id;
}

ParameterSetID Database::newParameterSet(ParameterSet parameters) {
  DatabaseQuery get_parameters =
      DatabaseQueryBuilder(*m_db)
      << "SELECT ParameterInstance.parameter_type_id, ParameterInstance.value "
      << "FROM ParameterSetParameter, ParameterInstance "
      << "WHERE "
      << "ParameterSetParameter.parameter_id = "
      << "ParameterInstance.parameter_id AND "
      << "ParameterSetParameter.parameter_set_id = "
      << QueryParamType::kInteger;

  // FIXME: This should check that the keys are identical if a conflict
  // arises.
  DatabaseQuery insert_parameter_type =
      DatabaseQueryBuilder(*m_db)
      << "INSERT OR IGNORE INTO ParameterType(parameter_type_id, "
         "parameter_type) "
      << "VALUES (" << QueryParamType::kInteger << ", "
      << QueryParamType::kInteger << ")";

  DatabaseQuery insert_parameter =
      DatabaseQueryBuilder(*m_db)
      << "INSERT INTO ParameterInstance(parameter_type_id, value) VALUES ("
      << QueryParamType::kInteger << ", " << QueryParamType::kBlob << ")";

  DatabaseQuery insert_into_parameter_set =
      DatabaseQueryBuilder(*m_db)
      << "INSERT INTO ParameterSetParameter(parameter_set_id, parameter_id) "
      << "VALUES (" << QueryParamType::kInteger << ", "
      << QueryParamType::kInteger << ")";

  // FIXME: This should check that the keys are identical if a conflict
  // arises.
  DatabaseQuery insert_parameter_debug =
      DatabaseQueryBuilder(*m_db)
      << "INSERT OR IGNORE INTO ParameterDebug(parameter_type_id, name) VALUES "
         "(" << QueryParamType::kInteger << ", " << QueryParamType::kText
      << ")";

  // Sort the parameter in the parameter set using a map, then serialize to a
  // a blob and hash. The hash forms the identifier for the parameter set
  // in the database.
  std::map<unsigned, std::vector<uint8_t>> param_map;
  for (auto I : parameters) {
    param_map[I->getParameterID()] = I->toBlob();
  }
  std::vector<uint8_t> blob;
  for (auto I : param_map) {
    util::write16LE(blob, I.first);
    blob.insert(blob.end(), I.second.begin(), I.second.end());
  }
  ParameterSetID param_set_id = static_cast<ParameterSetID>(
      util::crc64(blob.data(), static_cast<unsigned>(blob.size())));

  // Whole operation in a single transaction
  DatabaseTransaction db_transaction(m_db);

  // Check if there is already a parameter set with this id in the database.
  // If there is, but the feature sets don't actually match, then keep trying
  // the next consecutive id until either the matching parameter set is found,
  // or no set is found.
  bool param_set_id_found = false;
  while (!param_set_id_found) {
    // Check if there is already a parameter set with this id in the database.
    get_parameters.clearAllBindings();
    get_parameters << static_cast<int64_t>(param_set_id);

    auto param_iter = get_parameters.execute();
    if (!param_iter.done()) {
      // Parameter set already exists. Extract the set to compare against the
      // one we hashed.
      std::map<unsigned, std::vector<uint8_t>> db_param_map;
      for (; !param_iter.done(); param_iter = param_iter.next()) {
        unsigned param_id = static_cast<unsigned>(param_iter.getInteger(0));
        std::vector<uint8_t> param_blob = param_iter.getBlob(1);

        db_param_map[param_id] = param_blob;
      }
      if ((param_map.size() != db_param_map.size()) ||
          !std::equal(param_map.begin(), param_map.end(),
                      db_param_map.begin())) {
        // We have a hash collision, compare against the next consecutive
        // parameter set
        uint64_t tmp = static_cast<uint64_t>(param_set_id);
        param_set_id = static_cast<ParameterSetID>(tmp + 1);
      } else {
        param_set_id_found = true;
      }
    } else {
      for (auto I : parameters) {
        // clear parameters bindings for all queries
        insert_parameter_type.clearAllBindings();
        insert_parameter.clearAllBindings();
        insert_into_parameter_set.clearAllBindings();
        insert_parameter_debug.clearAllBindings();

        // add parameter type first if not present
        insert_parameter_type << static_cast<int64_t>(I->getParameterID())
                              << static_cast<int64_t>(I->getType());
        insert_parameter_type.execute().assertDone();

        // parameter insertion
        insert_parameter << static_cast<int64_t>(I->getParameterID())
                         << I->toBlob();
        insert_parameter.execute().assertDone();

        // The parameter_id is an integer primary key, and so it is equal to the
        // rowid. We use this to add the parameter to the set
        int64_t parameter_id = sqlite3_last_insert_rowid(m_db);

        // parameter set
        insert_into_parameter_set << static_cast<int64_t>(param_set_id)
                                  << parameter_id;
        insert_into_parameter_set.execute().assertDone();

        // debug table
        insert_parameter_debug << I->getParameterID() << I->getName();
        insert_parameter_debug.execute().assertDone();
      }
      param_set_id_found = true;
    }
  }
  db_transaction.commit();
  return param_set_id;
}

PassSequenceID Database::newPassSequence(std::vector<std::string> pass_names) {
  // FIXME: Introduce pass parameters
  DatabaseQuery get_passes =
      DatabaseQueryBuilder(*m_db)
      << "SELECT pass_name_id FROM PassSequencePass "
      << "WHERE pass_sequence_id = " << QueryParamType::kInteger << " "
      << "ORDER BY pass_pos";

  DatabaseQuery select_pass_sequence_pos =
      DatabaseQueryBuilder(*m_db)
      << "SELECT MAX(pass_pos) FROM PassSequencePass "
      << "WHERE pass_sequence_id = " << QueryParamType::kInteger;

  DatabaseQuery insert_pass_into_sequence =
      DatabaseQueryBuilder(*m_db)
      << "INSERT INTO PassSequencePass("
      << "pass_sequence_id, pass_pos, pass_name_id, parameter_set_id) "
      << "VALUES(" << QueryParamType::kInteger << ", "
      << QueryParamType::kInteger << ", " << QueryParamType::kText << ", "
      << QueryParamType::kInteger << ")";

  // Build a pass sequence id by hashing the passes
  std::vector<uint8_t> blob;
  for (auto pass : pass_names) {
    blob.insert(blob.begin(), pass.begin(), pass.end());
    blob.push_back(0);
  }
  PassSequenceID pass_sequence_id = static_cast<PassSequenceID>(
      util::crc64(blob.data(), static_cast<unsigned>(blob.size())));

  // Do all operations in a single transaction
  DatabaseTransaction db_transaction(m_db);

  bool pass_seq_found = false;
  while (!pass_seq_found) {
    get_passes.clearAllBindings();
    get_passes << static_cast<int64_t>(pass_sequence_id);

    auto pass_iter = get_passes.execute();
    if (!pass_iter.done()) {
      // A pass seqeunce with this id already exists. Extract the set to
      // compare against the one we hashed.
      std::vector<std::string> db_pass_names;
      for (; !pass_iter.done(); pass_iter = pass_iter.next()) {
        std::string pass = pass_iter.getText(0);
        db_pass_names.push_back(pass);
      }
      if ((db_pass_names.size() != pass_names.size()) ||
          !std::equal(pass_names.begin(), pass_names.end(),
                      db_pass_names.begin())) {
        // We have a hash collision, compare against the next consecutive
        // pass sequence.
        uint64_t tmp = static_cast<uint64_t>(pass_sequence_id);
        pass_sequence_id = static_cast<PassSequenceID>(tmp + 1);
      } else {
        pass_seq_found = true;
      }
    } else {
      for (const auto &pass_name : pass_names) {
        // Clear any bound data for the query
        select_pass_sequence_pos.clearAllBindings();
        insert_pass_into_sequence.clearAllBindings();

        // The next pass index is the current maximum for this sequence +1
        select_pass_sequence_pos << static_cast<int64_t>(pass_sequence_id);

        auto res = select_pass_sequence_pos.execute();
        int64_t pass_pos = 0;
        assert(res.numColumns() == 1);
        if (!res.isNull(0)) {
          pass_pos = res.getInteger(0) + 1;
          assert(pass_pos != 0 && "pass_pos overflowed");
        }

        // FIXME: Insert pass parameters if appropriate.
        insert_pass_into_sequence << static_cast<int64_t>(pass_sequence_id)
                                  << pass_pos << pass_name << nullptr;
        insert_pass_into_sequence.execute().assertDone();
      }
      pass_seq_found = true;
    }
  }
  db_transaction.commit();
  return pass_sequence_id;
}

//===------------------------ Results interface ---------------------------===//

void Database::addResults(std::set<InputResult> results) {
  DatabaseQuery insert_result =
      DatabaseQueryBuilder(*m_db)
      << "INSERT INTO RESULT(compilation_id, metric, result) VALUES("
      << QueryParamType::kInteger << ", " << QueryParamType::kInteger << ", "
      << QueryParamType::kInteger << ")";

  // All results in a single transaction
  DatabaseTransaction db_transaction(m_db);

  for (const auto &res : results) {
    insert_result.clearAllBindings();
    insert_result << static_cast<int64_t>(res.compilation_id)
                  << static_cast<int64_t>(res.metric)
                  << static_cast<int64_t>(res.value);
    insert_result.execute().assertDone();
    // FIXME: This will trigger an assertion if the compilation id fails
    // the foreign key constraint.
  }
  db_transaction.commit();
}

//===----------------------- Training interface ---------------------------===//

void Database::trainMachineLearner(util::UUID ml, Metric metric) {
  // Get all of the feature types and parameter types, even if some of them
  // don't occur for this metric. These will all be distinct.
  DatabaseQuery select_feature_types(
      *m_db, "SELECT feature_type_id, feature_type FROM FeatureType");

  DatabaseQuery select_parameter_types(
      *m_db, "SELECT parameter_type_id, parameter_type FROM ParameterType");

  // Get all of the passes encountered in the database
  DatabaseQuery select_pass_names(
      *m_db, "SELECT DISTINCT pass_name_id FROM PassSequencePass");

  // Insert a blob for the provided machine learner and metric
  // This will fail if there is already training data
  DatabaseQuery insert_blob =
      DatabaseQueryBuilder(*m_db)
      << "INSERT INTO MachineLearner(ml_id, metric, ml_blob) VALUES ("
      << QueryParamType::kBlob << ", " << QueryParamType::kInteger << ", "
      << QueryParamType::kBlob << ")";

  // Get the machine learner interface
  auto res = m_mls.find(ml);
  assert(res != m_mls.end() && "Cannot train an unregistered machine learner");
  const IMachineLearner &i_ml = *res->second;

  // Collect all information in a single transaction
  DatabaseTransaction db_transaction(m_db);

  std::set<FeatureDesc> feature_descs;
  std::set<ParameterDesc> parameter_descs;
  std::set<std::string> pass_names;

  for (auto feat_iter = select_feature_types.execute(); !feat_iter.done();
       feat_iter = feat_iter.next()) {
    assert(feat_iter.numColumns() == 2);
    FeatureDesc desc = {static_cast<unsigned>(feat_iter.getInteger(0)),
                        static_cast<FeatureType>(feat_iter.getInteger(1))};
    feature_descs.insert(desc);
  }

  for (auto param_iter = select_parameter_types.execute(); !param_iter.done();
       param_iter = param_iter.next()) {
    assert(param_iter.numColumns() == 2);
    ParameterDesc desc = {static_cast<unsigned>(param_iter.getInteger(0)),
                          static_cast<ParameterType>(param_iter.getInteger(1))};
    parameter_descs.insert(desc);
  }

  for (auto pass_iter = select_pass_names.execute(); !pass_iter.done();
       pass_iter = pass_iter.next()) {
    assert(pass_iter.numColumns() == 1);
    std::string name = pass_iter.getText(0);
    pass_names.insert(name);
  }

  // TODO: Read pass names

  db_transaction.commit();

  // Iterator to select each set of results in turn
  ResultIterator results(*m_db, metric);

  // Retrieve the blob and then insert it into the database
  auto blob = i_ml.train(feature_descs, parameter_descs, pass_names,
                         std::move(results));

  // FIXME: Handle case where the blob is empty. (causes a failure when
  // running the database query).

  insert_blob << std::vector<uint8_t>(std::begin(ml.data()),
                                      std::end(ml.data()))
              << static_cast<int64_t>(metric) << blob;
  insert_blob.execute().assertDone();
}

//===------------------------ Result Iterator -----------------------------===//

ResultIterator::ResultIterator(sqlite3 &db, Metric metric)
    : m_db(&db), m_metric(metric) {
  // Get each compilation and its accompanying results
  DatabaseQueryBuilder select_compilation_result =
      DatabaseQueryBuilder(*m_db)
      << "SELECT "
      << "Compilation.feature_group_id, Compilation.parameter_set_id, "
      << "Compilation.pass_sequence_id, Result.result "
      << "FROM Compilation, Result "
      << "WHERE "
      << "Compilation.compilation_id = Result.compilation_id AND "
      << "Result.metric = " << QueryParamType::kInteger << " "
      << "ORDER BY Compilation.compilation_id";
  m_query.reset(new DatabaseQuery(select_compilation_result));
  *m_query << static_cast<int64_t>(metric);

  m_result_iter.reset(new DatabaseQueryIterator(m_query->execute()));
}

ResultIterator::ResultIterator(ResultIterator &&other)
    : m_db(other.m_db), m_metric(other.m_metric),
      m_query(std::move(other.m_query)),
      m_result_iter(std::move(other.m_result_iter)) {
  other.m_db = nullptr;
}

ResultIterator &ResultIterator::operator=(ResultIterator &&other) {
  m_db = other.m_db;
  m_metric = other.m_metric;
  m_query = std::move(other.m_query);
  m_result_iter = std::move(other.m_result_iter);

  other.m_db = nullptr;
  return *this;
}

util::Option<Result> ResultIterator::operator*() {
  // Get all of the features in a feature group
  DatabaseQuery select_features =
      DatabaseQueryBuilder(*m_db)
      << "SELECT "
      << "FeatureInstance.feature_type_id, FeatureType.feature_type, "
      << "FeatureInstance.value "
      << "FROM FeatureInstance, FeatureType, FeatureSetFeature, "
      << "FeatureGroupSet "
      << "WHERE "
      << "FeatureInstance.feature_id = FeatureSetFeature.feature_id AND "
      << "FeatureType.feature_type_id = FeatureInstance.feature_type_id AND "
      << "FeatureSetFeature.feature_set_id = FeatureGroupSet.feature_set_id "
      << "AND "
      << "FeatureGroupSet.feature_group_id = " << QueryParamType::kInteger;

  // Get all of the parameters in a parameter set
  DatabaseQuery select_parameters =
      DatabaseQueryBuilder(*m_db)
      << "SELECT "
      << "ParameterInstance.parameter_id, ParameterInstance.parameter_type_id, "
      << "ParameterInstance.value "
      << "FROM ParameterInstance, ParameterSetParameter "
      << "WHERE "
      << "ParameterInstance.parameter_id = ParameterSetParameter.parameter_id "
         "AND "
      << "ParameterSetParameter.parameter_set_id = "
      << QueryParamType::kInteger;

  if (m_result_iter->done()) {
    return util::Option<Result>();
  }

  // Perform the extraction in a single transaction
  DatabaseTransaction db_transaction(m_db);

  assert(m_result_iter->numColumns() == 4);

  FeatureGroupID feature_group =
      static_cast<FeatureGroupID>(m_result_iter->getInteger(0));

  util::Option<ParameterSetID> param_set;
  if (!m_result_iter->isNull(1)) {
    param_set = static_cast<ParameterSetID>(m_result_iter->getInteger(1));
  }
  util::Option<PassSequenceID> pass_seq;
  if (!m_result_iter->isNull(2)) {
    pass_seq = static_cast<PassSequenceID>(m_result_iter->getInteger(2));
  }
  uint64_t value = static_cast<uint64_t>(m_result_iter->getInteger(3));

  // Feature and parameter set
  FeatureSet features;
  ParameterSet parameters;

  // Retrieve the features
  select_features << static_cast<int64_t>(feature_group);
  for (auto feature_iter = select_features.execute(); !feature_iter.done();
       feature_iter = feature_iter.next()) {
    assert(feature_iter.numColumns() == 3);

    unsigned feature_id = static_cast<unsigned>(feature_iter.getInteger(0));
    FeatureType feature_type =
        static_cast<FeatureType>(feature_iter.getInteger(1));
    auto feature_blob = feature_iter.getBlob(2);

    // TODO: Also retrieve feature names
    switch (feature_type) {
    case FeatureType::kBool:
      features.add(BoolFeature::fromBlob(feature_id, feature_blob, {}));
      break;
    case FeatureType::kInt:
      features.add(IntFeature::fromBlob(feature_id, feature_blob, {}));
      break;
    }
  }

  // Retrieve parameters
  if (param_set) {
    select_parameters << static_cast<int64_t>(param_set.get());
    for (auto param_iter = select_parameters.execute(); !param_iter.done();
         param_iter = param_iter.next()) {
      assert(param_iter.numColumns() == 3);

      unsigned param_id = static_cast<unsigned>(param_iter.getInteger(0));
      ParameterType param_type =
          static_cast<ParameterType>(param_iter.getInteger(1));
      auto param_blob = param_iter.getBlob(2);

      // TODO: Also retrieve parameter names
      switch (param_type) {
      case ParameterType::kBool:
        parameters.add(BoolParameter::fromBlob(param_id, param_blob, {}));
        break;
      case ParameterType::kRange:
        parameters.add(RangeParameter::fromBlob(param_id, param_blob, {}));
        break;
      }
    }
  }
  db_transaction.commit();
  return Result(features, parameters, value);
}

ResultIterator ResultIterator::next() {
  if (m_result_iter->done()) {
    return std::move(*this);
  } else {
    *m_result_iter = m_result_iter->next();
    return std::move(*this);
  }
}

DatabaseTransaction::DatabaseTransaction(sqlite3 *db)
    : m_is_committed(false), m_db(db) {
  DatabaseQuery start_transaction(*m_db, "BEGIN TRANSACTION");
  start_transaction.execute().assertDone();
  m_is_init = true;
}

DatabaseTransaction::~DatabaseTransaction() {
  if (m_is_init && !m_is_committed) {
    DatabaseQuery rollback_transaction(*m_db, "ROLLBACK");
    rollback_transaction.execute().assertDone();
  }
}

DatabaseTransaction::DatabaseTransaction(DatabaseTransaction &&other)
    : m_is_committed(std::move(other.m_is_committed)),
      m_db(std::move(other.m_db)) {
  assert(other.m_is_init && "Cannot move from a transaction which has already "
                            "been moved");
  other.m_is_init = false;
  m_is_init = true;
}

DatabaseTransaction &DatabaseTransaction::
operator=(DatabaseTransaction &&other) {
  assert(other.m_is_init && "Cannot move from a transaction which has already "
                            "been moved");

  m_is_committed = std::move(other.m_is_committed);
  m_db = std::move(other.m_db);

  other.m_is_init = false;
  m_is_init = true;
  return *this;
}

void DatabaseTransaction::commit() {
  assert(m_is_init && "Transaction has been moved");
  assert(!m_is_committed && "Transaction has already been committed");

  DatabaseQuery commit_transaction(*m_db, "COMMIT");
  commit_transaction.execute().assertDone();
  m_is_committed = true;
}

} // end of namespace mageec
