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

#ifndef DCDS_TRANSACTION_HPP
#define DCDS_TRANSACTION_HPP

#include "dcds/common/common.hpp"
#include "dcds/common/types.hpp"
#include "dcds/transaction/txn-utils.hpp"

namespace dcds::txn {

class TransactionManager;

class Txn {
 public:
  Txn(Txn&& other) = delete;
  Txn& operator=(Txn&& other) = delete;
  Txn(const Txn& other) = delete;
  Txn& operator=(const Txn& other) = delete;

  Txn(TxnTs txn_ts, bool is_read_only = false) : txnTs(txn_ts), read_only(is_read_only), status(TXN_STATUS::ACTIVE) {}

 public:
  // Should be in txn-manager
  //  static Txn getTxn(bool is_read_only = false);
  //  static xid_t getTxn(Txn* txnPtr, bool is_read_only = false);
  //  static std::unique_ptr<Txn> make_unique(bool is_read_only = false);

  struct [[maybe_unused]] TxnCmp {
    bool operator()(const Txn& a, const Txn& b) const { return a.txnTs.start_time < b.txnTs.start_time; }
  };

  auto getStatus() { return status; }

 public:
  const TxnTs txnTs;
  const bool read_only;

 private:
  [[maybe_unused]] xid_t commit_ts{};
  TXN_STATUS status;

  friend class TransactionManager;

  // std::vector<row_uuid_t> undoLogVector;
};

}  // namespace dcds::txn

#endif  // DCDS_TRANSACTION_HPP
