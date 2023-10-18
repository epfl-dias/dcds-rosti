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

#ifndef DCDS_LATCH_HPP
#define DCDS_LATCH_HPP

#include "dcds/util/locks/spin-lock.hpp"

namespace dcds::utils::locks {

// acquire on construct, release on destruct.

using Latch = SpinLock;

template <class spin_lock_t = SpinLock>
class ScopedLatch {
 public:
  ScopedLatch(ScopedLatch &&) = delete;
  ScopedLatch &operator=(ScopedLatch &&) = delete;
  ScopedLatch(const ScopedLatch &) = delete;
  ScopedLatch &operator=(const ScopedLatch &) = delete;

  ScopedLatch(spin_lock_t &latchReference) : latch_ref(&latchReference) { latchReference.acquire(); }

  ~ScopedLatch() { latch_ref->release(); }

 private:
  const spin_lock_t *latch_ref;
};

}  // namespace dcds::utils::locks

#endif  // DCDS_LATCH_HPP
