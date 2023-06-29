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

#ifndef DCDS_SCOPED_HPP
#define DCDS_SCOPED_HPP

#include "dcds/util/erase-constructor-idioms.hpp"

// Something like https://github.com/ricab/scope_guard/blob/main/scope_guard.hpp

namespace dcds {

// template <typename T>
// class scoped: private remove_copy_move{
//
//   scoped() = default;
//   scoped(Args...): obj(...Args){}
//
//  private:
//   T obj;
// };
//
// template<typename onDestruct>
// class Scoped : private remove_copy_move{
//
//
//
//   void release(){
//     engaged = false;
//   }
//
//   ~Scoped(){
//     if(engaged){
//       destruct_fn();
//     }
//   }
//
//  private:
//   bool engaged;
//   onDestruct destruct_fn;
// };

}

#endif  // DCDS_SCOPED_HPP
