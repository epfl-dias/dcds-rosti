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

#ifndef DCDS_CONTEXT_HPP
#define DCDS_CONTEXT_HPP

#include <iostream>

#include "dcds/util/erase-constructor-idioms.hpp"

namespace dcds {
namespace detail {
class DCDSContextImpl {
 public:
  explicit DCDSContextImpl(bool enable_multi_threading, bool allow_dangling_functions)
      : enableMultithreading(enable_multi_threading), allowDanglingFunctions(allow_dangling_functions) {}

  bool getMultiThreadingStatus() const { return enableMultithreading; }
  bool getDanglingFunctionsStatus() const { return allowDanglingFunctions; }

  // Add locking utilities for individual operations here
  // Even add transaction manager and related table stuff here?
  // Perform Dangling function acceptance/rejection here.
  // Add functionality to deal with storage preferences here.

 private:
  bool enableMultithreading;
  bool allowDanglingFunctions;
};
}  // namespace detail

class DCDSContext : dcds::remove_copy_move {
 public:
  explicit DCDSContext(bool enable_multi_threading = false, bool allow_dangling_functions = false)
      : contextImpl(detail::DCDSContextImpl(enable_multi_threading, allow_dangling_functions)) {}

  detail::DCDSContextImpl contextImpl;
};
}  // namespace dcds

#endif  // DCDS_CONTEXT_HPP
