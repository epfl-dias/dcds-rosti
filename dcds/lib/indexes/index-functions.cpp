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

#include "dcds/indexes/index-functions.hpp"

#include "dcds/indexes/index.hpp"

// #include <libcuckoo/cuckoohash_map.hh>

//// FIXME: maybe change it to uintptr_t
// void* getCuckooMap(dcds::valueType value_type) {
//   switch (value_type) {
//     case dcds::valueType::INT64:
//       return new libcuckoo::cuckoohash_map<int64_t, void*>();
//     case dcds::valueType::INT32:
//       return new libcuckoo::cuckoohash_map<int32_t, void*>();
//     case dcds::valueType::FLOAT:
//     case dcds::valueType::DOUBLE:
//     case dcds::valueType::RECORD_PTR:
//     case dcds::valueType::BOOL:
//     case dcds::valueType::VOID:
//       assert(false);
//       break;
//   }
// }

uintptr_t createIndexMap(dcds::valueType key_type) {
  void* ret;

  switch (key_type) {
    case dcds::valueType::INT64:
      ret = new dcds::indexes::CuckooHashIndex<int64_t>();
      break;
    case dcds::valueType::INT32:
      ret = new dcds::indexes::CuckooHashIndex<int32_t>();
      break;
    case dcds::valueType::FLOAT:
      ret = new dcds::indexes::CuckooHashIndex<float>();
      break;
    case dcds::valueType::DOUBLE:
      ret = new dcds::indexes::CuckooHashIndex<double>();
      break;
    case dcds::valueType::RECORD_PTR:
    case dcds::valueType::BOOL:
    case dcds::valueType::VOID:
      assert(false);
      break;
  }

  assert(ret != nullptr);

  // LOG(INFO) << "createIndexMap: ptr: " << ret << " | uintptr_t: " << reinterpret_cast<uintptr_t>(ret);
  return reinterpret_cast<uintptr_t>(ret);
}
