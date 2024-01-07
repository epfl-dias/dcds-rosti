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

#ifndef DCDS_INDEX_FUNCTIONS_HPP
#define DCDS_INDEX_FUNCTIONS_HPP

#include <iostream>

#include "dcds/common/common.hpp"
#include "dcds/common/types.hpp"
#include "dcds/indexes/index.hpp"

extern "C" uintptr_t createIndexMap(dcds::valueType key_type);

template <typename K>
uintptr_t index_find(uintptr_t index, K key) {
  uintptr_t ret = 0;
  reinterpret_cast<dcds::indexes::Index<K>*>(index)->_find(key, ret);
  // LOG(INFO) <<"[index_find] Index: " << index <<  " | key: " << key << " found_status: " << found <<  " | val: " <<
  // ret;
  return ret;
}

template <typename K>
bool contains(uintptr_t index, K key) {
  return reinterpret_cast<dcds::indexes::Index<K>*>(index)->_contains(key);
}

template <typename K>
bool index_insert(uintptr_t index, K key, uintptr_t value) {
  auto idx = reinterpret_cast<dcds::indexes::Index<K>*>(index);
  auto ins_res = idx->_insert(key, value);
  // LOG(INFO) <<"[index_insert] Index: " << index <<  " | key: " << key << " | val: " << value << " |res: " << ins_res;
  return ins_res;
}

template <typename K>
void index_remove(uintptr_t index, K key) {
  auto idx = reinterpret_cast<dcds::indexes::Index<K>*>(index);
  idx->_remove(key);
  // LOG(INFO) << "[index_remove]: remove key: " << key;
}

#endif  // DCDS_INDEX_FUNCTIONS_HPP
