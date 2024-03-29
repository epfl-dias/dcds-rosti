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

#ifndef DCDS_RECORD_METADATA_HPP
#define DCDS_RECORD_METADATA_HPP

#include <oneapi/tbb/rw_mutex.h>

#include "dcds/common/common.hpp"
// #include "dcds/transaction/transaction.hpp"
#include "dcds/util/locks/lock.hpp"
#include "dcds/util/locks/spin-lock.hpp"

namespace dcds::txn::cc {

struct ts_t {
  xid_t deleted : 1;
  xid_t t_min : 63;

  explicit ts_t() : deleted(0), t_min(0) {}
  explicit ts_t(xid_t xact_min) : deleted(0), t_min(xact_min) {}
};

class RecordMetaData {
  // protected:
 private:
  // record_id_t row_id; we do not need record_id for now.
  // utils::locks::SpinLock latch;
  // utils::locks::Lock write_lock;

  oneapi::tbb::rw_mutex _lock{};

 public:
  inline auto unlock_ex() { return _lock.unlock(); }

  inline auto unlock_shared() { return _lock.unlock_shared(); }

  inline auto lock_ex() { return _lock.try_lock(); }
  inline auto lock_shared() { return _lock.try_lock_shared(); }

  // FIXME: HACKED TO AVOID ABORT AND QUEUE LOCKS!
  //  inline auto lock_ex(){ _lock.lock(); return true;}
  //  inline auto lock_shared(){ _lock.lock_shared(); return true;}

  //  inline auto lock() { return write_lock.try_lock(); }
  //  inline void unlock() {
  //    // need some sanity check if unlock is by who locked it.
  //    return write_lock.unlock();
  //  }

  //  template <class lambda>
  //  inline void withLatch(lambda &&func) {
  //    this->latch.acquire();
  //    func(this);
  //    this->latch.release();
  //  }
};

class RecordMetaData_SingleVersion : public RecordMetaData {
 public:
  RecordMetaData_SingleVersion() = default;
  explicit RecordMetaData_SingleVersion(xid_t) {}

  //  template <class lambda>
  //  inline void withLatch(lambda &&func) {
  //    this->latch.acquire();
  //    func(this);
  //    this->latch.release();
  //  }

  //  // NOTE: the purpose of having separate functions write/read with latch:
  //  //  writeWithLatch: additionally log changes in case of abort.
  //  //  readWithLatch: future optimization for optimistic reads.
  //  template <class lambda>
  //  inline void writeWithLatch(lambda &&func) {
  //    this->latch.acquire();
  //    func(this);
  //    this->latch.release();
  //  }
  //
  //  template <class lambda>
  //  inline void readWithLatch(lambda &&func) {
  //    this->latch.acquire();
  //    func(this);
  //    this->latch.release();
  //  }
};

//    class RecordMetaData_MultiVersion : RecordMetaData {
//        ts_t record_ts;
//        void *deltaList;
//        // deltaList
//    public:
//        RecordMetaData_MultiVersion() {}
//
//        RecordMetaData_MultiVersion(xid_t ts) : record_ts(ts), deltaList(nullptr) {
//        }
//    };

}  // namespace dcds::txn::cc

#endif  // DCDS_RECORD_METADATA_HPP
