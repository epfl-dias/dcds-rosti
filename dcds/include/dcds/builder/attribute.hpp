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

#include "dcds/common/types.hpp"
#include "dcds/util/logging.hpp"

namespace dcds {

enum ATTRIBUTE_TYPE_CATEGORY {
  SIMPLE,
  COMPOSITE_POINTER, /* enables 1-N relation */
  COMPOSITE          /* 1-1 relation, contained within */
};

class Attribute {
 protected:
  Attribute(std::string _name, dcds::valueType _type, ATTRIBUTE_TYPE_CATEGORY _type_category)
      : name(std::move(_name)), type(_type), type_category(_type_category) {}

 public:
  const std::string name;
  const dcds::valueType type;
  const ATTRIBUTE_TYPE_CATEGORY type_category;
};

class SimpleAttribute : public Attribute {
 public:
  SimpleAttribute(std::string _name, dcds::valueType _type) : SimpleAttribute(std::move(_name), _type, {}) {}

  SimpleAttribute(std::string _name, dcds::valueType _type, std::any default_value)
      : Attribute(std::move(_name), _type, ATTRIBUTE_TYPE_CATEGORY::SIMPLE), defaultValue(std::move(default_value)) {}

  [[nodiscard]] auto getDefaultValue() const {
    assert(defaultValue.has_value());
    return defaultValue;
  }

  [[nodiscard]] bool hasDefaultValue() const { return defaultValue.has_value(); }

 public:
  const std::any defaultValue;
};
}  // namespace dcds
#endif  // DCDS_ATTRIBUTE_HPP
