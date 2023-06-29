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

#include <concepts>
#include <deque>
#include <iostream>
#include <unordered_map>
#include <utility>
#include <vector>

#include "dcds/common/types.hpp"

namespace dcds {

class Attribute {
 public:
  Attribute(std::string attribute_name, dcds::valueType attribute_type)
      : name(std::move(attribute_name)), type(attribute_type) {}

  // Transactional Variables.
  // GET/ INSERT/ DELETE/ UPDATE

 public:
  const std::string name;
  const dcds::valueType type;

 private:
};
//
// class Uint64T_Attribute{
//
//
//  const dcds::valueType type = valueType::INTEGER;
//  uint64_t val_;
//};
//
// template <class T>
// class AttributeCRTP{
//
//};
//
// class [[nodiscard]] attribute_t : public AttributeCRTP<attribute_t>{
//
//};
//

//
// class BoolConstant
//    : public TConstant<bool, BoolType, Constant::ConstantType::BOOL,
//                       BoolConstant> {
// public:
//  using TConstant<bool, BoolType, Constant::ConstantType::BOOL,
//                  BoolConstant>::TConstant;
//};

}  // namespace dcds
#endif  // DCDS_ATTRIBUTE_HPP
