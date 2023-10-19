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

#ifndef DCDS_LOGGING_HPP
#define DCDS_LOGGING_HPP

#include <absl/log/check.h>
#include <absl/log/log.h>

#include <cassert>
#include <concepts>
#include <numeric>

namespace dcds {
void InitializeLog(int argc, char** argv);

template <class Container>  // add enable_id is subtype is convertible to std::string
std::string joinString(const Container& container, const std::string& connector = ", ") {
  static_assert(std::is_convertible_v<typename Container::value_type, std::string>,
                "Container must be of type convertible_to std::string");

  if (container.empty()) return "";

  return std::accumulate(std::next(std::begin(container)), std::end(container), *std::begin(container),
                         [connector](const std::string& a, const std::string& b) { return a + connector + b; });
}
}  // namespace dcds

#endif  // DCDS_LOGGING_HPP
