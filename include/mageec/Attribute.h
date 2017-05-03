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
#include <iostream>
#include <string>
#include <vector>

namespace mageec {

/// \class AttributeBase
///
/// \brief Base class for arbitrary attributes used by MAGEEC.
///
/// An Attribute is made up of an identifier which uniquely identifies the
/// Attribute to the feature extractor or compiler, an identifier which defines
/// the type of the value held in the attribute, and the value of that type.
///
/// \tparam TypeIDType  The type of the identifier used to identify the
/// type of the attribute.
template <typename TypeIDType> class AttributeBase {
public:
  AttributeBase() = delete;

  // vtable may be emitted multiple times
  virtual ~AttributeBase() {}

  /// \brief Get the identifier which identifies this feature to the
  /// feature extractor or compiler.
  unsigned getID() const { return m_id; }

  /// \brief Get the identifier which defining the type of the attribute
  TypeIDType getType() const { return m_type; }

  /// \brief Get the string name describing the attribute
  std::string getName() const { return m_name; }

  /// \brief Serialize the value of the attribute to a blob of bytes
  ///
  /// \return The value of the Attribute as a serialized blob of bytes
  virtual std::vector<uint8_t> toBlob() const = 0;

  /// \brief Print out an attribute to the provided stream
  virtual void print(std::ostream &os) const = 0;

protected:
  /// \brief Create an Attribute with the specified type identifier,
  /// attribute identifier and name
  ///
  /// \param type  The identifier of the type of the attribute
  /// \param id  The integer identifier uniquely identifier the attribute
  /// \param name  A string name for the attribute
  AttributeBase(TypeIDType type, unsigned id, std::string name)
      : m_type(type), m_id(id), m_name(name) {}

private:
  /// Identifier of the type of the feature
  TypeIDType m_type;

  /// Identifier to uniquely identify the attribute to the feature
  /// extractor or compiler which makes use of the attribute.
  unsigned m_id;

  /// String name of the attribute, for debug purposes.
  std::string m_name;
};

/// \class Attribute
///
/// \brief Defines arbitrary attributes used by MAGEEC.
//
/// An Attribute is made up of an identifier which uniquely identifies the
/// Attribute to the feature extractor or compiler, an identifier which
/// defines the type of the value held in the attribute, and the value of that
/// type.
///
/// \tparam TypeIDType  The type of the identifier used to identify the
/// type of the attribute
/// \tparam type  Identifier which defines the type of the attribute
/// \tparam ValueT  The actual underlying type of the Attribute
template <typename TypeIDType, TypeIDType type, typename ValueT>
class Attribute : public AttributeBase<TypeIDType> {
public:
  typedef ValueT value_type;

  Attribute() = delete;
  ~Attribute() {}

  /// \brief Create an attribute with the specified attribute identifier,
  /// value and name.
  ///
  /// \param id  The integer identifier uniquely identifying the attribute
  /// \param value  The value of the attribute
  /// \param name  A string name for the attribute
  Attribute(unsigned id, const value_type &value, std::string name)
      : AttributeBase<TypeIDType>(type, id, name), m_value(value) {}

  /// \brief Get the value associated with the attribute
  const value_type &getValue() const { return m_value; }

  /// \brief Serialize the value of the attribute to a blob of bytes
  ///
  /// \return The value of the Attribute as a serialized blob of bytes
  std::vector<uint8_t> toBlob(void) const override {
    static_assert(std::is_integral<value_type>::value,
                  "Only integral types handled for now");

    std::vector<uint8_t> blob;
    blob.reserve(sizeof(value_type));

    const uint8_t *value = reinterpret_cast<const uint8_t *>(&m_value);
    for (unsigned i = 0; i < sizeof(value_type); ++i) {
      blob.push_back(value[i]);
    }
    return blob;
  }

  /// \brief Create an attribute of this type given the provided attribute
  /// identifier, binary blob to deserialize, and name
  ///
  /// \param id  The integer identifier uniquely identifying the attribute
  /// \param blob  Binary blob which holds the serialized value of the
  /// attribute, and must be deserialized.
  /// \param name  A string name for the attribute
  ///
  /// \return An initialize Attribute deserialized from the blob
  static std::unique_ptr<Attribute>
  fromBlob(unsigned id, std::vector<uint8_t> blob, std::string name) {
    static_assert(std::is_integral<value_type>::value,
                  "Only integral types handled for now");
    assert(blob.size() == sizeof(value_type));

    const value_type *value = reinterpret_cast<const value_type *>(blob.data());
    return std::unique_ptr<Attribute>(new Attribute(id, *value, name));
  }

  /// \brief Print out an attribute to the provided stream
  void print(std::ostream &os) const override {
    static_assert(std::is_integral<value_type>::value,
                  "Only integral types handled for now");
    os << this->getName() << ": " << m_value;
  }

private:
  /// Value of the feature
  const value_type m_value;
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

// Specialized parameter used to hold a sequence of passes
template <>
class Attribute<ParameterType, ParameterType::kPassSeq,
                std::vector<std::string>>
    : public AttributeBase<ParameterType> {
public:
  typedef std::vector<std::string> value_type;

  Attribute() = delete;
  ~Attribute() {}

  Attribute(unsigned id, const value_type &value, std::string name)
      : AttributeBase<ParameterType>(ParameterType::kPassSeq, id, name),
        m_value(value) {}

  const value_type &getValue() const { return m_value; }

  std::vector<uint8_t> toBlob(void) const override {
    std::vector<uint8_t> blob;

    // Separate each pass in the sequence with a comma
    for (unsigned i = 0; i < m_value.size(); i++) {
      if (i != 0) {
        blob.push_back(',');
      }
      for (auto c : m_value[i]) {
        blob.push_back(static_cast<uint8_t>(c));
      }
    }
    return blob;
  }

  static std::unique_ptr<Attribute>
  fromBlob(unsigned id, std::vector<uint8_t> blob, std::string name) {
    std::vector<std::string> passes;

    if (blob.size()) {
      passes[0] = std::string();

      unsigned num_passes = 0;
      for (auto c : blob) {
        if (c == ',') {
          num_passes++;
          passes[num_passes] = std::string();
        } else {
          passes[num_passes].push_back(static_cast<char>(c));
        }
      }
    }
    return std::unique_ptr<Attribute>(new Attribute(id, passes, name));
  }

  void print(std::ostream &os) const override {
    os << this->getName() << ": ";
    for (unsigned i = 0; i < m_value.size(); i++) {
      std::string str = m_value[i];
      if (i != 0) {
        os << ", ";
      }
      os << str;
    }
  }

private:
  const value_type m_value;
};

typedef Attribute<ParameterType, ParameterType::kPassSeq,
                  std::vector<std::string>> PassSeqParameter;

} // end of namespace mageec

#endif // MAGEEC_FEATURES_H
