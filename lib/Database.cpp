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
"CREATE TABLE FeatureSet("
  "feature_set_id INTEGER, "
  "feature_id     INTEGER, "
  "UNIQUE(feature_set_id, feature_id), "
  "FOREIGN KEY(feature_id) REFERENCES FeatureInstance(feature_id)"
")";

static const char * const create_feature_group_table =
"CREATE TABLE FeatureGroup("
  "feature_group_id INTEGER, "
  "feature_set_id   INTEGER, "
  "UNIQUE(feature_group_id, feature_set_id), "
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
"CREATE TABLE ParameterSet("
  "parameter_set_id INTEGER, "
  "parameter_id     INTEGER, "
  "UNIQUE(parameter_set_id, parameter_id), "
  "FOREIGN KEY(parameter_id) REFERENCES ParameterInstance(parameter_id)"
")";

// pass and pass sequence table creation strings
static const char * const create_pass_sequence_table =
"CREATE TABLE PassSequence("
  "pass_instance_id INTEGER PRIMARY KEY, "
  "pass_sequence_id INTEGER NOT NULL, "
  "pass_pos         INTEGER NOT NULL, "
  "pass_id          TEXT NOT NULL, "
  "UNIQUE(pass_sequence_id, pass_instance_id), "
  "UNIQUE(pass_instance_id, pass_pos)"
")";

static const char * const create_pass_parameter_table =
"CREATE TABLE PassParameter("
  "pass_instance_id INTEGER PRIMARY KEY, "
  "parameter_set_id INTEGER NOT NULL, "
  "FOREIGN KEY(pass_instance_id) REFERENCES PassSequence(pass_instance_id), "
  "FOREIGN KEY(parameter_set_id) REFERENCES ParameterSet(parameter_set_id)"
")";

// compilation table creation strings
static const char * const create_compilation_table =
"CREATE TABLE Compilation("
  "compilation_id   INTEGER PRIMARY KEY, "
  "feature_group_id INTEGER NOT NULL, "
  "pass_sequence_id INTEGER NOT NULL, "
  "parameter_set_id INTEGER NOT NULL, "
  "FOREIGN KEY(feature_group_id) REFERENCES FeatureGroup(feature_group_id), "
  "FOREIGN KEY(pass_sequence_id) REFERENCES PassSequence(pass_sequence_id), "
  "FOREIGN KEY(parameter_set_id) REFERENCES ParameterSet(parameter_set_id)"
")";

static const char * const create_compilation_feature_table =
"CREATE TABLE CompilationFeature("
  "compilation_id   INTEGER, "
  "pass_instance_id INTEGER, "
  "feature_group_id INTEGER NOT NULL, "
  "UNIQUE(compilation_id, pass_instance_id), "
  "FOREIGN KEY(compilation_id)   REFERENCES Compilation(compilation_id), "
  "FOREIGN KEY(pass_instance_id) REFERENCES PassSequence(pass_instance_id), "
  "FOREIGN KEY(feature_group_id) REFERENCES FeatureGroup(feature_group_id)"
")";

// result table creation strings
static const char * const create_result_table =
"CREATE TABLE Result("
  "compilation_id INTEGER, "
  "metric         INTEGER, "
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


//===------------------- Database feature queries -------------------------===//


static const char * const max_feature_set_id_query =
"SELECT MAX(feature_set_id) from FeatureSet";

static const char * const max_feature_group_id_query =
"SELECT MAX(feature_group_id) from FeatureGroup";

static const char * const insert_feature_type_query =
"INSERT INTO FeatureType VALUES(@feature_type_id, @feature_type)";

static const char * const insert_feature_instance_query =
"INSERT INTO FeatureInstance VALUES(@feature_id, @feature_type_id, @value)";

static const char * const insert_feature_to_set_query =
"INSERT INTO FeatureSet VALUES(@feature_set_id, @feature_id)";

static const char * const insert_feature_set_to_group_query =
"INSERT INTO FeatureGroup VALUES(@feature_group_id, @feature_set_id)";

// debug
static const char * const insert_feature_debug_query =
"INSERT INTO FeatureDebug VALUES(@feature_type_id, @name)";


//===------------------- Database parameter queries -----------------------===//


static const char * const max_parameter_set_id_query =
"SELECT MAX(parameter_set_id) from ParameterSet";

static const char * const insert_parameter_type_query =
"INSERT INTO ParameterType VALUES(@parameter_type_id, @parameter_type)";

static const char * const insert_parameter_instance_query =
"INSERT INTO ParameterInstance "
  "VALUES(@parameter_id, @parameter_type_id, @value)";

static const char * const insert_parameter_to_set_query =
"INSERT INTO ParameterSet VALUES(@parameter_set_id, @parameter_id)";

// debug
static const char * const insert_parameter_debug_query =
"INSERT INTO ParameterDebug VALUES(@parameter_type_id, @name)";


//===--------------------- Pass Sequence queries --------------------------===//


static const char * const max_pass_sequence_query =
"SELECT MAX(pass_sequence_id) from PassSequence";

