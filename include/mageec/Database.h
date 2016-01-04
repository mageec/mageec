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

#ifndef MAGEEC_DATABASE_H
#define MAGEEC_DATABASE_H

#include <map>
#include <string>
#include <vector>

#include "mageec/DatabaseQuery.h"
#include "mageec/FeatureSet.h"
#include "mageec/ParameterSet.h"
#include "mageec/Result.h"
#include "mageec/TrainedML.h"
#include "mageec/Types.h"
#include "mageec/Util.h"

#include "sqlite3.h"


#define MAGEEC_DATABASE_VERSION_MAJOR 1
#define MAGEEC_DATABASE_VERSION_MINOR 0
#define MAGEEC_DATABASE_VERSION_PATCH 0


namespace mageec {


class IMachineLearner;
class FeatureBase;
class ParameterBase;


class Database {
private:
  /// \brief Version of the database interface. A newly created database will
  /// have this version number.
  static const util::Version version;


public:
  /// \brief Load a database from the provided path
  ///
  /// If the database does not exist then this will fail
  ///
  /// \param db_path  Path to the database to be loaded.
  /// \param mls  Map of the machine learner interfaces available to the
  /// database
  /// \return The database if it could be loaded, nullptr otherwise.
  static std::unique_ptr<Database>
  loadDatabase(std::string db_path,
               std::map<util::UUID, IMachineLearner*> mls);

  /// \brief Create a database from the provided path
  ///
  /// If the database already exists then this will fail
  ///
  /// \param db_path  Path to the database to be created.
  /// \param mls  Map of the machine learner interfaces available to the
  /// database
  /// \return The database if it could be created, nullptr otherwise.
  static std::unique_ptr<Database>
  createDatabase(std::string db_path,
                 std::map<util::UUID, IMachineLearner*> mls);

  /// \brief Load or create a database from the provided path
  ///
  /// If the database does not already exists, then a new database will be
  /// created. If the database could not be loaded or created, then
  /// this will fail.
  ///
  /// \param db_path  Path to the database to be created or loaded
  /// \param mls  Map of the machine learner interfaces available to the
  /// database
  /// \return The database if it could be created or loaded, nullptr
  /// otherwise.
  static std::unique_ptr<Database>
  getDatabase(std::string db_path,
              std::map<util::UUID, IMachineLearner*> mls);


private:
  /// \brief Construct a database from the provided database path.
  ///
  /// If the database does not exist then an empty database is created and
  /// used.
  ///
  /// \param db  Handle to the sqlite database
  /// \param mls  Map of the machine learner interfaces available to the
  /// database.
  /// \param create  True if the database should be constructed.
  Database(sqlite3 &db,
           std::map<util::UUID, IMachineLearner*> mls,
           bool create);


public:
  Database(void) = delete;
  ~Database(void);

  Database(const Database &other) = delete;
  Database(Database &&other) = default;

  Database &operator=(const Database &other) = delete;
  Database &operator=(Database &&other) = default;


public:
  /// \brief Get the version of the database
  util::Version getVersion(void);

  /// \brief Check whether the version of the database is compatible with the
  /// provided code.
  ///
  /// \return True if the database is compatible
  bool isCompatible(void);

  /// \brief Get all of the trained machine learners in the database
  /// \return All machine learners in the database which are trained.
  std::vector<TrainedML> getTrainedMachineLearners(void);


private:
  /// \brief Get a metadata field of the database
  ///
  /// \param field  The field to retrieve from the database, which may not
  /// exist
  /// \return The string value of the retrieved field. This may be the empty
  /// string if the field does not exist.
  std::string getMetadata(MetadataField field);

  /// \brief Set a metadata field of the database
  ///
  /// \param field  The field to set in the database.
  /// \param value  The string value which that field should take
  void setMetadata(MetadataField field, std::string value);

public:


//===------------------- Feature extractor interface-----------------------===//


  /// \brief Add a new set of features to the database
  ///
  /// \param features  The features to be added, there should be no features
  /// with the same identifier in this set.
  ///
  /// \return The identifier of the new feature set in the database
  FeatureSetID newFeatureSet(FeatureSet features);

  /// \brief Create a new group of features from a number of feature sets
  ///
  /// \param features  The feature sets which form this feature group, there
  /// should be no features with the same identifier throughout the groups.
  ///
  /// \return The identifier of the feature group in the database
  FeatureGroupID newFeatureGroup(std::vector<FeatureSetID> features);

