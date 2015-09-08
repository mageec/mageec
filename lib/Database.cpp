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

#include <cassert>
#include <cstdio>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#ifdef MAGEEC_DEBUG
  #include <iostream>
#endif // MAGEEC_DEBUG

#include "mageec/DatabaseQuery.h"
#include "mageec/TrainedML.h"
#include "mageec/Types.h"
#include "mageec/Util.h"

#include "sqlite3.h"


namespace mageec {


// Init the version of the database
const util::Version
Database::version(MAGEEC_DATABASE_VERSION_MAJOR,
                  MAGEEC_DATABASE_VERSION_MINOR,
                  MAGEEC_DATABASE_VERSION_PATCH);


//===------------------- Database table creation queries ------------------===//


// metadata table creation
static const char * const create_metadata_table =
"CREATE TABLE Metadata("
  "field INTEGER PRIMARY KEY, "
  "value TEXT NOT NULL"
")";

// database feature table creation strings
static const char * const create_feature_type_table =
"CREATE TABLE FeatureType("
  "feature_type_id INTEGER PRIMARY KEY, "
  "feature_type    INTEGER NOT NULL"
")";

static const char * const create_feature_instance_table =
"CREATE TABLE FeatureInstance("
  "feature_id      INTEGER PRIMARY KEY, "
  "feature_type_id INTEGER NOT NULL, "
  "value           BLOB NOT NULL, "
  "FOREIGN KEY(feature_type_id) REFERENCES FeatureType(feature_type_id)"
")";

static const char * const create_feature_set_table =
"CREATE TABLE FeatureSet(feature_set_id INTEGER PRIMARY KEY)";

static const char * const create_feature_set_feature_table =
"CREATE TABLE FeatureSetFeature("
  "feature_set_id INTEGER NOT NULL, "
  "feature_id     INTEGER NOT NULL, "
  "UNIQUE(feature_set_id, feature_id), "
  "FOREIGN KEY(feature_set_id) REFERENCES FeatureSet(feature_set_id), "
  "FOREIGN KEY(feature_id) REFERENCES FeatureInstance(feature_id)"
")";

static const char * const create_feature_group_table =
"CREATE TABLE FeatureGroup(feature_group_id INTEGER PRIMARY KEY)";

static const char * const create_feature_group_set_table =
"CREATE TABLE FeatureGroupSet("
  "feature_group_id INTEGER NOT NULL, "
  "feature_set_id   INTEGER NOT NULL, "
  "UNIQUE(feature_group_id, feature_set_id), "
  "FOREIGN KEY(feature_group_id) REFERENCES FeatureGroup(feature_group_id), "
  "FOREIGN KEY(feature_set_id) REFERENCES FeatureSet(feature_set_id)"
")";

// database parameter table creation strings
static const char * const create_parameter_type_table =
"CREATE TABLE ParameterType("
  "parameter_type_id INTEGER PRIMARY KEY, "
  "parameter_type    INTEGER NOT NULL"
")";

static const char * const create_parameter_instance_table =
"CREATE TABLE ParameterInstance("
  "parameter_id      INTEGER PRIMARY KEY, "
  "parameter_type_id INTEGER NOT NULL, "
  "value             BLOB NOT NULL, "
  "FOREIGN KEY(parameter_type_id) REFERENCES ParameterType(parameter_type_id)"
")";

static const char * const create_parameter_set_table =
"CREATE TABLE ParameterSet(parameter_set_id INTEGER PRIMARY KEY)";

static const char * const create_parameter_set_parameter_table =
"CREATE TABLE ParameterSetParameter("
  "parameter_set_id INTEGER NOT NULL, "
  "parameter_id     INTEGER NOT NULL, "
  "UNIQUE(parameter_set_id, parameter_id), "
  "FOREIGN KEY(parameter_set_id) REFERENCES ParameterSet(parameter_set_id), "
  "FOREIGN KEY(parameter_id) REFERENCES ParameterInstance(parameter_id)"
")";

// pass and pass sequence table creation strings
static const char * const create_pass_sequence_table =
"CREATE TABLE PassSequence(pass_sequence_id INTEGER PRIMARY KEY)";

static const char * const create_pass_sequence_pass_table =
"CREATE TABLE PassSequencePass("
  "pass_id          INTEGER PRIMARY KEY, "
  "pass_sequence_id INTEGER NOT NULL, "
  "pass_pos         INTEGER NOT NULL, "
  "pass_name_id     TEXT NOT NULL, "
  "parameter_set_id INTEGER, "
  "UNIQUE(pass_sequence_id, pass_id), "
  "UNIQUE(pass_id, pass_pos), "
  "FOREIGN KEY(pass_sequence_id) REFERENCES PassSequence(pass_sequence_id), "
  "FOREIGN KEY(parameter_set_id) REFERENCES ParameterSet(parameter_set_id)"
")";

// compilation table creation strings
static const char * const create_compilation_table =
"CREATE TABLE Compilation("
  "compilation_id   INTEGER PRIMARY KEY, "
  "feature_group_id INTEGER NOT NULL, "
  "parameter_set_id INTEGER NOT NULL, "
  "pass_sequence_id INTEGER, "
  "FOREIGN KEY(feature_group_id) REFERENCES FeatureGroup(feature_group_id), "
  "FOREIGN KEY(pass_sequence_id) REFERENCES PassSequence(pass_sequence_id), "
  "FOREIGN KEY(parameter_set_id) REFERENCES ParameterSet(parameter_set_id)"
")";

static const char * const create_compilation_feature_table =
"CREATE TABLE CompilationFeature("
  "compilation_id   INTEGER NOT NULL, "
  "pass_id          INTEGER NOT NULL, "
  "feature_group_id INTEGER NOT NULL, "
  "UNIQUE(compilation_id, pass_id), "
  "FOREIGN KEY(compilation_id) REFERENCES Compilation(compilation_id), "
  "FOREIGN KEY(pass_id)        REFERENCES PassSequencePass(pass_id), "
  "FOREIGN KEY(feature_group_id) REFERENCES FeatureGroup(feature_group_id)"
")";

// result table creation strings
static const char * const create_result_table =
"CREATE TABLE Result("
  "compilation_id INTEGER NOT NULL, "
  "metric         INTEGER NOT NULL, "
  "result         INTEGER NOT NULL, "
  "UNIQUE(compilation_id, metric), "
  "FOREIGN KEY(compilation_id) REFERENCES Compilation(compilation_id)"
")";

// machine learner table creation strings
static const char * const create_machine_learner_table =
"CREATE TABLE MachineLearner("
  "ml_id    BLOB, "
  "metric   INTEGER, "
  "ml_blob  BLOB NOT NULL, "
  "UNIQUE(ml_id, metric)"
")";

// debug table creation
static const char * const create_program_unit_debug_table =
"CREATE TABLE ProgramUnitDebug("
  "compilation_id INTEGER PRIMARY KEY, "
  "name           TEXT NOT NULL, "
  "type           TEXT NOT NULL, "
  "parent_id      INTEGER, "
  "FOREIGN KEY(compilation_id) REFERENCES Compilation(compilation_id), "
  "FOREIGN KEY(parent_id) REFERENCES Compilation(compilation_id)"
")";

static const char * const create_feature_debug_table =
"CREATE TABLE FeatureDebug("
  "feature_type_id INTEGER PRIMARY KEY, "
  "name            TEXT NOT NULL, "
  "FOREIGN KEY(feature_type_id) REFERENCES FeatureType(feature_type_id)"
")";

static const char * const create_parameter_debug_table =
"CREATE TABLE ParameterDebug("
  "parameter_type_id INTEGER PRIMARY KEY, "
  "name              TEXT NOT NULL, "
  "FOREIGN KEY(parameter_type_id) REFERENCES ParameterType(parameter_type_id)"
")";


//===-------------------- Database implementation -------------------------===//


Database::Database(std::string db_path,
                   std::map<util::UUID, const IMachineLearner*> mls,
                   bool create)
  : m_db(nullptr), m_mls(mls)
{
  int res = sqlite3_open (db_path.c_str(), &m_db);
  
  assert(res == SQLITE_OK && "Unable to load or create mageec database");
  assert(m_db != nullptr);

  // Enable foreign keys (requires sqlite 3.6.19 or above)
  // If foreign keys are not available the database is still usable, but no
  // foreign key checking will do done.
  DatabaseQuery(*m_db, "PRAGMA foreign_keys = ON").execute().assertDone();

  if (create) {
    init_db(*m_db);
    validate();
  }
  else {
    if (!isCompatible()) {
      // TODO: trigger exception
      assert(0 && "Loaded incompatible database");
    }
    validate();
  }
}

Database::~Database(void)
{
  int res = sqlite3_close(m_db);
#ifdef MAGEEC_DEBUG
  if (res != SQLITE_OK) {
    std::cerr << "Unable to close mageec database" << std::endl
      << sqlite3_errmsg(m_db) << std::endl;
  }
#endif // MAGEEC_DEBUG
  assert(res == SQLITE_OK && "Unable to close mageec database!");
}

void Database::init_db(sqlite3 &db)
{
  DatabaseQuery start_transaction(db, "BEGIN TRANSACTION");
  DatabaseQuery commit_transaction(db, "COMMIT");

  // Create the entire database in a single transaction
  start_transaction.execute().assertDone();

  // Create table to hold database metadata
  DatabaseQuery(db, create_metadata_table).execute().assertDone();

  // Create tables to hold features
  DatabaseQuery(db, create_feature_type_table).execute().assertDone();
  DatabaseQuery(db, create_feature_instance_table).execute().assertDone();
  DatabaseQuery(db, create_feature_set_table).execute().assertDone();
  DatabaseQuery(db, create_feature_set_feature_table).execute().assertDone();
  DatabaseQuery(db, create_feature_group_table).execute().assertDone();
  DatabaseQuery(db, create_feature_group_set_table).execute().assertDone();

  // Tables to hold parameters
  DatabaseQuery(db, create_parameter_type_table).execute().assertDone();
  DatabaseQuery(db, create_parameter_instance_table).execute().assertDone();
  DatabaseQuery(db, create_parameter_set_table).execute().assertDone();
  DatabaseQuery(db, create_parameter_set_parameter_table)
    .execute().assertDone();

  // Pass sequences
  DatabaseQuery(db, create_pass_sequence_table).execute().assertDone();
  DatabaseQuery(db, create_pass_sequence_pass_table).execute().assertDone();

  // Compilation
  DatabaseQuery(db, create_compilation_table).execute().assertDone();
  DatabaseQuery(db, create_compilation_feature_table) .execute().assertDone();

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
      << QueryParamType::kText
    << ")";
  query << static_cast<int64_t>(MetadataField::kDatabaseVersion);
  query << std::string(Database::version);
  query.execute().assertDone();

  // commit changes
  commit_transaction.execute().assertDone();

  // Add other metadata now that the database is in a valid state
  // setMetadata();
}


void Database::validate(void)
{
  assert(isCompatible() && "Cannot validate incompatible database!");
}


bool Database::isCompatible(void)
{
  util::Version db_version = getVersion();
  return (db_version == Database::version);
}


util::Version Database::getVersion(void)
{
  std::string str = getMetadata(MetadataField::kDatabaseVersion);

  unsigned major, minor, patch;
  int res = sscanf(str.c_str(), "%u.%u.%u", &major, &minor, &patch);

  assert(res == 3 && "Database has a malformed version number");
  return util::Version(major, minor, patch);
}


std::vector<TrainedML> Database::getTrainedMachineLearners(void)
{
  assert(isCompatible());

  // Store for each of the trained machine learners as they are extracted
  // from the database.
  std::vector<TrainedML> trained_mls;

  DatabaseQuery query = DatabaseQueryBuilder(*m_db)
    << "SELECT metric, ml_blob FROM MachineLearner WHERE "
      << "ml_id = " << QueryParamType::kBlob;
  
  for (auto I : m_mls) {
    const util::UUID uuid = I.first;
    const IMachineLearner& ml = *I.second;

    query.clearAllBindings();
    query << std::vector<uint8_t>(uuid.data().begin(), uuid.data().end());

    auto res = query.execute();
    res.assertDone();
    assert(res.numColumns() == 2);

    Metric metric = static_cast<Metric>(res.getInteger(0));
    std::vector<uint8_t> ml_blob = res.getBlob(1);

    TrainedML trained_ml(*m_db, ml, metric, ml_blob);
    trained_mls.push_back(trained_ml);
  }

  return trained_mls;
}


std::string Database::getMetadata(MetadataField field)
{
  std::string value;

  DatabaseQuery query = DatabaseQueryBuilder(*m_db)
    << "SELECT value FROM Metadata WHERE field="
      << QueryParamType::kInteger;
  query << static_cast<int64_t>(field);

  DatabaseQueryIterator res = query.execute();
  if (!res.done()) {
    assert(res.numColumns() == 1);
    value = res.getText(0);

    res.next().assertDone();
  }
  return value;
}

void Database::setMetadata(MetadataField field, std::string value)
{
  assert(isCompatible());

  DatabaseQuery query = DatabaseQueryBuilder(*m_db)
    << "INSERT INTO Metadata(field, value) VALUES("
      << QueryParamType::kText << ", "
      << QueryParamType::kInteger
    << ")";
  query << static_cast<int64_t>(field) << value;

  query.execute().assertDone();
}


//===------------------- Feature extractor interface-----------------------===//



FeatureSetID Database::newFeatureSet(const std::vector<FeatureBase*> features)
{
  DatabaseQuery start_transaction(*m_db, "BEGIN TRANSACTION");
  DatabaseQuery commit_transaction(*m_db, "COMMIT");

  DatabaseQuery add_feature_set = DatabaseQueryBuilder(*m_db)
    << "INSERT INTO FeatureSet DEFAULT VALUES";

  // FIXME: This should check that the keys are identical if a conflict
  // arises.
  DatabaseQuery insert_feature_type = DatabaseQueryBuilder(*m_db)
    << "INSERT OR IGNORE INTO FeatureType(feature_type_id, feature_type) "
    << "VALUES ("
      << QueryParamType::kInteger << ", "
      << QueryParamType::kInteger << ")";

  DatabaseQuery insert_feature = DatabaseQueryBuilder(*m_db)
    << "INSERT INTO FeatureInstance(feature_type_id, value) VALUES ("
      << QueryParamType::kInteger << ", "
      << QueryParamType::kBlob << ")";

  DatabaseQuery insert_into_feature_set = DatabaseQueryBuilder(*m_db)
    << "INSERT INTO FeatureSetFeature(feature_set_id, feature_id) VALUES ("
      << QueryParamType::kInteger << ", "
      << QueryParamType::kInteger << ")";

  // FIXME: This should check that the keys are identical if a conflict
  // arises.
  DatabaseQuery insert_feature_debug = DatabaseQueryBuilder(*m_db)
    << "INSERT OR IGNORE INTO FeatureDebug(feature_type_id, name) VALUES ("
      << QueryParamType::kInteger << ", "
      << QueryParamType::kText << ")";

  // Add all features in a single transaction
  start_transaction.execute().assertDone();

  // Add the new feature set, retrieving its id (equal to the row id)
  add_feature_set.execute().assertDone();
  int64_t feature_set_id = sqlite3_last_insert_rowid(m_db);
  assert(feature_set_id != 0 && "feature_set_id overflow");

  for (auto I : features) {
    // clear parameters bindings for all queries
    insert_feature_type.clearAllBindings();
    insert_feature.clearAllBindings();
    insert_into_feature_set.clearAllBindings();
    insert_feature_debug.clearAllBindings();

    // add feature type first if not present
    insert_feature_type
      << static_cast<int64_t>(I->getFeatureID())
      << static_cast<int64_t>(I->getType());
    insert_feature_type.execute().assertDone();

    // feature insertion
    insert_feature << static_cast<int64_t>(I->getFeatureID()) << I->toBlob();
    insert_feature.execute().assertDone();

    // The feature_id is an integer primary key, and so it is equal to the
    // rowid. We use this to add the feature to the feature set
    int64_t feature_id = sqlite3_last_insert_rowid(m_db);

    // feature set
    insert_into_feature_set << feature_set_id << feature_id;
    insert_into_feature_set.execute().assertDone();

    // debug table
    insert_feature_debug << I->getFeatureID() << I->getName();
    insert_feature_debug.execute().assertDone();
  }

  commit_transaction.execute().assertDone();

  return static_cast<FeatureSetID>(feature_set_id);
}


FeatureGroupID Database::newFeatureGroup(std::vector<FeatureSetID> features)
{
  DatabaseQuery start_transaction(*m_db, "BEGIN TRANSACTION");
  DatabaseQuery commit_transaction(*m_db, "COMMIT");

  DatabaseQuery add_feature_group = DatabaseQueryBuilder(*m_db)
    << "INSERT INTO FeatureGroup DEFAULT VALUES";

  DatabaseQuery insert_into_feature_group = DatabaseQueryBuilder(*m_db)
    << "INSERT INTO FeatureGroupSet(feature_group_id, feature_set_id) "
    << "VALUES ("
      << QueryParamType::kInteger << ", "
      << QueryParamType::kInteger << ")";

  // add in a single transaction
  start_transaction.execute().assertDone();

  // Add the new feature group, retrieving its id (equal to the row id)
  add_feature_group.execute().assertDone();
  int64_t feature_group_id = sqlite3_last_insert_rowid(m_db);
  assert(feature_group_id != 0 && "feature_group_id overflow");

  // Add all of the feature sets to the group
  for (auto I : features) {
    insert_into_feature_group.clearAllBindings();
    insert_into_feature_group << feature_group_id << static_cast<int64_t>(I);
    insert_into_feature_group.execute().assertDone();
  }

  // commit the changes
  commit_transaction.execute().assertDone();

  return static_cast<FeatureGroupID>(feature_group_id);
}

void Database::addFeaturesAfterPass(FeatureGroupID features,
                                    CompilationID compilation,
                                    PassID after_pass)
{
  DatabaseQuery add_feature_group_after_pass = DatabaseQueryBuilder(*m_db)
    << "INSERT INTO CompilationFeature("
      << "compilation_id, pass_id, feature_group_id) "
    << "VALUES ("
      << QueryParamType::kInteger << ", "
      << QueryParamType::kInteger << ", "
      << QueryParamType::kInteger << ")";

  add_feature_group_after_pass
    << static_cast<int64_t>(compilation)
    << static_cast<int64_t>(after_pass)
    << static_cast<int64_t>(features);
  add_feature_group_after_pass.execute().assertDone();
}


//===----------------------- Compiler interface ---------------------------===//


CompilationID Database::
newCompilation(std::string name, std::string type,
               FeatureGroupID features,
               ParameterSetID parameters,
               util::Option<PassSequenceID> pass_sequence,
               util::Option<CompilationID> parent)
{
  DatabaseQuery start_transaction(*m_db, "BEGIN TRANSACTION");
  DatabaseQuery commit_transaction(*m_db, "COMMIT");

  DatabaseQuery insert_into_compilation = DatabaseQueryBuilder(*m_db)
    << "INSERT INTO Compilation("
      << "feature_group_id, parameter_set_id, pass_sequence_id) "
    << "VALUES ("
      << QueryParamType::kInteger << ", "
      << QueryParamType::kInteger << ", "
      << QueryParamType::kInteger << ")";

  DatabaseQuery insert_compilation_debug = DatabaseQueryBuilder(*m_db)
    << "INSERT INTO ProgramUnitDebug("
      << "compilation_id, name, type, parent_id) VALUES("
      << QueryParamType::kInteger << ", "
      << QueryParamType::kText << ", "
      << QueryParamType::kText << ", "
      << QueryParamType::kInteger << ")";

  // add in a single transaction
  start_transaction.execute().assertDone();

  // Add the compilation
  insert_into_compilation
    << static_cast<int64_t>(features)
    << static_cast<int64_t>(parameters);
  if (pass_sequence) {
    insert_into_compilation << static_cast<int64_t>(pass_sequence.get());
  }
  else {
    insert_into_compilation << nullptr;
  }
  insert_into_compilation.execute().assertDone();

  // The rowid of the insert is the compilation_id, retrieve it
  int64_t compilation_id = sqlite3_last_insert_rowid(m_db);
  assert(compilation_id != 0 && "compilation_id overflow");

  // Add debug information
  insert_compilation_debug << compilation_id << name << type;
  if (parent) {
    insert_compilation_debug << static_cast<int64_t>(parent.get());
  }
  else {
    insert_compilation_debug << nullptr;
  }
  insert_compilation_debug.execute().assertDone();

  // commit the changes
  commit_transaction.execute().assertDone();

  return static_cast<CompilationID>(compilation_id);
}

ParameterSetID
Database::newParameterSet(const std::vector<ParameterBase*> parameters)
{
  DatabaseQuery start_transaction(*m_db, "BEGIN TRANSACTION");
  DatabaseQuery commit_transaction(*m_db, "COMMIT");

  DatabaseQuery add_parameter_set = DatabaseQueryBuilder(*m_db)
    << "INSERT INTO ParameterSet DEFAULT VALUES";

  // FIXME: This should check that the keys are identical if a conflict
  // arises.
  DatabaseQuery insert_parameter_type = DatabaseQueryBuilder(*m_db)
    << "INSERT OR IGNORE INTO ParameterType(parameter_type_id, parameter_type) "
    << "VALUES ("
      << QueryParamType::kInteger << ", "
      << QueryParamType::kInteger << ")";

  DatabaseQuery insert_parameter = DatabaseQueryBuilder(*m_db)
    << "INSERT INTO ParameterInstance(parameter_type_id, value) VALUES ("
      << QueryParamType::kInteger << ", "
      << QueryParamType::kBlob << ")";

  DatabaseQuery insert_into_parameter_set = DatabaseQueryBuilder(*m_db)
    << "INSERT INTO ParameterSetParameter(parameter_set_id, parameter_id) "
    << "VALUES ("
      << QueryParamType::kInteger << ", "
      << QueryParamType::kInteger << ")";

  // FIXME: This should check that the keys are identical if a conflict
  // arises.
  DatabaseQuery insert_parameter_debug = DatabaseQueryBuilder(*m_db)
    << "INSERT OR IGNORE INTO ParameterDebug(parameter_type_id, name) VALUES ("
      << QueryParamType::kInteger << ", "
      << QueryParamType::kText << ")";

  // Add all parameters in a single transaction
  start_transaction.execute().assertDone();

  // Add the new parameter set, retrieving its id (equal to the row id)
  add_parameter_set.execute().assertDone();
  int64_t parameter_set_id = sqlite3_last_insert_rowid(m_db);
  assert(parameter_set_id != 0 && "parameter_set_id overflow");

  for (auto I : parameters) {
    // clear parameters bindings for all queries
    insert_parameter_type.clearAllBindings();
    insert_parameter.clearAllBindings();
    insert_into_parameter_set.clearAllBindings();
    insert_parameter_debug.clearAllBindings();

    // add parameter type first if not present
    insert_parameter_type
      << static_cast<int64_t>(I->getParameterID())
      << static_cast<int64_t>(I->getType());
    insert_parameter_type.execute().assertDone();

    // parameter insertion
    insert_parameter 
      << static_cast<int64_t>(I->getParameterID())
      << I->toBlob();
    insert_parameter.execute().assertDone();

    // The parameter_id is an integer primary key, and so it is equal to the
    // rowid. We use this to add the parameter to the set
    int64_t parameter_id = sqlite3_last_insert_rowid(m_db);

    // parameter set
    insert_into_parameter_set << parameter_set_id << parameter_id;
    insert_into_parameter_set.execute().assertDone();

    // debug table
    insert_parameter_debug << I->getParameterID() << I->getName();
    insert_parameter_debug.execute().assertDone();
  }

  commit_transaction.execute().assertDone();

  return static_cast<ParameterSetID>(parameter_set_id);
}

PassSequenceID Database::newPassSequence(void)
{
  DatabaseQuery add_pass_sequence(
    *m_db,"INSERT INTO PassSequence DEFAULT VALUES");

  add_pass_sequence.execute().assertDone();
  int64_t pass_sequence_id = sqlite3_last_insert_rowid(m_db);
  assert(pass_sequence_id != 0 && "pass_sequence_id overflow");

  return static_cast<PassSequenceID>(pass_sequence_id);
}

PassID Database::addPass(std::string pass_name, PassSequenceID sequence_id,
                         util::Option<ParameterSetID> parameters)
{
  DatabaseQuery start_transaction(*m_db, "BEGIN TRANSACTION");
  DatabaseQuery commit_transaction(*m_db, "COMMIT");

  DatabaseQuery select_pass_sequence_pos = DatabaseQueryBuilder(*m_db)
    << "SELECT MAX(pass_pos) FROM PassSequencePass "
    << "WHERE pass_sequence_id = " << QueryParamType::kInteger;

  DatabaseQuery insert_pass_into_sequence = DatabaseQueryBuilder(*m_db)
    << "INSERT INTO PassSequencePass("
      << "pass_sequence_id, pass_pos, pass_name_id, parameter_set_id) "
    << "VALUES("
      << QueryParamType::kInteger << ", "
      << QueryParamType::kInteger << ", "
      << QueryParamType::kText << ", "
      << QueryParamType::kInteger << ")";

  // Complete in a single transaction
  start_transaction.execute().assertDone();

  // The next pass index is the current maximum for this sequence +1
  select_pass_sequence_pos << static_cast<int64_t>(sequence_id);

  auto res = select_pass_sequence_pos.execute();
  int64_t pass_pos = 0;
  assert(res.numColumns() == 1);
  if (!res.isNull(0)) {
    pass_pos = res.getInteger(0) + 1;
    assert(pass_pos != 0 && "pass_pos overflowed");
  }

  insert_pass_into_sequence
    << static_cast<int64_t>(sequence_id)
    << pass_pos << pass_name;
  if (parameters) {
    insert_pass_into_sequence << static_cast<int64_t>(parameters.get());
  }
  else {
    insert_pass_into_sequence << nullptr;
  }
  insert_pass_into_sequence.execute().assertDone();

  // The pass id is the primary key, and equal to the row id.
  int64_t pass_id = sqlite3_last_insert_rowid(m_db);

  // Commit the transaction
  commit_transaction.execute().assertDone();

  return static_cast<PassID>(pass_id);
}


//===------------------------ Results interface ---------------------------===//


void Database::addResult(CompilationID compilation,
                         Metric metric,
                         uint64_t result)
{
  DatabaseQuery insert_result = DatabaseQueryBuilder(*m_db)
    << "INSERT INTO Result(compilation_id, metric, result) VALUES("
      << QueryParamType::kInteger << ", "
      << QueryParamType::kInteger << ", "
      << QueryParamType::kInteger << ")";
  insert_result
    << static_cast<int64_t>(compilation)
    << static_cast<int64_t>(metric)
    << static_cast<int64_t>(result);
  insert_result.execute().assertDone();
}


//===----------------------- Training interface ---------------------------===//


std::vector<uint8_t> Database::trainMachineLearner(util::UUID ml,
                                                   Metric metric)
{
  (void)ml;
  (void)metric;
  return std::vector<uint8_t>();
}


//===------------------------ Result Iterator -----------------------------===//


ResultIterator::ResultIterator(sqlite3 &db, Metric metric)
  : m_db(db), m_metric(metric)
{
}

Result ResultIterator::operator*()
{
  return Result(FeatureSet(), ParameterSet(), 0);
}

ResultIterator ResultIterator::operator++()
{
  return *this; // FIXME
}

bool ResultIterator::operator==(const ResultIterator& other) const
{
  (void)other;
  return false;
}

bool ResultIterator::operator!=(const ResultIterator& other) const
{
  (void)other;
  return false;
}


} // end of namespace mageec