static const char * const max_pass_pos_query =
"SELECT MAX(pass_pos) from PassSequence WHERE "
  "pass_sequence_id = @pass_sequence_id";

static const char * const insert_pass_to_sequence_query =
"INSERT INTO PassSequence "
  "VALUES(@pass_instance_id, @pass_sequence_id, @pass_pos, @pass_id)";

static const char * const insert_pass_parameter_query =
"INSERT INTO PassParameters VALUES(@pass_instance_id, @parameter_set_id)";




//===-------------------- Database implementation -------------------------===//


Database::Database(std::string db_path,
                   std::map<util::UUID, IMachineLearner*> mls,
                   bool create)
  : m_db(nullptr), m_mls(mls)
{
  int res = sqlite3_open (db_path.c_str(), &m_db);
  
  assert(res == SQLITE_OK && "Unable to load or create mageec database!");
  assert(m_db != nullptr);

  // Enable foreign keys (requires sqlite 3.6.19 or above)
  // If foreign keys are not available the database is still usable, but no
  // foreign key checking will do done.
  bool success = DatabaseQuery(*m_db, "PRAGMA foreign_keys = ON")
    .execute().done();
  assert(success);

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
  assert(res == SQLITE_OK && "Unable to close mageec database!");
}

void Database::init_db(sqlite3 &db)
{
  bool success;

  // Create tables to hold features
  success = DatabaseQuery(db, create_feature_type_table).execute().done();
  assert(success);
  success = DatabaseQuery(db, create_feature_instance_table).execute().done();
  assert(success);
  success = DatabaseQuery(db, create_feature_set_table).execute().done();
  assert(success);
  success = DatabaseQuery(db, create_feature_group_table).execute().done();
  assert(success);

  // Tables to hold parameters
  success = DatabaseQuery(db, create_parameter_type_table).execute().done();
  assert(success);
  success = DatabaseQuery(db, create_parameter_instance_table).execute().done();
  assert(success);
  success = DatabaseQuery(db, create_parameter_set_table).execute().done();
  assert(success);

  // Pass sequences
  success = DatabaseQuery(db, create_pass_sequence_table).execute().done();
  assert(success);
  success = DatabaseQuery(db, create_pass_parameter_table).execute().done();
  assert(success);

  // Compilation
  success = DatabaseQuery(db, create_compilation_table).execute().done();
  assert(success);
  success = DatabaseQuery(db, create_compilation_feature_table)
    .execute().done();
  assert(success);

  // Results
  success = DatabaseQuery(db, create_result_table).execute().done();
  assert(success);

  // Machine learner
  success = DatabaseQuery(db, create_machine_learner_table).execute().done();
  assert(success);

  // Debug tables
  success = DatabaseQuery(db, create_program_unit_debug_table).execute().done();
  assert(success);
  success = DatabaseQuery(db, create_feature_debug_table).execute().done();
  assert(success);
  success = DatabaseQuery(db, create_parameter_debug_table).execute().done();
  assert(success);


  // Manually insert the version into the metadata table
  DatabaseQuery query = DatabaseQueryBuilder(db)
    << "INSERT INTO Metadata VALUES("
      << QueryParamType::kInteger << ", "
      << QueryParamType::kText
    << ")";
  query << static_cast<unsigned>(MetadataField::kDatabaseVersion);
  query << std::string(Database::version);

  success = query.execute().done();
  assert(success);

  // Add other metadata now that the database is in a valid state
  // setMetadata();
}


void Database::validate(void)
{
  assert(isCompatible() && "Cannot validate incompatible database!");
}


bool Database::isCompatible(void)
{
  util::Version version = getVersion();
  return (version == Database::version);
}


util::Version Database::getVersion(void)
{
  std::string str = getMetadata(MetadataField::kDatabaseVersion);

  unsigned major, minor, patch;
  int res = sscanf(str.c_str(), "%u.%u.%u", &major, &minor, &patch);

  assert(res == 3 && "Database has a malformed version number");
  return util::Version(major, minor, patch);
}



std::string Database::getMetadata(MetadataField field)
{
  std::string value;

  DatabaseQuery query = DatabaseQueryBuilder(*m_db)
    << "SELECT value FROM Metadata WHERE field = "
      << QueryParamType::kInteger;
  query << static_cast<int64_t>(field);

  DatabaseQueryIterator res = query.execute();
  if (!res.done()) {
    assert(res.numColumns() == 1);
    value = res.getText(0);

    res.next();
    assert(res.done());
  }
  return value;
}

void Database::setMetadata(MetadataField field, std::string value)
{
  assert(isCompatible());

  DatabaseQuery query = DatabaseQueryBuilder(*m_db)
    << "INSERT INTO Metadata VALUE("
      << QueryParamType::kText << ", "
      << QueryParamType::kInteger
    << ")";

  query << static_cast<int64_t>(field) << value;

  auto res = query.execute();
  assert(res.done());
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
    assert(!res.done());
    assert(res.numColumns() == 2);

    Metric metric = static_cast<Metric>(res.getInteger(0));
    std::vector<uint8_t> ml_blob = res.getBlob(1);

    TrainedML trained_ml(*m_db, ml, metric, ml_blob);
    trained_mls.push_back(trained_ml);
  }

  return trained_mls;
}


