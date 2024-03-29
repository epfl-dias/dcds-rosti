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

#ifndef DCDS_TRANSACTION_MANAGER_HPP
#define DCDS_TRANSACTION_MANAGER_HPP

#include <atomic>
#include <iostream>
#include <utility>

#include "dcds/common/common.hpp"
#include "dcds/common/types.hpp"
#include "dcds/transaction/transaction.hpp"
#include "dcds/transaction/txn-utils.hpp"
#include "oneapi/tbb/tbb_allocator.h"

// TransactionTable
namespace dcds::txn {

using txn_ptr_t = Txn *;

class TransactionManager {
 public:
  explicit TransactionManager(std::string namespace_name = "default") : txn_namespace(std::move(namespace_name)) {}
  TransactionManager(TransactionManager &&) = delete;
  TransactionManager &operator=(TransactionManager &&) = delete;
  TransactionManager(const TransactionManager &) = delete;
  TransactionManager &operator=(const TransactionManager &) = delete;

 public:
  ~TransactionManager() {
    // LOG(INFO) <<"~TransactionManager["<<txn_namespace<<"]";
    LOG(INFO) << "~TransactionManager[" << txn_namespace << "] commits: " << commit;
    LOG(INFO) << "~TransactionManager[" << txn_namespace << "] aborts: " << abort;
  }

 private:
  const std::string txn_namespace;

 public:
  txn_ptr_t beginTransaction(bool is_read_only);
  bool endTransaction(txn_ptr_t txn);

 private:
  bool commitTransaction(txn_ptr_t txn);
  bool abortTransaction(txn_ptr_t txn);

 private:
  void releaseAllLocks(txn_ptr_t txn);

 private:
  TxnTsGenerator txnIdGenerator{};

 private:
  std::atomic<size_t> commit{};
  std::atomic<size_t> abort{};

  // need some concurrent data structure for storing transactions in sorted manner.
};

}  // namespace dcds::txn

#endif  // DCDS_TRANSACTION_MANAGER_HPP
