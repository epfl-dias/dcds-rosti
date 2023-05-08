//
// Created by Aunn Raza on 19.04.23.
//

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
