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

#ifndef DCDS_LRU_LIST_GLOBAL_LOCK_HPP
#define DCDS_LRU_LIST_GLOBAL_LOCK_HPP

#include <iostream>
#include <list>
#include <mutex>
#include <unordered_map>

namespace dcds::baseline {

template <typename KeyType, typename ValueType>
class LruListStd {
 public:
  explicit LruListStd(size_t _capacity) : capacity(_capacity) {}

  // Get the value associated with the key. If key is not present, return -1.
  ValueType get(const KeyType& key) {
    std::unique_lock<std::mutex> lk(_lk);

    auto it = cacheMap.find(key);

    // If key is not present in the cache, return -1
    if (it == cacheMap.end()) {
      return -1;
    }

    // Move the accessed key to the front (most recently used)
    cacheList.splice(cacheList.begin(), cacheList, it->second);
    return it->second->second;
  }

  // Put the key-value pair into the cache.
  void insert(const KeyType& key, const ValueType& value) {
    std::unique_lock<std::mutex> lk(_lk);

    auto it = cacheMap.find(key);

    // If the key is already present, update the value and move it to the front
    if (it != cacheMap.end()) {
      it->second->second = value;
      cacheList.splice(cacheList.begin(), cacheList, it->second);
    } else {
      // If the cache is full, remove the least recently used element
      if (cacheList.size() >= capacity) {
        auto leastUsed = cacheList.back();
        cacheMap.erase(leastUsed.first);
        cacheList.pop_back();
      }

      // Insert the new key-value pair at the front
      cacheList.emplace_front(key, value);
      cacheMap[key] = cacheList.begin();
    }
  }

 private:
  const size_t capacity;

 private:
  std::mutex _lk;

  std::list<std::pair<KeyType, ValueType>> cacheList;  // Stores the key-value pairs
  std::unordered_map<KeyType, typename std::list<std::pair<KeyType, ValueType>>::iterator>
      cacheMap;  // Maps keys to iterators in the list
};

}  // namespace dcds::baseline

#endif  // DCDS_LRU_LIST_GLOBAL_LOCK_HPP
