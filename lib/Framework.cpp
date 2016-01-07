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


//===-------------------------- MAGEEC framework --------------------------===//
//
// Contains the implementation of the MAGEEC framework. This provides the
// entry point to MAGEEC, through which users can register machine learners and
// retrieve access to a MAGEEC database.
//
//===----------------------------------------------------------------------===//

#include "mageec/Framework.h"

#include <cassert>
#include <set>
#include <string>

#include "mageec/Database.h"
#include "mageec/ML.h"
#include "mageec/Util.h"


namespace mageec {


const util::Version
Framework::version(MAGEEC_VERSION_MAJOR,
                   MAGEEC_VERSION_MINOR,
                   MAGEEC_VERSION_PATCH);


Framework::Framework(bool with_debug)
  : m_mls()
{
  if (with_debug) {
    util::setDebug(true);
  }
}

Framework::~Framework(void)
{
  // Delete ml interfaces
  for (auto ml : m_mls) {
    delete ml.second;
  }
}

void Framework::setDebug(bool with_debug) const
{
  util::setDebug(with_debug);
}


util::Version Framework::getVersion(void) const
{
  return Framework::version;
}


util::Option<util::UUID> Framework::loadMachineLearner(std::string path)
{
  (void)path;
  return nullptr;
}


bool
Framework::registerMachineLearner(std::unique_ptr<IMachineLearner> ml)
{
  util::UUID ml_id = ml->getUUID();
  auto res = m_mls.emplace(std::make_pair(ml_id, ml.release()));
  assert(res.second);

  return true;
}


std::unique_ptr<Database>
Framework::getDatabase(std::string db_path, bool create) const
{
  std::unique_ptr<Database> db;
  if (create) {
    db = Database::createDatabase(db_path, m_mls);
  }
  else {
    db = Database::loadDatabase(db_path, m_mls);
  }
  return db;
}


bool Framework::hasMachineLearner(util::UUID uuid) const 
{
  const auto it = m_mls.find(uuid);
  return (it != m_mls.cend());
}


std::set<IMachineLearner *> Framework::getMachineLearners() const
{
  std::set<IMachineLearner *> mls;
  for (auto &it : m_mls) {
    mls.emplace(it.second);
  }
  return mls;
}


} // end of namespace mageec
