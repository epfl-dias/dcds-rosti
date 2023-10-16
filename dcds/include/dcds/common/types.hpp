//
// Created by Aunn Raza on 18.04.23.
//

#ifndef DCDS_TYPES_HPP
#define DCDS_TYPES_HPP

#include <iostream>
#include <utility>
#include <vector>

namespace dcds {

// NOTE: VOID is strictly a return type.
enum valueType { INTEGER, FLOAT, RECORD_ID, CHAR, RECORD_PTR, VOID, BOOL };

enum VAR_SOURCE_TYPE { DS_ATTRIBUTE, TEMPORARY_VARIABLE, FUNCTION_ARGUMENT };

class jit_function_t {
 public:
  const std::string name;
  const void* address;
  const dcds::valueType returnType;
  const std::vector<std::pair<std::string, dcds::valueType>> args;

  jit_function_t(std::string _name, void* _address, dcds::valueType _return_type,
                 std::vector<std::pair<std::string, dcds::valueType>> _args)
      : name(std::move(_name)), address(_address), returnType(_return_type), args(std::move(_args)) {}
};

// How to store VARCHAR? separate dictionary or std::string and ptr?

// auto valueTypeToString(valueType v){
//   switch (v) {
//     case INTEGER:
//       return "INTEGER";
//     case FLOAT:
//       return "FLOAT";
//     case RECORD_ID:
//       return "RECORD_ID";
//     case CHAR:
//       return "CHAR";
//     case RECORD_PTR:
//       return "RECORD_PTR";
//     case VOID:
//       return "VOID";
//   }
// }

using record_id_t = size_t;
using xid_t = size_t;
using table_id_t = uint16_t;
using column_id_t = uint8_t;

}  // namespace dcds

#endif  // DCDS_TYPES_HPP
