//
// Created by Aunn Raza on 19.04.23.
//

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
