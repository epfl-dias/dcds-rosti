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

#include "dcds/common/exceptions/exception.hpp"

using namespace dcds::exceptions;

dcds_dynamic_exception::dcds_dynamic_exception(std::string msg) : msg_(std::make_shared<std::string>(std::move(msg))) {}

const char* dcds_dynamic_exception::what() const noexcept { return msg_->c_str(); }

dcds_invalid_type_exception::dcds_invalid_type_exception(std::string msg)
    : msg_(std::make_shared<std::string>(std::move(msg))) {}

const char* dcds_invalid_type_exception::what() const noexcept { return msg_->c_str(); }
