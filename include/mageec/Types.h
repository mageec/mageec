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


//===--------------------------- MAGEEC types -----------------------------===//
//
// This file defines various enumeration types which are used by MAGEEC, and
// users of the MAGEEC library.
//
//===----------------------------------------------------------------------===//

#ifndef MAGEEC_TYPES_H
#define MAGEEC_TYPES_H


namespace mageec {


enum class CompilationID    : unsigned {};
enum class FeatureSetID     : unsigned {};
enum class FeatureGroupID   : unsigned {};
enum class PassID           : unsigned {};
enum class PassSequenceID   : unsigned {};
enum class ParameterSetID   : unsigned {};
enum class ParameterGroupID : unsigned {};


enum class MetadataField : unsigned {
  // The database version always has field number 0
  kDatabaseVersion = 0
};

enum class Metric : unsigned {
  kCodeSize,
  kTime,
  kEnergy
};

enum class FeatureType : unsigned {
  kBool,
  kInt
};

enum class ParameterType : unsigned {
  kBool,
  kRange
};

enum class DecisionType : unsigned {
  kNative,
  kBool,
  kRange,
  kPassList
};

enum class DecisionRequestType : unsigned {
  kBool,
  kRange,
  kPassList,
  kPassGate
};


struct FeatureDesc {
  unsigned    id;
  FeatureType type;
};


struct ParameterDesc {
  unsigned      id;
  ParameterType type;
};


} // end of namespace mageec


#endif // MAGEEC_TYPES_H
