//
// Created by Aunn Raza on 19.04.23.
//

#ifndef DCDS_LATCH_HPP
#define DCDS_LATCH_HPP

#include "dcds/util/locks/spin-lock.hpp"

namespace dcds::utils::locks{

// acquire on construct, release on destruct.

using Latch = SpinLock;

template <class spin_lock_t = SpinLock>
class ScopedLatch{
 public:
  ScopedLatch(ScopedLatch &&) = delete;
  ScopedLatch &operator=(ScopedLatch &&) = delete;
  ScopedLatch(const ScopedLatch &) = delete;
  ScopedLatch &operator=(const ScopedLatch &) = delete;

  ScopedLatch(spin_lock_t &latchReference):latch_ref(&latchReference){
    latchReference.acquire();
  }

  ~ScopedLatch(){
    latch_ref->release();
  }
 private:
  const spin_lock_t* latch_ref;

};


}

#endif  // DCDS_LATCH_HPP