  /// \brief Associate features with a specific pass in the compilation of a
  /// program unit.
  /// 
  /// \param features  The feature group to associate with this compilation and
  /// pass
  /// \param compilation  The identifier of a compilation of a program unit
  /// \param after_pass  Identifier of the pass which the features are
  /// associated with. This must be a pass which is in the pass sequence of the
  /// compilation.
  void addFeaturesAfterPass(FeatureGroupID features,
                            CompilationID compilation,
                            PassID after_pass);


//===----------------------- Compiler interface ---------------------------===//


  /// \brief Create a new compilation of a program unit
  ///
  /// \param name  The name of the program unit (debug)
  /// \param type  The type of the program unit (debug)
  /// \param features  The group of features for the initial program unit
  /// \param parameters  Set of parameters which apply to the overall
  /// compilation
  /// \param pass_sequence  Optional sequence of passes which were applied to
  /// this compilation
  /// \param parent  Optional parent of this compilation. For example the
  /// encapsulating module for a function. (debug)
  ///
  /// \return An identifier for the compilation of the program unit
  CompilationID newCompilation(std::string name, std::string type,
                               FeatureGroupID features,
                               ParameterSetID parameters,
                               util::Option<PassSequenceID> pass_sequence,
                               util::Option<CompilationID> parent);

  /// \brief Create a new set of parameters
  ///
  /// \param parameters  The parameters composing this set, there should be
  /// no parameters with the same identifier
  ///
  /// \return The identifier of the new parameter set in the database
  ParameterSetID newParameterSet(ParameterSet parameters);

  /// \brief Create a new empty pass sequence
  /// \return The identifier of the new pass sequence
  PassSequenceID newPassSequence(void);

  /// \brief Add a new pass to a pass sequence
  ///
  /// Passes should be added in the order in which they are run
  ///
  /// \param name  The name used to identify the pass
  /// \param pass_seq  The pass sequence the pass belongs to
  /// \param parameters  An optional set of parameters which are appropriate
  /// for this pass
  ///
  /// \return The identifier of the pass instance
  PassID addPass(std::string name, PassSequenceID pass_seq,
                 util::Option<ParameterSetID> parameters = nullptr);


//===------------------------ Results interface ---------------------------===//


  /// \brief Add a single result entry to the database for a previously
  /// established compilation.
  ///
  /// \param compilation  The compilation of the program unit which this result
  /// corresponds to.
  /// \param metric  The metric which the result measured
  /// \param result  The value of the metric for this compilation
  void addResult(CompilationID compilation, Metric metric, uint64_t result);


//===----------------------- Training interface ---------------------------===//


  /// \brief Train the provided machine learner using the results data in the
  /// database for the target metric.
  ///
  /// \param ml  Identifier of the machine learner in the database to train,
  /// this must be the UUID of a machine learner for which we have a
  /// corresponding interface.
  /// \param metric  The metric to train against.
  void trainMachineLearner(util::UUID ml, Metric metric);

  // TODO: Incremental training interface

private:
  /// Handle to the underlying sqlite3 database
  sqlite3 *m_db;

  /// Mapping of machine learner uuids to machine learners
  std::map<util::UUID, IMachineLearner*> m_mls;

  /// \brief Initialize a new empty database
  ///
  /// The provided handle should point at a valid, empty sqlite3 database
  ///
  /// \param db  The database to be initialized
  static void init_db(sqlite3 &db);

  /// \brief Validate the contents of the database
  ///
  /// This is used to check that a database is valid and well formed, and to
  /// check constraints which cannot be handled by sqlite
  void validate(void);
};


/// \class ResultIterator
///
/// Provides an interface to retrieve individual results data from the database
/// in sequence.
class ResultIterator {
public:
  /// \brief Constructor an iterator to iterate through results in the database
  ///
  /// \param db  Database to retrieve results from
  /// \param metric  Metric of the results
  ResultIterator(sqlite3 &db, Metric metric);

  ResultIterator() = delete;
  ResultIterator(const ResultIterator &other) = delete;
  ResultIterator(ResultIterator &&other);

  ResultIterator& operator=(ResultIterator &&other);

  util::Option<Result> operator*();
  ResultIterator next();

private:
  sqlite3 *m_db;
  Metric   m_metric;
  std::unique_ptr<DatabaseQuery> m_query;
  std::unique_ptr<DatabaseQueryIterator> m_result_iter;
};


} // end of namespace mageec


#endif // MAGEEC_DATABASE_H
