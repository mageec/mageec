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

enum class CompilationID    : uint64_t {};
enum class FeatureSetID     : uint64_t {};
enum class FeatureGroupID   : uint64_t {};
enum class PassID           : uint64_t {};
enum class PassSequenceID   : uint64_t {};
enum class ParameterSetID   : uint64_t {};
enum class ParameterGroupID : uint64_t {};

enum class MetadataField : unsigned {
  // The database version always has field number 0
  kDatabaseVersion = 0
};

enum class Metric : unsigned { kCodeSize, kTime, kEnergy };

enum class FeatureType : unsigned { kBool, kInt };

enum class ParameterType : unsigned { kBool, kRange };

enum class DecisionType : unsigned { kNative, kBool, kRange, kPassList };

enum class DecisionRequestType : unsigned {
  kBool,
  kRange,
  kPassList,
  kPassGate
};

struct FeatureDesc {
  unsigned id;
  FeatureType type;

  bool operator<(const FeatureDesc &other) const { return id < other.id; }
};

struct ParameterDesc {
  unsigned id;
  ParameterType type;

  bool operator<(const ParameterDesc &other) const { return id < other.id; }
};

} // end of namespace mageec

#endif // MAGEEC_TYPES_H
