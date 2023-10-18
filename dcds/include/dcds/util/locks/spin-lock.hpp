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

#ifndef DCDS_SPIN_LOCK_HPP
#define DCDS_SPIN_LOCK_HPP

#include <atomic>

namespace dcds::utils::locks {

class SpinLock {
 public:
  SpinLock(SpinLock &&) = delete;
  SpinLock &operator=(SpinLock &&) = delete;
  SpinLock(const SpinLock &) = delete;
  SpinLock &operator=(const SpinLock &) = delete;
  SpinLock() : lk(false) {}

 private:
  std::atomic<bool> lk{};

 public:
  inline bool try_acquire() {
    bool e_false = false;
    if (lk.compare_exchange_strong(e_false, true))
      return true;
    else
      return false;
  }

  inline void acquire() {
    for (int tries = 0; true; ++tries) {
      bool e_false = false;
      if (lk.compare_exchange_strong(e_false, true)) return;
      if (tries == 100) {
        tries = 0;
        sched_yield();
      }
    }
  }
  inline void release() { lk.store(false); }
};

}  // namespace dcds::utils::locks

#endif  // DCDS_SPIN_LOCK_HPP
