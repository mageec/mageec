/*  MAGEEC Machine Learner Datbase
    Copyright (C) 2013 Embecosm Limited and University of Bristol

    This file is part of MAGEEC.

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

/** @file database.cpp MAGEEC Machine Learner Database */
#include "mageec-ml.h"
#include "mageec-db.h"
#include <cassert>

using namespace mageec;

/**
 * Creates a new database, loading from the SQlite3 database dbname.
 * @param dbname Path to database.
 * @param create Attempt to create the database if it does not exist.
 */
database::database(std::string dbname, bool create)
{
  int loaded;
  if (create)
    loaded = sqlite3_open (dbname.c_str(), &db);
  else
    loaded = sqlite3_open_v2 (dbname.c_str(), &db, SQLITE_OPEN_READONLY, NULL);

  // If we could not open the database, we can not use machine learning
  assert(loaded == 0 && "Unable to load machine learning database.");

  if (create)
    initdb();
}

database::~database()
{
  if (db)
    sqlite3_close (db);
}

/**
 * Given a new database, initilize it with the MAGEEC tables.
 * @return 0 if success, sqlite3 error otherwise.
 */
int database::initdb()
{
  if (!db)
    return 1;

  int retval;

  char qinit[] = "CREATE TABLE IF NOT EXISTS passes (passname TEXT PRIMARY KEY)";
  retval = sqlite3_exec(db, qinit, NULL, NULL, NULL);

  return retval;
}

/**
 * Loads known flags from the database.
 * @returns empty vector if error, else list of flags as mageec_flags.
 */
std::vector<mageec_flag*> database::get_flag_list()
{
  std::vector<mageec_flag*> flags;
  if (!db)
    return flags;

  sqlite3_stmt *stmt;
  char query[] = "SELECT * FROM passes";
  int retval = sqlite3_prepare_v2 (db, query, -1, &stmt, 0);
  if (retval) {
    return flags;
  }

  /* The current design of the database we always use the first column. Should
     this change in the future, this function should also be changed. */
  while (1)
  {
    retval = sqlite3_step(stmt);
    if(retval == SQLITE_ROW)
    {
      const char *val = (const char*)sqlite3_column_text(stmt,0);
      flags.push_back(new basic_flag(val));
    }
    else if (retval == SQLITE_DONE)
      break;
  }
  return flags;
}
