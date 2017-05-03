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

//===----------------------------- Database query -------------------------===//
//
// This file defines a wrapper around the sqlite3 interface, to simply the
// creating and execution of queries.
//
//===----------------------------------------------------------------------===//

#ifndef MAGEEC_SQL_QUERY_H
#define MAGEEC_SQL_QUERY_H

#include "sqlite3.h"

#include <cstddef>
#include <string>
#include <vector>

namespace mageec {

class SQLQuery;

/// \enum SQLType
///
/// Defines the types which can appear in SQL queries
enum class SQLType {
  /// 64-bit signed integer type
  kInteger,
  /// 64-bit floating point value
  kReal,
  /// String
  kText,
  /// Binary blob
  kBlob
};

/// \class SQLQueryBuilder
///
/// \brief This provides an interface to build an SQLQuery piece-by-piece
///
/// During construction, the query is treated as a list of strings, each of
/// which is interleaved by a parameter slot in a parameterized query.
/// Each of these slots has a type, and once the query has been fully
/// constructed values can be bound to the slots provided they have a matching
/// type.
class SQLQueryBuilder {
public:
  SQLQueryBuilder(void) = delete;

  /// \brief Begin building a query for a database
  ///
  /// \param db  Handle to the database this query is targeting
  SQLQueryBuilder(sqlite3 &db);

  /// \brief Retrieve the query from the builder
  ///
  /// This extracts a query from the builder, ready to have parameters bound
  /// to it.
  operator SQLQuery(void);

  /// \brief Append a string to the query
  SQLQueryBuilder &operator<<(std::string str);

  /// \brief Append a slot for a parameter to the query
  ///
  /// The slot will be subsequently populated when a parameter is bound to it
  ///
  /// \param param The type of the parameter which should be bound to the
  /// provided current slot.
  SQLQueryBuilder &operator<<(SQLType param);

private:
  /// Handle to the database which this query targets
  sqlite3 &m_db;

  /// True when the last value appended to the query was a string. When
  /// multiple strings are appended in turn they are internally concatenated
  /// into a single string. This means that parameters and strings will always
  /// be interleaved
  bool m_last_input_was_string;

  /// Substrings which make up the query. Every substring except the last is
  /// followed by the parameter which occurs at the same index. For the
  /// last substring this parameter is optional.
  std::vector<std::string> m_substrs;

  /// Types of parameters which occur after each of the substrings.
  std::vector<SQLType> m_params;
};

/// \class SQLQueryIterator
///
/// Provides the interface to iterate through each row of the results for a
/// query.
///
/// This is tightly coupled to the SQLQuery, as only a single iterator
/// may be executing a query at any point.
class SQLQueryIterator {
public:
  SQLQueryIterator(void) = delete;

  SQLQueryIterator(const SQLQueryIterator &other) = delete;
  SQLQueryIterator(SQLQueryIterator &&other);

  SQLQueryIterator &operator=(const SQLQueryIterator &other) = delete;
  SQLQueryIterator &operator=(SQLQueryIterator &&other);

  /// \brief Iterator destructor
  ~SQLQueryIterator(void);

  /// \brief Construct an iterator to iterate through result for a query
  SQLQueryIterator(sqlite3 &db, SQLQuery &query);

  /// \brief Start the execution of the query from the beginning
  void restart(void);

  /// \brief Step to the next row of results
  SQLQueryIterator next(void);

  /// \brief Assert that the query has completed execution
  void assertDone() const;

  /// \brief Return whether the query has completed execution
  bool done() const;

  /// \brief Return the number of columns in the results table
  int numColumns(void);

  /// \brief Check whether the given value in the results table is null
  /// \param index  Index of the column containing the potential null value
  /// \return True if the value is null
  bool isNull(int index);

  /// \brief Retrieve a blob from the results table
  ///
  /// \param index  Index of the column containing the blob
  ///
  /// \return A vector of bytes holding the blob
  std::vector<uint8_t> getBlob(int index);

  /// \brief Retrieve text from the results table
  ///
  /// \param index  Index of the column containing the text
  ///
  /// \return A string containing the text
  std::string getText(int index);

  /// \brief Retrieve an integer from the results table
  ///
  /// \param index  Index of column containing the text
  ///
  /// \return The integer value
  int64_t getInteger(int index);

