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

#ifndef DCDS_SINGLETON_HPP
#define DCDS_SINGLETON_HPP

#include "dcds/util/erase-constructor-idioms.hpp"

namespace dcds {

template <typename T>
class Singleton : public remove_copy_move {
 protected:
  Singleton() = default;

 public:
  static T& getInstance()  // singleton
  {
    static T instance;
    return instance;
  }
};

}  // namespace dcds

#endif  // DCDS_SINGLETON_HPP
