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
// Contains the implementation of a wrapper around the sqlite3 interface, to
// simplify the creation and execution of queries.
//
//===----------------------------------------------------------------------===//

#include "mageec/SQLQuery.h"
#include "mageec/Util.h"

#include "sqlite3.h"

#include <cassert>
#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

namespace mageec {

//===----------------------------- Database query -------------------------===//

SQLQuery::~SQLQuery(void) {
  if (m_is_init) {
    validate();
    int res = sqlite3_finalize(m_stmt);

    if (res != SQLITE_OK) {
      MAGEEC_DEBUG("Error destroying database query:\n"
                   << sqlite3_errmsg(&m_db));
    }
    assert(res == SQLITE_OK && "Error destroying query statement!");
  }
}

SQLQuery::SQLQuery(SQLQuery &&other)
    : m_is_init(false), m_db(other.m_db), m_is_locked(false),
      m_stmt(other.m_stmt), m_curr_param(other.m_curr_param),
      m_param_count(other.m_param_count), m_param_types(other.m_param_types),
      m_sql_query(other.m_sql_query) {
  assert(!other.m_is_locked && "Cannot moved a query which is currently "
                               "being executed");
  other.m_is_init = false;
  m_is_init = true;
}

SQLQuery::SQLQuery(sqlite3 &db, std::string str)
    : m_is_init(false), m_db(db), m_is_locked(false), m_stmt(), m_curr_param(0),
      m_param_count(0), m_param_types(), m_sql_query(str) {
  int res = sqlite3_prepare_v2(&m_db, str.c_str(), -1, &m_stmt, NULL);

  if (res != SQLITE_OK) {
    MAGEEC_DEBUG("Error creating database query:\n" << sqlite3_errmsg(&m_db));
  }
  assert(res == SQLITE_OK && m_stmt && "Error creating database query!");
  m_is_init = true;
}

SQLQuery::SQLQuery(sqlite3 &db, std::vector<std::string> substrs,
                   std::vector<SQLType> params)
    : m_is_init(false), m_db(db), m_is_locked(false), m_stmt(), m_curr_param(0),
      m_param_count(params.size()), m_param_types(params), m_sql_query() {
  assert(substrs.size() == (m_param_count + 0) ||
         substrs.size() == (m_param_count + 1));

  std::stringstream query;
  for (unsigned i = 0; i < m_param_count; ++i) {
    query << substrs[i] << "?";
  }
  if (substrs.size() == (m_param_count + 1)) {
    query << substrs[substrs.size() - 1];
  }

  m_sql_query = query.str();
  int res = sqlite3_prepare_v2(&m_db, query.str().c_str(), -1, &m_stmt, NULL);

  if (res != SQLITE_OK) {
    MAGEEC_DEBUG("Error creating database query:\n" << sqlite3_errmsg(&m_db));
  }
  assert(res == SQLITE_OK && m_stmt && "Error creating database query!");
  m_is_init = true;
}

SQLQuery &SQLQuery::operator<<(int64_t i) {
  validate();
  assert(m_param_types[m_curr_param] == SQLType::kInteger);

  // bind positions index from 1
  int res = sqlite3_bind_int64(m_stmt, static_cast<int>(m_curr_param) + 1, i);

  if (res != SQLITE_OK) {
    MAGEEC_DEBUG("Error binding integer value to database query:\n"
                 << sqlite3_errmsg(&m_db));
  }
  assert(res == SQLITE_OK && "Error binding integer value to database query");

  m_curr_param++;
  return *this;
}

SQLQuery &SQLQuery::operator<<(double i) {
  validate();
  assert(m_param_types[m_curr_param] == SQLType::kReal);

  // bind positions index from 1
  int res = sqlite3_bind_double(m_stmt, static_cast<int>(m_curr_param) + 1, i);

  if (res != SQLITE_OK) {
    MAGEEC_DEBUG("Error binding integer value to database query:\n"
                 << sqlite3_errmsg(&m_db));
  }
  assert(res == SQLITE_OK && "Error binding integer value to database query");

  m_curr_param++;
  return *this;
}

SQLQuery &SQLQuery::operator<<(std::string str) {
  validate();
  assert(m_param_types[m_curr_param] == SQLType::kText);

  // bind positions index from 1
  int res = sqlite3_bind_text(m_stmt, static_cast<int>(m_curr_param) + 1,
                              str.c_str(), -1, SQLITE_TRANSIENT);
  if (res != SQLITE_OK) {
    MAGEEC_DEBUG("Error binding text value to database query:\n"
                 << sqlite3_errmsg(&m_db));
  }
  assert(res == SQLITE_OK && "Error binding text value to database query");

  m_curr_param++;
  return *this;
}

SQLQuery &SQLQuery::operator<<(const std::vector<uint8_t> blob) {
  validate();
  assert(m_param_types[m_curr_param] == SQLType::kBlob);

  // bind positions index from 1
  int res =
      sqlite3_bind_blob(m_stmt, static_cast<int>(m_curr_param) + 1, blob.data(),
                        static_cast<int>(blob.size()), SQLITE_TRANSIENT);
  if (res != SQLITE_OK) {
    MAGEEC_DEBUG("Error binding blob value to database query:\n"
                 << sqlite3_errmsg(&m_db));
  }
  assert(res == SQLITE_OK && "Error binding blob value to database query");

  m_curr_param++;
  return *this;
}

SQLQuery &SQLQuery::operator<<(std::nullptr_t nullp) {
  (void)nullp;
  validate();

  // bind positions index from 1
  int res = sqlite3_bind_null(m_stmt, static_cast<int>(m_curr_param) + 1);

  if (res != SQLITE_OK) {
    MAGEEC_DEBUG("Error binding null value to database query:\n"
                 << sqlite3_errmsg(&m_db));
  }
  assert(res == SQLITE_OK && "Error binding null value to database query");

  m_curr_param++;
  return *this;
}

SQLQuery::iterator SQLQuery::exec(void) {
  validate();
  assert(allBindingsPopulated() &&
         "Cannot execute query with unbound parameters");

  if (util::withSQLTrace()) {
    MAGEEC_DEBUG("SQL: " << m_sql_query);
  }

  return iterator(m_db, *this);
}

bool SQLQuery::allBindingsPopulated(void) const {
  return m_curr_param == m_param_types.size();
}

void SQLQuery::clearAllBindings(void) {
  validate();

  int res = sqlite3_clear_bindings(m_stmt);

  if (res != SQLITE_OK) {
    MAGEEC_DEBUG("Failed to clear bindings on database query:\n"
                 << sqlite3_errmsg(&m_db));
  }
  assert(res == SQLITE_OK && "Failed to clear bindings on query!");
  m_curr_param = 0;
}

void SQLQuery::unlockQuery(void) { m_is_locked = false; }

sqlite3_stmt &SQLQuery::lockQuery(void) {
  m_is_locked = true;
  return *m_stmt;
}

void SQLQuery::validate(void) const {
  assert(m_is_init && "Query is uninitialized");

  if (m_param_types.size() != 0) {
    assert(m_curr_param <= m_param_types.size());
  } else {
    assert(m_curr_param == 0);
  }
  assert(m_stmt != nullptr);
  assert(!m_is_locked);
}

//===----------------------- Database query builder -----------------------===//

SQLQueryBuilder::SQLQueryBuilder(sqlite3 &db)
    : m_db(db), m_last_input_was_string(false), m_substrs(), m_params() {}

SQLQueryBuilder::operator SQLQuery(void) {
  return SQLQuery(m_db, m_substrs, m_params);
}

SQLQueryBuilder &SQLQueryBuilder::operator<<(std::string str) {
  if (m_last_input_was_string) {
    assert(m_substrs.size() > 0);
    m_substrs.back() = m_substrs.back() + str;
  } else {
    m_substrs.push_back(str);
  }
  m_last_input_was_string = true;
  return *this;
}

SQLQueryBuilder &SQLQueryBuilder::operator<<(SQLType param) {
  if (m_last_input_was_string) {
    m_params.push_back(param);
  } else {
    m_substrs.push_back(std::string());
    m_params.push_back(param);
  }
  m_last_input_was_string = false;
  return *this;
}

//===----------------------- Database query iterator ----------------------===//

SQLQueryIterator::~SQLQueryIterator(void) {
  if (m_query && m_stmt) {
    validate();

    int res = sqlite3_reset(m_stmt);
    assert(res == SQLITE_OK && "Failed to reset query after execution!");

    // Finished executing the query, so unlock it so that it can be used again
    m_query->unlockQuery();
  } else {
    assert(!m_query && !m_stmt && "Query or statement has been moved");
  }
}

SQLQueryIterator::SQLQueryIterator(SQLQueryIterator &&other)
    : m_done(std::move(other.m_done)), m_db(other.m_db),
      m_query(std::move(other.m_query)), m_stmt(std::move(other.m_stmt)) {
  other.m_query = nullptr;
  other.m_stmt = nullptr;
}

SQLQueryIterator::SQLQueryIterator(sqlite3 &db, SQLQuery &query)
    : m_done(false), m_db(db), m_query(&query), m_stmt(&query.lockQuery()) {
  *this = next();
}

SQLQueryIterator &SQLQueryIterator::operator=(SQLQueryIterator &&other) {
  m_done = std::move(other.m_done);
  m_query = std::move(other.m_query);
  m_stmt = std::move(other.m_stmt);

  other.m_query = nullptr;
  other.m_stmt = nullptr;
  return *this;
}

void SQLQueryIterator::restart(void) {
  validate();

  int res = sqlite3_reset(m_stmt);
  assert(res == SQLITE_OK && "Failed to restart query execution");
  m_done = false;

  *this = next();
}

SQLQueryIterator SQLQueryIterator::next(void) {
  validate();
  assert(!done() && "Cannot continue execution, no more results");

  int res;
  while ((res = sqlite3_step(m_stmt)) == SQLITE_OK)
    ;

  if (res == SQLITE_ROW) {
    m_done = false;
  } else if (res == SQLITE_DONE) {
    m_done = true;
  } else {
    MAGEEC_DEBUG("Error executing query:\n" << sqlite3_errmsg(&m_db));
    assert(0 && "Error executing query");
  }
  return std::move(*this);
}

void SQLQueryIterator::assertDone() const {
  assert(done() && "Query execution incomplete!");
}

bool SQLQueryIterator::done() const { return m_done; }

int SQLQueryIterator::numColumns(void) { return sqlite3_data_count(m_stmt); }

bool SQLQueryIterator::isNull(int index) {
  validate();
  assert(!done());
  assert(index < numColumns());

  return sqlite3_column_type(m_stmt, index) == SQLITE_NULL;
}

std::vector<uint8_t> SQLQueryIterator::getBlob(int index) {
  validate();
  assert(!done());
  assert(index < numColumns());
  assert(sqlite3_column_type(m_stmt, index == SQLITE_BLOB));

  const uint8_t *data =
      static_cast<const uint8_t *>(sqlite3_column_blob(m_stmt, index));
  const int blob_size = sqlite3_column_bytes(m_stmt, index);

  std::vector<uint8_t> blob;
  blob.reserve(static_cast<std::vector<uint8_t>::size_type>(blob_size));

  for (int i = 0; i < blob_size; ++i) {
    blob.push_back(data[i]);
  }
  return blob;
}

std::string SQLQueryIterator::getText(int index) {
  validate();
  assert(!done());
  assert(index < numColumns());
  assert(sqlite3_column_type(m_stmt, index) == SQLITE_TEXT);

  const unsigned char *text = sqlite3_column_text(m_stmt, index);
  return std::string(reinterpret_cast<const char *>(text));
}

int64_t SQLQueryIterator::getInteger(int index) {
  validate();
  assert(!done());
  assert(index < numColumns());
  assert(sqlite3_column_type(m_stmt, index) == SQLITE_INTEGER);

  return sqlite3_column_int64(m_stmt, index);
}

double SQLQueryIterator::getReal(int index) {
  validate();
  assert(!done());
  assert(index < numColumns());
  assert(sqlite3_column_type(m_stmt, index) == SQLITE_FLOAT);

  return sqlite3_column_double(m_stmt, index);
}

void SQLQueryIterator::validate(void) {
  assert(m_stmt && "Iterator has been moved");
  assert(m_query && "Query has been moved");
}

} // end of namespace mageec