  /// \brief Retrieve a real value from the results table
  ///
  /// \param index  Index of column containing the text
  ///
  /// \return The real value
  double getReal(int index);

private:
  /// \brief Validate the state of the iterator
  void validate(void);

  /// Dictates whether this iterator has completed execution
  bool m_done;

  /// Handle to the database for the query
  sqlite3 &m_db;

  /// The query being executed
  SQLQuery *m_query;

  /// Pointer to the underlying sqlite statement
  sqlite3_stmt *m_stmt;
};

/// \class SQLQuery
///
/// \brief Wrapper around an SQL statement to simplify the interface and
/// provide additional error checking.
///
/// A query is made up of a string, as well as zero or more typed slots. A
/// user must bind values of the correct type to all of the slots before
/// executing the query. This ensures the right number and type of values
/// are bound to the sql query before it is executed.
///
/// When the query is executed, an SQLQueryIterator is provided to provide
/// access to each row of the result in turn.
class SQLQuery {
  // The SQLQueryIterator needs to be able to steal ownership of the
  // underlying sqlite statement.
  friend class SQLQueryIterator;

public:
  typedef SQLQueryIterator iterator;

  SQLQuery(void) = delete;

  /// \brief Destructor releases sqlite resources
  ~SQLQuery(void);

  SQLQuery(const SQLQuery &other) = delete;
  SQLQuery(SQLQuery &&other);

  SQLQuery &operator=(const SQLQuery &other) = delete;
  SQLQuery &operator=(SQLQuery &&other) = delete;

  /// \brief Construct a simple query with no parameters
  ///
  /// \param db  The database for the query
  /// \param str  Complete query string
  SQLQuery(sqlite3 &db, std::string str);

  /// \brief Construct a query with parameters
  ///
  /// The query consists of interleaved strings and parameter slots.
  /// Values of the correct type must be bound to all of the parameter slots
  /// in turn before the query can be executed.
  ///
  /// \param db  sqlite database this query is for.
  /// \param substrs  Substrings which make up the query. Every parameter is
  /// preceded by a substring.
  /// \param params  Parameter slots which make up the query.
  SQLQuery(sqlite3 &db, std::vector<std::string> substrs,
           std::vector<SQLType> params);

  /// \brief Bind an integer value to the next available parameter
  SQLQuery &operator<<(int64_t i);

  /// \brief Bind a real value to the next available parameter
  SQLQuery &operator<<(double i);

  /// \brief Bind text to the next available parameter
  SQLQuery &operator<<(std::string str);

  /// \brief Bind a blob to the next available parameter
  SQLQuery &operator<<(const std::vector<uint8_t> blob);

  /// \brief Bind a null to the next available parameter
  SQLQuery &operator<<(std::nullptr_t nullp);

  /// \brief Execute the query, retrieving an iterator which provides access
  /// to the results.
  ///
  /// Only one iterator may be executing the query at a given point.
  iterator exec(void);

  /// \brief Returns true if values are bound to all of the paramters
  bool allBindingsPopulated(void) const;

  /// \brief Clear all bound parameters to make way for new values
  void clearAllBindings(void);

private:
  /// \brief Unlock the statement so that it can be used again
  void unlockQuery(void);

  /// \brief Lock the statement so that it cannot be altered while it is
  /// being executed
  ///
  /// \return The underlying sqlite statement, for which the user needs
  /// exclusive access.
  sqlite3_stmt &lockQuery(void);

  /// Record whether the query is initialized. It may not be initialized
  /// if the query has been moved.
  bool m_is_init;

  /// \brief Check that the query is in a usable state
  void validate(void) const;

  /// Handle to the underlying database connection
  sqlite3 &m_db;

  /// Records whether the query is locked (and therefore should not be used)
  bool m_is_locked;

  /// prepared sqlite statement for the query
  sqlite3_stmt *m_stmt;

  /// The current parameter for a value to be binded to
  std::vector<SQLType>::size_type m_curr_param;

  /// Number of parameters in the query
  const std::vector<SQLType>::size_type m_param_count;

  /// A vector of the parameter types which make up the query.
  const std::vector<SQLType> m_param_types;

  /// Final string for the query. Used when debugging the SQL commands run
  std::string m_sql_query;
};

} // end of namespace mageec

#endif // MAGEEC_SQL_QUERY_H
