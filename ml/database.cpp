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
#include "mageec.h"
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

  /* Enable foreign keys (requires sqlite 3.6.19 or above)
     (If foreign keys are not available, the database will perform, but no
      foreign key checking will be done) */
  sqlite3_exec(db, "PRAGMA foreign_keys = ON", NULL, NULL, NULL);

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

  // Pass list table
  char qinit[] = "CREATE TABLE IF NOT EXISTS passes (passname TEXT PRIMARY KEY)";
  retval = sqlite3_exec(db, qinit, NULL, NULL, NULL);

  // Pass (executed) order table
  char qinit2[] = "CREATE TABLE IF NOT EXISTS passorder (seq INTEGER NOT NULL, "
                  "pos INTEGER NOT NULL, pass TEXT NOT NULL, "
                  "PRIMARY KEY (seq, pos))";
  retval |= sqlite3_exec(db, qinit2, NULL, NULL, NULL);

  // Program result table
  // FIXME: prog should be hash (similar to seq) for a third feature table
  char qinit3[] = "CREATE TABLE IF NOT EXISTS results (prog TEXT NOT NULL, "
                  "seq INTEGER NOT NULL, time INTEGER, energy INTEGER, "
                  "PRIMARY KEY (prog, seq), "
                  "FOREIGN KEY (seq) REFERENCES passorder(seq))";
  retval |= sqlite3_exec(db, qinit3, NULL, NULL, NULL);

  return retval;
}

/**
 * Loads known passes from the database.
 * @returns empty vector if error, else list of passes as mageec_pass.
 */
std::vector<mageec_pass*> database::get_pass_list()
{
  std::vector<mageec_pass*> passes;
  if (!db)
    return passes;

  sqlite3_stmt *stmt;
  char query[] = "SELECT * FROM passes";
  int retval = sqlite3_prepare_v2 (db, query, -1, &stmt, 0);
  if (retval) {
    return passes;
  }

  /* The current design of the database we always use the first column. Should
     this change in the future, this function should also be changed. */
  while (1)
  {
    retval = sqlite3_step(stmt);
    if(retval == SQLITE_ROW)
    {
      const char *val = (const char*)sqlite3_column_text(stmt,0);
      passes.push_back(new basic_pass(val));
    }
    else if (retval == SQLITE_DONE)
      break;
  }
  return passes;
}

/**
 * Adds a result object to the database
 * @param res Test result to store
 */
void database::add_result(result res)
{
  char *buffer;
  sqlite3_stmt *stmt;

  // Calcualte hash
  std::string hashdata;
  for (int i=0, size=res.passlist.size(); i < size; i++)
    hashdata += res.passlist[i]->name();
  uint64_t hash = hash_data (hashdata.c_str(), hashdata.size());

  /* For SQLite we can only store signed numbers, therefore we sign transform,
     allowing any sorting to still work */
  int64_t shash = hash ^ (uint64_t)0x8000000000000000;

  // Check if hash already exists
  buffer = sqlite3_mprintf ("SELECT seq FROM passorder WHERE seq = %lli "
                            "GROUP BY seq", (long long int)shash);
  if (!buffer)
    return;
  int retval = sqlite3_prepare_v2 (db, buffer, -1, &stmt, 0);
  sqlite3_free (buffer);
  if (retval)
    return;

  // FIXME: Add collision detection
  retval = sqlite3_step(stmt);
  if (retval != SQLITE_DONE)
  {
    // The hash search has not found any result, add passes to database
    for (int i=0, size=res.passlist.size(); i < size; i++)
    {
      buffer = sqlite3_mprintf ("INSERT INTO passorder VALUES (%lli, %i, %Q)",
                                (long long int)shash, i,
                                res.passlist[i]->name().c_str());
      if (!buffer)
        return;
      sqlite3_exec (db, buffer, NULL, NULL, NULL);
      sqlite3_free (buffer);
    }
  }
  
  // Add result
  buffer = sqlite3_mprintf ("INSERT INTO results VALUES (%Q, %lli, 0, %i)",
                            res.progname.c_str(), (long long int)shash,
                            res.metric);
  if (!buffer)
    return;
  sqlite3_exec (db, buffer, NULL, NULL, NULL);
  sqlite3_free (buffer);
}
