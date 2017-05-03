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
#include "mageec/Util.h"

#include <memory>
#include <ostream>
#include <set>

namespace mageec {

/// \class AttributeSet
///
/// \brief Class to store a set of Attributes
///
/// \tparam TypeIDType  The type of the identifier used to identify the
/// type of the attribute.
template <typename TypeIDType> class AttributeSet {
private:
  /// \struct AttributeIDComparator
  ///
  /// \brief Comparator object to sort attributes by comparing their identifiers
  struct AttributeIDComparator {
    bool
    operator()(std::shared_ptr<AttributeBase<TypeIDType>> const &lhs,
               std::shared_ptr<AttributeBase<TypeIDType>> const &rhs) const {
      return lhs->getID() < rhs->getID();
    }
  };

public:
  /// Constant iterator to attributes in the set
  typedef
      typename std::set<std::shared_ptr<AttributeBase<TypeIDType>>,
                        AttributeIDComparator>::const_iterator const_iterator;

  /// \brief Construct a new empty AttributeSet
  AttributeSet() : m_attributes() {}

  /// \brief Construct a new AttributeSet based on an initialize list
  AttributeSet(
      std::initializer_list<std::shared_ptr<AttributeBase<TypeIDType>>> l)
      : m_attributes(l) {}

  /// \brief Add a new attribute to the set
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

  /// \brief Produce a 64-bit hash representing the attributes which make up
  /// this set.
  uint64_t hash() const {
    std::vector<uint8_t> blob;
    for (auto I : *this) {
      std::vector<uint8_t> attr_blob = I->toBlob();
      util::write16LE(blob, I->getID());
      blob.insert(blob.end(), attr_blob.begin(), attr_blob.end());
    }
    return util::crc64(blob.data(), static_cast<unsigned>(blob.size()));
  }

  bool operator<(const AttributeSet &other) const {
    return compare(other) < 0;
  }
  bool operator==(const AttributeSet &other) const {
    return compare(other) == 0;
  }
  bool operator!=(const AttributeSet &other) const {
    return compare(other) != 0;
  }

  /// \brief Print out the held attributes to the provided output stream
  void print(std::ostream &os) const {
    for (auto attr : m_attributes) {
      attr->print(os);
      os << '\n';
    }
  }

  /// \brief Dump out the held attributes to stdout
  void dump() const {
    print(util::out());
  }

private:
  /// Set of attributes. Attributes are ordered based on the ordering of
  /// identifiers, so only one attribute with a given identifier may
  /// appear in the set at once.
  std::set<std::shared_ptr<AttributeBase<TypeIDType>>,
           AttributeIDComparator> m_attributes;

  /// \brief Compare the current AttributeSet against another
  ///
  /// First, the size of the sets are compared, then the identifiers of
  /// each consequtive Attribute is compared, then the Attribute value
  /// itself is compared.
  ///
  /// \return -1 if this < other, 0 if this == other, 1 if this > other
  int compare(const AttributeSet &other) const {
    if (size() < other.size())
      return -1;
    else if (size() > other.size())
      return 1;

    // Before checking the values of attribute, check the attribute identifiers
    auto lhs_iter = begin();
    auto rhs_iter = other.begin();
    while (lhs_iter != end()) {
      unsigned lhs_id = (*lhs_iter)->getID();
      unsigned rhs_id = (*rhs_iter)->getID();
      if (lhs_id < rhs_id)
        return -1;
      else if (lhs_id > rhs_id)
        return 1;

      ++lhs_iter;
      ++rhs_iter;
    }

    // Now check attributes one by one. For now, we do the expensive thing and
    // check the serialized blobs
    lhs_iter = begin();
    rhs_iter = other.begin();
    while (lhs_iter != end()) {
      std::vector<uint8_t> lhs_blob = (*lhs_iter)->toBlob();
      std::vector<uint8_t> rhs_blob = (*rhs_iter)->toBlob();
      if (lhs_blob.size() < rhs_blob.size())
        return -1;
      else if (lhs_blob.size() > rhs_blob.size())
        return 1;

      if (lhs_blob < rhs_blob)
        return -1;
      else if (lhs_blob > rhs_blob)
        return 1;

      ++lhs_iter;
      ++rhs_iter;
    }
    return 0;
  }
};

// Declare the different types of set that we may have.
typedef AttributeSet<FeatureType> FeatureSet;
typedef AttributeSet<ParameterType> ParameterSet;

} // end of namespace mageec

#endif // MAGEEC_ATTRIBUTE_SET_H