//===------------------- Feature extractor interface-----------------------===//



FeatureSetID Database::newFeatureSet(std::vector<FeatureBase*> features)
{
  // SQL statements that are about to be used
  DatabaseQuery start_transaction(*m_db, "BEGIN TRANSACTION");
  DatabaseQuery commit_transaction(*m_db, "COMMIT");

  DatabaseQuery select_feature_set_id(
    *m_db, "SELECT MAX(feature_set_id) FROM FeatureSet");

  DatabaseQuery insert_feature_type = DatabaseQueryBuilder(*m_db)
    << "INSERT INTO FeatureType VALUES ("
      << QueryParamType::kInteger << ", "
      << QueryParamType::kInteger << ")"
    << "WHERE NOT EXISTS ("
      << "SELECT * FROM FeatureType WHERE "
        << "feature_type_id = " << QueryParamType::kInteger << ")";

  DatabaseQuery insert_feature = DatabaseQueryBuilder(*m_db)
    << "INSERT INTO FeatureInstance(feature_type_id, value) VALUES ("
      << QueryParamType::kInteger << ", "
      << QueryParamType::kBlob << ")";

  DatabaseQuery insert_into_feature_set = DatabaseQueryBuilder(*m_db)
    << "INSERT INTO FeatureSet VALUES ("
      << QueryParamType::kInteger << ", "
      << QueryParamType::kInteger << ")";

  DatabaseQuery insert_feature_debug = DatabaseQueryBuilder(*m_db)
    << "INSERT INTO FeatureDebug VALUES ("
      << QueryParamType::kInteger << ", "
      << QueryParamType::kText << ")";

  // Add the entire feature set in a single transaction
  auto res = start_transaction.execute();
  assert(res.done());

  // Get the next feature set id
  res = select_feature_set_id.execute();
  assert(res.numColumns() == 1);

  int64_t feature_set_id = res.getInteger(0) + 1;
  assert(res.next().done());

  // Insert each feature in turn
  for (auto I : features) {
    // clear parameter bindings for all the queries we're about to use
    insert_feature_type.clearAllBindings();
    insert_feature.clearAllBindings();
    insert_into_feature_set.clearAllBindings();
    insert_feature_debug.clearAllBindings();

    // first insert the feature type if it isn't present already
    insert_feature_type
      << static_cast<int64_t>(I->getType())
      << static_cast<int64_t>(I->getFeatureID())
      << static_cast<int64_t>(I->getFeatureID());
    res = insert_feature_type.execute();
    assert(res.done());

    // now insert the feature
    insert_feature << static_cast<int64_t>(I->getFeatureID()) << I->toBlob();
    res = insert_feature.execute();
    assert(res.done());

    // The feature_id is an integer primary key, and so it is equal to the
    // rowid. We use this to add the feature to the feature set
    int64_t feature_id = sqlite3_last_insert_rowid(m_db);

    // Add the feature to the feature set
    insert_into_feature_set << feature_set_id << feature_id;
    res = insert_into_feature_set.execute();
    assert(res.done());

    // Add the feature to the debug table
    insert_feature_debug << feature_id << I->getName();
    res = insert_feature_debug.execute();
    assert(res.done());
  }

  // commit the changes
  res = commit_transaction.execute();
  assert(res.done());

  return static_cast<FeatureSetID>(feature_set_id);
}


FeatureGroupID Database::newFeatureGroup(std::vector<FeatureSetID> features)
{
  (void)features;
  return static_cast<FeatureGroupID>(0);
}

void Database::addFeaturesAfterPass(FeatureGroupID features,
                                    CompilationID compilation,
                                    PassID after_pass)
{
  (void)features;
  (void)compilation;
  (void)after_pass;
}


//===----------------------- Compiler interface ---------------------------===//


CompilationID Database::newCompilation(std::string name, std::string type,
                                       FeatureGroupID features,
                                       ParameterSetID parameters)
{
  (void)name;
  (void)type;
  (void)features;
  (void)parameters;
  return static_cast<CompilationID>(0);
}

ParameterSetID
Database::newParameterSet(std::vector<ParameterBase*> parameters)
{
  (void)parameters;
  return static_cast<ParameterSetID>(0);
}

PassSequenceID Database::newPassSequence(void)
{
  return static_cast<PassSequenceID>(0);
}

PassID Database::addPass(std::string name, PassSequenceID pass_seq,
                         ParameterSetID parameters)
{
  (void)name;
  (void)pass_seq;
  (void)parameters;
  return static_cast<PassID>(0);
}


//===------------------------ Results interface ---------------------------===//


void Database::addResult(CompilationID compilation,
                         Metric metric,
                         uint64_t result)
{
  (void)compilation;
  (void)metric;
  (void)result;
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
