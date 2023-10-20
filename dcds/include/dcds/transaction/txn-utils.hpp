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

#ifndef DCDS_TXN_UTILS_HPP
#define DCDS_TXN_UTILS_HPP

#include "dcds/common/common.hpp"
#include "dcds/common/types.hpp"

namespace dcds::txn {

// ACTIVE, IDLE, FINISHED
// ACTIVE, COMMITTED, ABORTED
enum class TXN_STATUS { ACTIVE, COMMITTED, ABORTED };

class TxnTs {
 public:
  xid_t txn_id;
  xid_t start_time;

  explicit TxnTs(xid_t txn_id_, xid_t start_time_) : txn_id(txn_id_), start_time(start_time_) {}
  explicit TxnTs(std::pair<xid_t, xid_t> txnTs_pair) : txn_id(txnTs_pair.first), start_time(txnTs_pair.second) {}

  struct TxnTsCmp {
    bool operator()(const TxnTs& a, const TxnTs& b) const { return a.start_time < b.start_time; }
  };
};

class TxnTsGenerator {
 public:
  static constexpr auto baseShift = 27u;

 public:
  inline xid_t __attribute__((always_inline)) getCommitTs() { return gen.fetch_add(1); }

  inline TxnTs __attribute__((always_inline)) getTxnTs() {
    auto x = gen.fetch_add(1);
    return TxnTs{x << baseShift, x};
  }

 private:
  std::atomic<xid_t> gen{};
};

}  // namespace dcds::txn

#endif  // DCDS_TXN_UTILS_HPP
