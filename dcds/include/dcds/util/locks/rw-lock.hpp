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

#ifndef DCDS_RW_LOCK_HPP
#define DCDS_RW_LOCK_HPP

#include <pthread.h>

#include <atomic>

#include "dcds/util/erase-constructor-idioms.hpp"
#include "dcds/util/intrinsic-macros.hpp"

namespace dcds::utils::locks {

class RWLock : public dcds::remove_copy_move {
 public:
  RWLock() = default;
  RWLock(RWLock &&) = delete;
  RWLock &operator=(RWLock &&) = delete;
  RWLock(const RWLock &) = delete;
  RWLock &operator=(const RWLock &) = delete;

 public:
  virtual void lock_exclusive() = 0;
  virtual void lock_shared() = 0;
  virtual void lock_upgrade() = 0;

  virtual bool try_lock_exclusive() = 0;
  virtual bool try_lock_shared() = 0;
  virtual bool try_lock_upgrade() = 0;

  virtual void unlock_exclusive() = 0;
  virtual void unlock_shared() = 0;

  virtual ~RWLock();
};

// To implement:
//  - seqLocks
//  - counter-based RW lock
//  - reserve/update/downgrade

class pthreadRwLock : public RWLock {
 public:
  pthreadRwLock() : RWLock() { pthread_rwlock_init(&lk, nullptr); }
  ~pthreadRwLock() override { pthread_rwlock_destroy(&lk); }

  void lock_exclusive() override {}
  void lock_shared() override {}
  void lock_upgrade() override {}

  bool try_lock_exclusive() override {}
  bool try_lock_shared() override {}
  bool try_lock_upgrade() override {}

  void unlock_exclusive() override {}
  void unlock_shared() override {}

 private:
  pthread_rwlock_t lk;
};

// Note: can lead to writer starvation as exclusive lock can only be acquired when there are no concurrent shared locks
// taken.
class AtomicCtrRwLock : public RWLock {
 private:
  std::atomic<int> counter{0};

 public:
  void lock_exclusive() override {
    // expected == 0 : that is, neither exclusive, nor shared lock has been taken already.
    int expected;
    do {
      expected = 0;

    } while (!lockExclusive(expected));
  }
  void lock_shared() override {
    // expected != -1 : that is, not exclusively locked
    int expected;

    do {
      //      do {
      //        expected = counter.load(std::memory_order_relaxed);
      //      } while (expected == -1);  // spin until writer unlocks
      expected = counter.load(std::memory_order_relaxed);

      while (expected == -1) {
        DCDS_SPIN_PAUSE()
        expected = counter.load(std::memory_order_relaxed);
      }

    } while (!lockShared(expected));
  }
  void lock_upgrade() override {
    // expected == 1 : assuming the requester already has a shared lock
    int expected;
    do {
      expected = 1;

    } while (!lockExclusive(expected));
  }

  bool try_lock_exclusive() override {
    int expected = 0;
    return lockExclusive(expected);
  }
  bool try_lock_shared() override {
    int expected = counter.load(std::memory_order_relaxed);
    if (expected != -1) {
      return lockShared(expected);
    } else {
      return false;
    }
  }
  bool try_lock_upgrade() override {
    int expected = 1;
    return lockExclusive(expected);
  }

  void unlock_exclusive() override { counter.store(0, std::memory_order_release); }
  void unlock_shared() override { counter.fetch_sub(1, std::memory_order_release); }

 private:
  inline bool lockExclusive(auto &expected) {
    return counter.compare_exchange_weak(expected, -1, std::memory_order_acquire);
  }

  inline bool lockShared(auto &expected) {
    return counter.compare_exchange_weak(expected, expected + 1, std::memory_order_acquire);
  }
};

}  // namespace dcds::utils::locks

#endif  // DCDS_RW_LOCK_HPP
