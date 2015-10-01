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
#include <string>

#include "mageec/Database.h"
#include "mageec/ML.h"
#include "mageec/Util.h"


namespace mageec {


const util::Version
Framework::version(MAGEEC_VERSION_MAJOR,
                   MAGEEC_VERSION_MINOR,
                   MAGEEC_VERSION_PATCH);


Framework::Framework(void)
  : m_mls()
{
}


util::Version Framework::getVersion(void) const
{
  return Framework::version;
}


bool Framework::loadMachineLearner(std::string path)
{
  (void)path;
  return false;
}


bool Framework::registerMachineLearner(const IMachineLearner &ml)
{
  util::UUID ml_id = ml.getUUID();
  auto res = m_mls.emplace(std::make_pair(ml_id, &ml));
  assert(res.second);

  return true;
}


Database Framework::getDatabase(std::string db_path, bool create) const
{
  return Database(db_path, m_mls, create);
}


} // end of namespace mageec
