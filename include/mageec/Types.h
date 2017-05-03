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

#include <cstdint>

namespace mageec {

/// Integer type of mageec type enumerations.
typedef uint16_t TypeID;

/// Integer type of mageec identifiers.
typedef uint64_t ID;

enum class CompilationID    : ID {};
enum class FeatureSetID     : ID {};
enum class ParameterSetID   : ID {};
enum class ParameterGroupID : ID {};

/// \enum MetadataField
///
/// \brief Unique identifiers for entries in the metadata table in the database
enum class MetadataField : unsigned {
  /// Metadata which identifies the version of the database.
  // The database version always has field number 0
  kDatabaseVersion = 0
};

/// \enum FeatureType
///
/// \brief Types which features extracted by a feature extractor can take, and
/// which machine learners can use.
///
/// These are the types of features which feature extractors are allowed to
/// generated, and which machine learners are allowed to consume. These
/// types are defined by MAGEEC so that it is able to provide a common
/// interface to both feature extractors and machine learners, and so that
/// it is able to serialize and store features in the database.
enum class FeatureType : TypeID {
  /// A feature which is either true or false
  kBool,
  /// A feature which is a signed 64-bit value
  kInt
};

/// \enum FeatureClass
///
/// \brief Specify the class of a feature
///
/// The class of a feature defines the type of program unit which the
/// feature is appropriate for. For example, it doesn't make sense to
/// generate a 'number of arguments' function feature for a module.
///
/// During training each 'class' of features is trained independently,
/// generating a separate training blob.
enum class FeatureClass : TypeID {
  kFIRST_FEATURE_CLASS,

  /// Features for a module program unit
  kModule = kFIRST_FEATURE_CLASS,
  /// Features for a function program unit
  kFunction,

  kLAST_FEATURE_CLASS = kFunction,
};

/// \enum ParameterType
///
/// \brief Types which parameters controlled by a compiler can take, and
/// which machine learners can use.
///
/// These are the types of parameters which compilers are allowed to
/// control, and which machine learners are allowed to make decisions about.
/// These types are defined by MAGEEC so that it is able to provide a common
/// interface to both the compiler and machine learners, and so that it is
/// able to serialize and store parameters in the database.
enum class ParameterType : TypeID {
  /// A parameter which is either enabled or disabled
  kBool,
  /// A parameter which can take any integer value in a signed 64-bit integer
  /// range.
  kRange,
  /// A parameter which describes the sequence of passes executed by a compiler.
  kPassSeq
};

/// \enum DecisionType
///
/// \brief Types of decision which machine learners can make
///
/// These are the decisions that the machine learner can make and can
/// therefore be returned to a user of the machine learner when it makes
/// a DecisionRequest.
enum class DecisionType : TypeID {
  /// This is returned when the machine learner cannot, or does not want to make
  /// a decision.
  kNative,
  /// A true or false decision
  kBool,
  /// A decision which takes any value in a signed 64-bit integer range
  kRange,
  /// A decision for a full set of passes to be run
  kPassSeq
};

/// \enum DecisionRequestType
///
/// \brief Types of decision which a user can request of a machine learner
///
/// These are the decisions which a user can make of a machine learner.
enum class DecisionRequestType : TypeID {
  /// A request for a true or false decision to be made
  kBool,
  /// A request for a decision to be made over a value in a signed 64-bit
  /// integer range
  kRange,
  /// A request for a decision to be made a full pass sequence
  kPassSeq,
  /// A request for a decision to be made about whether to run or not run a
  /// single pass
  kPassGate
};

/// \struct FeatureDesc
///
/// \brief An integer identifier of a Feature combined with an identifier for
/// its type
struct FeatureDesc {
  unsigned id;
  FeatureType type;

  bool operator<(const FeatureDesc &other) const { return id < other.id; }
};

/// \struct ParameterDesc
///
/// \brief An integer identifier of a Parameter combined with an identifier for
/// its type
struct ParameterDesc {
  unsigned id;
  ParameterType type;

  bool operator<(const ParameterDesc &other) const { return id < other.id; }
};

} // end of namespace mageec

#endif // MAGEEC_TYPES_H
