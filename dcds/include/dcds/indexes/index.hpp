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

#ifndef DCDS_INDEX_HPP
#define DCDS_INDEX_HPP

#include <iostream>
#include <libcuckoo/cuckoohash_map.hh>

namespace dcds::indexes {

// NOTE: value_type should be default constructible

template <typename K>
class Index {
 public:
  using key_type = K;
  using value_type = uintptr_t;

  virtual value_type _find(key_type key) = 0;
  virtual bool _find(key_type key, value_type &value) = 0;
  virtual bool _contains(key_type key) = 0;

  virtual bool _insert(key_type key, value_type value) = 0;
  virtual bool _update(key_type key, value_type value) = 0;
  virtual void _remove(key_type key) = 0;
};

template <typename K>
class CuckooHashIndex : public Index<K> {
 public:
  CuckooHashIndex() { _idx.reserve(1_M); };

  using value_type = typename Index<K>::value_type;
  using key_type = typename Index<K>::key_type;

  value_type _find(key_type key) override { return _idx.find(key); }

  bool _find(key_type key, value_type &value) override {
    // return _idx.find(key, value);
    return _idx.find(key, value);
    // return _idx.find_fn(key, [&value](const auto &v) mutable { value = v; });
  }

  bool _insert(key_type key, value_type value) override { return _idx.insert(key, value); }

  bool _contains(key_type key) override { return _idx.contains(key); }

  bool _update(key_type key, value_type value) override { return _idx.update(key, value); }

  void _remove(key_type key) override { _idx.erase(key); }

 private:
  libcuckoo::cuckoohash_map<K, uintptr_t> _idx;
};

}  // namespace dcds::indexes

#endif  // DCDS_INDEX_HPP
