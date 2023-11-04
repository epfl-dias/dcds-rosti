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

#ifndef DCDS_TYPES_HPP
#define DCDS_TYPES_HPP

#include <iostream>
#include <map>
#include <set>
#include <utility>
#include <vector>

namespace dcds {

// NOTE: VOID is strictly a return type.
enum class valueType : uint32_t { INT64, INT32, FLOAT, DOUBLE, RECORD_PTR, VOID, BOOL };
enum class VAR_SOURCE_TYPE : uint32_t { DS_ATTRIBUTE, TEMPORARY_VARIABLE, FUNCTION_ARGUMENT };

inline std::ostream &operator<<(std::ostream &os, dcds::valueType ty) {
  // prefix?
  // os << "dcds::valueType::";
  switch (ty) {
    case dcds::valueType::INT64:
      os << "INT64";
      break;
    case dcds::valueType::INT32:
      os << "INT32";
      break;
    case dcds::valueType::FLOAT:
      os << "FLOAT";
      break;
    case dcds::valueType::DOUBLE:
      os << "DOUBLE";
      break;
    case dcds::valueType::RECORD_PTR:
      os << "RECORD_PTR";
      break;
    case dcds::valueType::VOID:
      os << "VOID";
      break;
    case dcds::valueType::BOOL:
      os << "BOOL";
      break;
  }

  return os;
}
inline std::ostream &operator<<(std::ostream &os, dcds::VAR_SOURCE_TYPE ty) {
  os << "dcds::VAR_SOURCE_TYPE::";
  switch (ty) {
    case dcds::VAR_SOURCE_TYPE::DS_ATTRIBUTE:
      os << "DS_ATTRIBUTE";
      break;
    case dcds::VAR_SOURCE_TYPE::TEMPORARY_VARIABLE:
      os << "TEMPORARY_VARIABLE";
      break;
    case dcds::VAR_SOURCE_TYPE::FUNCTION_ARGUMENT:
      os << "FUNCTION_ARGUMENT";
      break;
  }
  return os;
}

}  // namespace dcds

#endif  // DCDS_TYPES_HPP
