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

#ifndef DCDS_ATTRIBUTE_DEF_HPP
#define DCDS_ATTRIBUTE_DEF_HPP

#include <iostream>

#include "dcds/common/common.hpp"

namespace dcds::storage {

class AttributeDef {
 public:
  [[nodiscard]] inline auto getName() const { return std::get<0>(col); }
  [[nodiscard]] inline auto getType() const { return std::get<1>(col); }
  [[nodiscard]] inline auto getWidth() const { return std::get<2>(col); }
  [[nodiscard]] inline auto getSize() const { return getWidth(); }
  [[nodiscard]] inline auto getColumnDef() const { return col; }

  explicit AttributeDef(const std::string &name,
                        valueType dType,
                        size_t width)
      : col(name, dType, width) {}

 private:
  std::tuple<std::string, valueType, size_t> col;
};

}

#endif  // DCDS_ATTRIBUTE_DEF_HPP
