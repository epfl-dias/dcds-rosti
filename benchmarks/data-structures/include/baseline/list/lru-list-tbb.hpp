/*
                              Copyright (c) 2024.
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

#ifndef DCDS_LRU_LIST_TBB_HPP
#define DCDS_LRU_LIST_TBB_HPP

#define TBB_PREVIEW_CONCURRENT_LRU_CACHE 1
#include "oneapi/tbb/concurrent_lru_cache.h"

namespace dcds::baseline {

template <typename K = size_t, typename V = size_t>
class LruListTbb {
 private:
  tbb::concurrent_lru_cache<K, V> instance;
  static V ConstructValue([[maybe_unused]] K key) { return key; }

 public:
  explicit LruListTbb(size_t capacity) : instance(&ConstructValue, capacity) {}

  inline auto insert(K key, V value) {
    auto x = instance[key];
    return true;
  }
};

}  // namespace dcds::baseline

#endif  // DCDS_LRU_LIST_TBB_HPP
