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

#include "dcds/transaction/transaction-manager.hpp"

#include "dcds/storage/table.hpp"
#include "dcds/util/logging.hpp"

namespace dcds::txn {

txn_ptr_t TransactionManager::beginTransaction(bool is_read_only) {
  // Create a txn object, add in txn table, and return.
  // As the purpose of this transaction is to support concurrent data structure, this should be very lean and mean.

  //  LOG(WARNING) << "FIXME: beginTxn does not include in txnTable yet";
  // do we need txnTable/lock table at the moment?

  //  return std::make_shared<Txn>(txnIdGenerator.getTxnTs(), is_read_only);
  return new Txn(txnIdGenerator.getTxnTs(), is_read_only);
}

void TransactionManager::releaseAllLocks(txn_ptr_t txn) {
  for (auto& rec : txn->exclusive_locks) {
    dcds::storage::record_reference_t(rec)->unlock();
  }

  for (auto& rec : txn->shared_locks) {
    dcds::storage::record_reference_t(rec)->unlock();
  }
}

bool TransactionManager::endTransaction(txn_ptr_t txn) {
  bool success = true;
  if (txn->status == TXN_STATUS::ACTIVE) {
    // start commit
    commitTransaction(txn);
  } else {
    abortTransaction(txn);
    success = false;
  }
  return success;
}

bool TransactionManager::commitTransaction(txn_ptr_t txn) {
  txn->commit_ts = txnIdGenerator.getCommitTs();
  txn->status = TXN_STATUS::COMMITTED;
  releaseAllLocks(txn);

  return true;
}
bool TransactionManager::abortTransaction(txn_ptr_t txn) {
  // TODO: undo changes.

  releaseAllLocks(txn);
  txn->status = TXN_STATUS::ABORTED;
  return false;
}

// bool LockShared(txn_ptr_t txn, storage::record_reference_t rid) {
//   // blocked on wait
//   // return true when granted
//   // false if txn rolled back / abort
//
//   return true;
// }
//
// bool lockExclusive(txn_ptr_t txn, storage::record_reference_t rid) { return true; }

}  // namespace dcds::txn
