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

//===---------------------------- Attribute Set ---------------------------===//
//
// A set of attributes. There may be no duplicates of a given attribute id
// within a set.
//
//===----------------------------------------------------------------------===//

#ifndef MAGEEC_ATTRIBUTE_SET_H
#define MAGEEC_ATTRIBUTE_SET_H

#include "mageec/Attribute.h"

#include <memory>
#include <ostream>
#include <set>

namespace mageec {

template <typename TypeIDType> class AttributeSet {
private:
  /// \brief Comparator object to sort attributes by comparing their ids
  struct AttributeIDComparator {
    bool
    operator()(std::shared_ptr<AttributeBase<TypeIDType>> const &lhs,
               std::shared_ptr<AttributeBase<TypeIDType>> const &rhs) const {
      return lhs->getID() < rhs->getID();
    }
  };

public:
  /// \brief constant iterator to attributes in the set
  typedef
      typename std::set<std::shared_ptr<AttributeBase<TypeIDType>>,
                        AttributeIDComparator>::const_iterator const_iterator;

  /// \brief Construct a new empty attribute set
  AttributeSet() : m_attributes() {}
  AttributeSet(
      std::initializer_list<std::shared_ptr<AttributeBase<TypeIDType>>> l)
      : m_attributes(l) {}

  /// \brief Add a new attribute to an attribute set
  ///
  /// \param attr The new attribute to be added to the set
  void add(std::shared_ptr<AttributeBase<TypeIDType>> attr) {
    assert(attr != nullptr && "Cannot add null attribute to attribute set");

    const auto &result = m_attributes.emplace(attr);
    assert(result.second && "Added attribute already present in the set!");
  }

  /// \brief Get the number of attributes in the set
  unsigned size() const { return static_cast<unsigned>(m_attributes.size()); }

  /// \brief Get a constant iterator to the beginning of the attribute set
  const_iterator begin() const { return m_attributes.cbegin(); }

  /// \brief Get a constant iterator to the end of the attribute set
  const_iterator end() const { return m_attributes.cend(); }

  /// \brief Print out the held attributes to the provided output stream
  void print(std::ostream &os, std::string prefix) const {
    for (auto attr : m_attributes) {
      os << prefix;
      attr->print(os);
      os << '\n';
    }
  }

private:
  std::set<std::shared_ptr<AttributeBase<TypeIDType>>, AttributeIDComparator>
      m_attributes;
};

// Declare the different types of set that we may have.
typedef AttributeSet<FeatureType> FeatureSet;
typedef AttributeSet<ParameterType> ParameterSet;

} // end of namespace mageec

#endif // MAGEEC_ATTRIBUTE_SET_H
