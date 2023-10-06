//
// Created by Aunn Raza on 18.04.23.
//

#ifndef DCDS_TYPES_HPP
#define DCDS_TYPES_HPP

#include <iostream>

namespace dcds {

// NOTE: VOID is strictly a return type.
enum valueType { INTEGER, FLOAT, DATE, RECORD_ID, CHAR, RECORD_PTR, VOID };

// How to store VARCHAR? separate dictionary or std::string and ptr?

using record_id_t = size_t;
using xid_t = size_t;
using table_id_t = uint16_t;
using column_id_t = uint8_t;

}  // namespace dcds

#endif  // DCDS_TYPES_HPP
