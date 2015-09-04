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
// This file contains the definition of the MAGEEC framework. This provides
// the entry point to MAGEEC, through which users can register machine learners
// and retrieve access to a MAGEEC database.
//
//===----------------------------------------------------------------------===//

#ifndef MAGEEC_FRAMEWORK_H
#define MAGEEC_FRAMEWORK_H

#include <map>
#include <string>

#include "mageec/Util.h"


#define MAGEEC_FRAMEWORK_VERSION_MAJOR 2
#define MAGEEC_FRAMEWORK_VERSION_MINOR 0
#define MAGEEC_FRAMEWORK_VERSION_PATCH 0


namespace mageec {


class Database;
class IMachineLearner;


class Framework {
public:
  /// \brief Version of the mageec framework
  static const util::Version version;


  /// \brief Create the framework
  Framework();

  /// \brief Get the version of the mageec framework
  util::Version getVersion(void) const;

  /// \brief Load a machine learner from a provided plugin
  ///
  /// \param ml_path  Path to the machine learner plugin
  /// \return True if the machine learne was successfully loaded, otherwise
  /// false.
  bool loadMachineLearner(std::string ml_path);

  /// \brief Register a machine learner usable by the mageec framework.
  ///
  /// \param ml  The machine learner to register with the framework
  /// \return True if the machine learner was successfully register, otherwise
  /// false.
  bool registerMachineLearner(const IMachineLearner& ml);

  /// \brief Load the database at the provided path
  ///
  /// If the database does not exist, then a new database will be created.
  /// When the database is retrieved, it is provided the interfaces to all of
  /// the machine learners registered with mageec up to this point. Therefore,
  /// the database will only provide access to the machine learners which it
  /// has an interface for.
  ///
  /// \param db_path  Path to the database to be loaded or created
  Database getDatabase(std::string db_path) const;


private:
  /// A map of machine learner interfaces registers with the framework, keyed
  /// based on their uuids.
  std::map<util::UUID, IMachineLearner*> m_mls;
};


} // end of namespace MAGEEC


#endif // MAGEEC_FRAMEWORK_H
