/*
                              Copyright (c) 2023.
          Data Intensive Applications and Systems Laboratory (DIAS)
                  École Polytechnique Fédérale de Lausanne

                              All Rights Reserved.

      Permission to use, copy, modify and distribute this software and
      its documentation is hereby granted, provided that both the
      copyright notice and this permission notice appear in all copies of
      the software, derivative works or modified versions, and any
      portions thereof, and that both notices appear in supporting
      documentation.

      This code is distributed in the hope that it will be useful, but
      WITHOUT ANY WARRANTY; without even the implied warranty of
      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. THE AUTHORS
      DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER
      RESULTING FROM THE USE OF THIS SOFTWARE.
 */

#ifndef DCDS_ATTRIBUTE_HPP
#define DCDS_ATTRIBUTE_HPP

#include <any>
#include <concepts>
#include <deque>
#include <iostream>
#include <unordered_map>
#include <utility>
#include <vector>

#include "dcds/common/common.hpp"
#include "dcds/common/types.hpp"
#include "dcds/util/logging.hpp"

namespace dcds {

class Builder;

enum class ATTRIBUTE_TYPE_CATEGORY {
  PRIMITIVE,
  ARRAY_LIST,
  COMPOSITE_POINTER, /* enables 1-N relation */
  COMPOSITE          /* 1-1 relation, contained within */
};

inline std::ostream& operator<<(std::ostream& os, dcds::ATTRIBUTE_TYPE_CATEGORY ty) {
  os << "ATTRIBUTE_TYPE_CATEGORY::";
  switch (ty) {
    case dcds::ATTRIBUTE_TYPE_CATEGORY::PRIMITIVE:
      os << "PRIMITIVE";
      break;
    case dcds::ATTRIBUTE_TYPE_CATEGORY::ARRAY_LIST:
      os << "ARRAY_LIST";
      break;
    case ATTRIBUTE_TYPE_CATEGORY::COMPOSITE_POINTER:
      os << "COMPOSITE_POINTER";
      break;
    case ATTRIBUTE_TYPE_CATEGORY::COMPOSITE:
      os << "COMPOSITE";
      break;
  }
  return os;
}

class Attribute {
 protected:
  explicit Attribute(std::string _name, dcds::valueType _type, ATTRIBUTE_TYPE_CATEGORY _type_category)
      : Attribute(std::move(_name), _type, _type_category, false, false) {}

  explicit Attribute(std::string _name, dcds::valueType _type, ATTRIBUTE_TYPE_CATEGORY _type_category,
                     bool _is_compile_time_constant, bool _is_runtime_constant)
      : name(std::move(_name)),
        type(_type),
        type_category(_type_category),
        is_compile_time_constant(_is_compile_time_constant),
        is_runtime_constant(_is_runtime_constant) {}

 public:
  const std::string name;
  const dcds::valueType type;
  const ATTRIBUTE_TYPE_CATEGORY type_category;

  bool is_compile_time_constant;
  bool is_runtime_constant;
};

class SimpleAttribute : public Attribute {
 public:
  SimpleAttribute(std::string _name, dcds::valueType _type) : SimpleAttribute(std::move(_name), _type, {}) {}

  SimpleAttribute(std::string _name, dcds::valueType _type, std::any default_value)
      : Attribute(std::move(_name), _type, ATTRIBUTE_TYPE_CATEGORY::PRIMITIVE),
        defaultValue(std::move(default_value)) {}

  [[nodiscard]] auto getDefaultValue() const {
    assert(defaultValue.has_value());
    return defaultValue;
  }

  [[nodiscard]] bool hasDefaultValue() const { return defaultValue.has_value(); }

 public:
  const std::any defaultValue;
};

class CompositeAttributePointer : public Attribute {
 public:
  explicit CompositeAttributePointer(std::string _name, std::string _composite_type_name)
      : Attribute(std::move(_name), dcds::valueType::RECORD_PTR, ATTRIBUTE_TYPE_CATEGORY::COMPOSITE_POINTER),
        composite_type_name(std::move(_composite_type_name)) {}

  const std::string composite_type_name;
};

// make sure, nobody tries to change this pointer. it should be const!
// maybe is_const attribute in the Attribute class make sure nobody updates it.
class AttributeList : public Attribute {
 protected:
  AttributeList(std::string _name, const std::shared_ptr<Builder>& _type, size_t _len = 0)
      : Attribute(std::move(_name), valueType::RECORD_PTR, ATTRIBUTE_TYPE_CATEGORY::ARRAY_LIST, false, true),
        is_primitive_type(false),
        composite_type(_type),
        is_fixed_size(_len != 0),
        len(_len) {}

  AttributeList(std::string _name, std::shared_ptr<SimpleAttribute> _type, size_t _len = 0)
      : Attribute(std::move(_name), valueType::RECORD_PTR, ATTRIBUTE_TYPE_CATEGORY::ARRAY_LIST, false, true),
        is_primitive_type(true),
        simple_type(std::move(_type)),
        is_fixed_size(_len != 0),
        len(_len) {}

 public:
  const bool is_primitive_type;
  const std::shared_ptr<Builder> composite_type{};
  const std::shared_ptr<SimpleAttribute> simple_type{};

  const bool is_fixed_size;
  const size_t len;
};

// fixed-length, can be integer-indexed.
class AttributeArray : public AttributeList {
 public:
  AttributeArray(std::string _name, const std::shared_ptr<Builder>& _type, size_t _len)
      : AttributeList(std::move(_name), _type, _len) {}

  AttributeArray(const std::string& _name, dcds::valueType _type, size_t _len, std::any default_value = {})
      : AttributeList(_name, std::make_shared<SimpleAttribute>(_name, _type, std::move(default_value)), _len) {}
};

// variable-length, index. cannot be integer-indexed.
class AttributeIndexedList : public AttributeList {
 public:
  // later: have type of index also, maybe we can figure that out from workload.
  AttributeIndexedList(std::string _name, const std::shared_ptr<Builder>& _type, std::string key_attribute_name)
      : AttributeList(std::move(_name), _type, 0), key_attribute(std::move(key_attribute_name)) {}

  const std::string key_attribute;

  // this needs its own functions also.
  // std::vector<std::string> intrinsics{"contains", "get", "insert", "remove"};
};

}  // namespace dcds

#endif  // DCDS_ATTRIBUTE_HPP
