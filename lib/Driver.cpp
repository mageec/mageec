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


/// \brief Entry point for the MAGEEC driver
int main(int argc, const char *argv[])
{
  DriverMode mode = DriverMode::kNone;

  util::Option<std::string> db_str;

  bool create_database = false;
  bool seen_create_database = false;

  for (int i = 0; i < argc; ++i) {
    std::string arg = argv[i];
    
    if (arg == "--create-database") {
      if (seen_create_database) {
        MAGEEC_ERR("--create-database argument already seen");
        return -1;
      }
      ++i;
      if (i >= argc) {
        MAGEEC_ERR("No database argument provided");
        return -1;
      }

      db_str = std::string(argv[i]);
      create_database = true;
      seen_create_database = true;
    }
  }

  if (create_database) {
    assert(db_str);
    mode = DriverMode::kCreateDatabase;
  }

  switch (mode) {
  case DriverMode::kCreateDatabase:
    return createDatabase(db_str.get());
  case DriverMode::kNone:
    MAGEEC_WARN("No operation provided, doing nothing");
    return -1;
  }
  assert(0 && "Unreachable");
  return 0;
}
