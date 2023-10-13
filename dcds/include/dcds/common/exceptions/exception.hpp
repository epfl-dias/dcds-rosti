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

#ifndef DCDS_EXCEPTION_HPP
#define DCDS_EXCEPTION_HPP

#include <iostream>

namespace dcds::exceptions {

class dcds_dynamic_exception : public std::exception {
  // shared_ptr as exceptions are not allowed to throw during copy
  std::shared_ptr<std::string> msg_;

 public:
  explicit dcds_dynamic_exception(std::string msg);

  [[nodiscard]] const char* what() const noexcept override;
};

class dcds_invalid_type_exception : public std::exception {
  // shared_ptr as exceptions are not allowed to throw during copy
  std::shared_ptr<std::string> msg_;

 public:
  explicit dcds_invalid_type_exception(std::string msg);

  [[nodiscard]] const char* what() const noexcept override;
};

}  // namespace dcds::exceptions

#endif  // DCDS_EXCEPTION_HPP
