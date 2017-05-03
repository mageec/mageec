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

//===-------------------------- MAGEEC decision ---------------------------===//
//
// This defines the types of decisions which can be requested of a machine
// learner and the decisions themselves. It also describes the type of
// decision which is expected to result for a provided request.
//
//===----------------------------------------------------------------------===//

#ifndef MAGEEC_DECISION_H
#define MAGEEC_DECISION_H

#include "mageec/Types.h"

#include <cassert>
#include <string>
#include <vector>

namespace mageec {

/// \class DecisionBase
///
/// \brief Base class for arbitrary decision types
///
/// A Decision is comprised of an identifier which identifies the type of
/// the decision which is made, as well as a value of that type.
class DecisionBase {
public:
  DecisionBase() = delete;

  /// \brief Get the type of the decision
  DecisionType getType() const { return m_decision_type; }

protected:
  /// \brief Create a decision with the provided type identifier
  DecisionBase(DecisionType decision_type) : m_decision_type(decision_type) {}

private:
  /// Identifier which specifies the type of the Decision.
  DecisionType m_decision_type;
};

/// \class Decision
///
/// \brief An arbitrary decision type
///
/// A Decision is comprised of an identifier which identifies the type of
/// the decision which is made, as well as a value of that type.
///
/// \tparam decision_type Identifier for the type of the decision
/// \tparam T The corresponding type for the decision
template <DecisionType decision_type, typename T>
class Decision : public DecisionBase {
public:
  typedef T value_type;

  Decision() = delete;

  /// \brief Create a new decision from a given value
  ///
  /// \param value The value of the decision
  explicit Decision(const T &value)
      : DecisionBase(decision_type), m_value(value) {}

  /// \brief Get the value associated with this feature.
  const T &getValue() const { return m_value; }

private:
  /// The value of the made decision
  const T m_value;
};

/// \class DecisionRequestBase
///
/// \brief Base class for arbitrary decision request types
///
/// A DecisionRequest defines a request made to a machine learner, as well
/// as the type of the resultant decision.
class DecisionRequestBase {
public:
  DecisionRequestBase() = delete;

  /// \brief Get the type of the decision request
  DecisionRequestType getType() const { return m_request_type; }

protected:
  /// \brief Create a decision with the provided type
  DecisionRequestBase(DecisionRequestType request_type)
      : m_request_type(request_type) {}

private:
  DecisionRequestType m_request_type;
};

/// \class DecisionRequest
///
/// \brief Specific decision request type
///
/// Associates a given enumeration value for the decision request type with
/// the appropriate type of identifier and decision.
template <DecisionRequestType request_type, DecisionType decision_type,
          typename IdentifierT>
class DecisionRequest : public DecisionRequestBase {
public:
  DecisionRequest() = delete;

  explicit DecisionRequest(const IdentifierT &id)
      : DecisionRequestBase(request_type), m_id(id) {}

  /// \brief Get the identifier associated with this feature
  const IdentifierT &getID() const { return m_id; }

  /// \brief Get the decision type associated with this feature
  DecisionType getDecisionType() const { return decision_type; }

private:
  const IdentifierT m_id;
};

/// A PassSeq is a list of pass name strings
typedef std::vector<std::string> PassSeq;

// Types of decision requests which can be made, and decisions which can result
typedef DecisionRequest<DecisionRequestType::kBool, DecisionType::kBool,
                        unsigned> BoolDecisionRequest;

typedef DecisionRequest<DecisionRequestType::kRange, DecisionType::kRange,
                        unsigned> RangeDecisionRequest;

typedef DecisionRequest<DecisionRequestType::kPassGate, DecisionType::kBool,
                        std::string> PassGateDecisionRequest;

typedef DecisionRequest<DecisionRequestType::kPassSeq, DecisionType::kPassSeq,
                        unsigned> PassSeqDecisionRequest;

/// \class NativeDecision
///
/// \brief The native decision is equivalent to no decision being made.
class NativeDecision : public DecisionBase {
public:
  NativeDecision() : DecisionBase(DecisionType::kNative) {}
};
typedef Decision<DecisionType::kBool, bool> BoolDecision;
typedef Decision<DecisionType::kRange, int64_t> RangeDecision;
typedef Decision<DecisionType::kPassSeq, PassSeq> PassSeqDecision;

} // end of namespace mageec

#endif // MAGEEC_DECISION_H
