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


//===-------------------------- Parameter Set -----------------------------===//
//
// A set of parameters, there may be no duplicates of a given parameter id
// within a set.
//
//===----------------------------------------------------------------------===//

#ifndef MAGEEC_PARAMETER_SET_H
#define MAGEEC_PARAMETER_SET_H

#include <cassert>
#include <memory>
#include <set>
#include <utility>

#include "mageec/Parameter.h"
#include "mageec/Types.h"


namespace mageec {


class ParameterSet {
private:
  /// \brief Function object to sort parameters by comparing their ids
  struct ParameterIDComparator {
    bool operator() (std::shared_ptr<ParameterBase> const &lhs,
                     std::shared_ptr<ParameterBase> const &rhs) const {
      return lhs->getParameterID() < rhs->getParameterID();
    }
  };

public:
  /// \brief constant iterator to parameters in the set
  typedef std::set<std::shared_ptr<ParameterBase>,
                   ParameterIDComparator>::const_iterator const_iterator;


  /// \brief Construct a new empty parameter set
  ParameterSet() : m_parameters() {}
  ParameterSet(std::initializer_list<std::shared_ptr<ParameterBase>> l)
    : m_parameters(l)
  {}

  /// \brief Add a new parameter to a parameter set
  ///
  /// The ParameterSet takes ownership of the provided parameter
  ///
  /// \param param  The parameter to be added to the set
  void addParameter(std::shared_ptr<ParameterBase> param) {
    assert (param != nullptr && "Cannot add null parameter to parameter set");

    const auto& result = m_parameters.emplace(param);
    assert (result.second && "Added parameter already present in the set!");
  }

  /// \brief Get a constant iterator to the beginning of the parameter set
  const_iterator begin() const { return m_parameters.cbegin(); }

  /// \brief Get a constant iterator to the end of the parameter set
  const_iterator end() const { return m_parameters.cend(); }

private:
  std::set<std::shared_ptr<ParameterBase>, ParameterIDComparator> m_parameters;
};


} // end of namespace mageec


#endif // MAGEEC_PARAMETER_SET_H
