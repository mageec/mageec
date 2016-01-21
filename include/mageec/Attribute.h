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

//===------------------------- Program Attributes -------------------------===//
//
// This contains the definition of the various types of MAGEEC attributes.
// Attributes quantify some aspect of a program unit or its compilation,
// and can be broken into two categories; Parameters, which define the
// compiler configuration when building a program; and Features, which
// quantify some measureable property of that program.
//
// Features are extracted by the feature extract and used as training data
// for the machine learner. Parameters are parts of the compiler configuration
// which the machine learner can target for tuning.
//
//===----------------------------------------------------------------------===//

#ifndef MAGEEC_ATTRIBUTE_H
#define MAGEEC_ATTRIBUTE_H

#include "Types.h"

#include <cassert>
#include <memory>
#include <iosfwd>
#include <string>
#include <vector>

namespace mageec {

/// \class AttributeBase
///
/// \brief Base class for arbitrary attribute types
template <typename TypeIDType> class AttributeBase {
public:
  AttributeBase() = delete;

  // vtable may be emitted multiple times
  virtual ~AttributeBase() {}

  /// \brief Get the feature extractor or compiler specific identifier
  /// associated with an attribute.
  unsigned getID() const { return m_id; }

  /// \brief Get the type of the attribute.
  TypeIDType getType() const { return m_type; }

  /// \brief Get the string identifier associated with an attribute
  std::string getName() const { return m_name; }

  /// \brief Get the value of an attribute as a blob of bytes
  virtual std::vector<uint8_t> toBlob() const = 0;

  /// \brief Print out an attribute to the provided stream
  virtual void print(std::ostream &os) const = 0;

protected:
  /// \brief Create an Attribute with the provided type and identifier
  AttributeBase(TypeIDType type, unsigned id, std::string name)
      : m_type(type), m_id(id), m_name(name) {}

private:
  /// The MAGEEC type of the feature
  TypeIDType m_type;

  /// Feature extractor or compiler dependent identifier of the attribute
  unsigned m_id;

  /// String identifier of the specific attribute, used for debug purposes
  std::string m_name;
};

/// \class Attribute
///
/// \brief Represents a specific type of attribute
///
/// Associates a given enumeration value for the attribute type with the
/// appropriate type of value.
template <typename TypeIDType, TypeIDType type, typename ValueT>
class Attribute : public AttributeBase<TypeIDType> {
public:
  typedef ValueT value_type;

  Attribute() = delete;
  ~Attribute() {}

  Attribute(unsigned id, const ValueT &value, std::string name)
      : AttributeBase<TypeIDType>(type, id, name), m_value(value) {}

  /// \brief Get the value associated with this attribute.
  const ValueT &getValue() const { return m_value; }

  /// \brief Convert the held attribute to a binary blob
  std::vector<uint8_t> toBlob(void) const override {
    static_assert(std::is_integral<ValueT>::value,
                  "Only integral types handled for now");

    std::vector<uint8_t> blob;
    blob.reserve(sizeof(ValueT));

    const uint8_t *value = reinterpret_cast<const uint8_t *>(&m_value);
    for (unsigned i = 0; i < sizeof(ValueT); ++i) {
      blob.push_back(value[i]);
    }
    return blob;
  }

  /// \brief Create an attribute of this type given the provided id, blob and
  /// name
  static std::unique_ptr<Attribute>
  fromBlob(unsigned id, std::vector<uint8_t> blob, std::string name) {
    static_assert(std::is_integral<ValueT>::value,
                  "Only integral types handled for now");
    assert(blob.size() == sizeof(ValueT));

    const ValueT *value = reinterpret_cast<const ValueT *>(blob.data());
    return std::unique_ptr<Attribute>(new Attribute(id, *value, name));
  }

  /// \brief Print out an attribute type to the provided stream
  void print(std::ostream &os) const override {
    static_assert(std::is_integral<ValueT>::value,
                  "Only integral types handled for now");
    os << this->getName() << ": " << m_value;
  }

private:
  const ValueT m_value;
};

// Types of features supported by MAGEEC
template <FeatureType type, typename ValueT>
using Feature = Attribute<FeatureType, type, ValueT>;

typedef AttributeBase<FeatureType> FeatureBase;

typedef Feature<FeatureType::kBool, bool>    BoolFeature;
typedef Feature<FeatureType::kInt,  int64_t> IntFeature;

// Types of parameters supported by MAGEEC
template <ParameterType type, typename ValueT>
using Parameter = Attribute<ParameterType, type, ValueT>;

typedef AttributeBase<ParameterType> ParameterBase;

typedef Parameter<ParameterType::kBool, bool> BoolParameter;
typedef Parameter<ParameterType::kRange, int64_t> RangeParameter;

} // end of namespace mageec

#endif // MAGEEC_FEATURES_H