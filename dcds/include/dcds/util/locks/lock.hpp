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

#ifndef DCDS_LOCK_HPP
#define DCDS_LOCK_HPP

#include <atomic>
#include <cassert>

namespace dcds::utils::locks {

class Lock {
 public:
  Lock() {
    _lk.store(false);
    assert(_lk.is_lock_free());
  }

  inline bool try_lock() {
    bool e_false = false;
    return _lk.compare_exchange_strong(e_false, true, std::memory_order_acquire);
  }

  inline void unlock() { _lk.store(false, std::memory_order_release); }

 private:
  std::atomic<bool> _lk;
};

}  // namespace dcds::utils::locks

#endif  // DCDS_LOCK_HPP
