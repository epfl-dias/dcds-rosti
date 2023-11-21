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

#ifndef DCDS_COMMON_HPP
#define DCDS_COMMON_HPP

#include <absl/log/log.h>

#include "dcds/common/types.hpp"

#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

namespace dcds {

using record_id_t = size_t;
using xid_t = size_t;
using table_id_t = uint16_t;
using column_id_t = uint8_t;
using rw_set_t = std::map<std::string, std::set<std::string>>;

class jit_function_t {
 public:
  const std::string name;
  const void *address;
  const dcds::valueType returnType;
  const std::vector<std::pair<std::string, dcds::valueType>> args;

  jit_function_t(std::string _name, void *_address, dcds::valueType _return_type,
                 std::vector<std::pair<std::string, dcds::valueType>> _args)
      : name(std::move(_name)), address(_address), returnType(_return_type), args(std::move(_args)) {}
};

}  // namespace dcds

constexpr size_t operator""_K(unsigned long long int x) { return x * 1024; }

constexpr size_t operator""_M(unsigned long long int x) { return x * 1024_K; }

constexpr size_t operator""_G(unsigned long long int x) { return x * 1024_M; }

#endif  // DCDS_COMMON_HPP
