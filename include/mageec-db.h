/*  MAGEEC Machine Learning Database Interface
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

/** @file mageec-db.h MAGEEC Machine Learner Database Interface. */
#ifndef __MAGEEC_DB_H__
#define __MAGEEC_DB_H__

#include <string>
#include "sqlite3.h"

namespace mageec
{
/**
 * MAGEEC Machine Learner Database
 */
class database
{
  /**
   * SQlite3 database used by this object.
   */
  sqlite3 *db;
public:
  /**
   * Creates a new database, loading from the SQlite3 database dbname.
   * @param dbname Path to database.
   * @param create Attempt to create the database if it does not exist.
   */
  database (std::string dbname, bool create);
  ~database ();

  /**
   * Given a new database, initilize it with the MAGEEC tables.
   * @return 0 if success, sqlite3 error otherwise.
   */
  int initdb();

  /**
   * Loads known flags from the database.
   * @returns empty vector if error, else list of flags as mageec_flags.
   */
  std::vector<mageec_flag*> get_flag_list();
};

} // End namespace mageec

#endif
