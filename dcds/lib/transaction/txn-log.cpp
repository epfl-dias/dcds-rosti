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

#include "dcds/transaction/txn-log.hpp"

#include "dcds/storage/table.hpp"
#include "dcds/util/logging.hpp"

namespace dcds::txn {

void TransactionLog::addUpdateLog(uintptr_t record, column_id_t attribute_idx, void* prev_value, size_t len) {
  // FIXME: potentially do a one big malloc, so that the actual updateLog and data can be in the same allocation.
  void* data_location = malloc(len);
  memcpy(data_location, prev_value, len);
  this->log.push_back(new UpdateLog(record, attribute_idx, data_location, len));
}
void TransactionLog::addInsertLog(uintptr_t record) { this->log.push_back(new InsertLog(record)); }

void TransactionLog::rollback() {
  for (auto& action : this->log) {
    auto mainRecord = dcds::storage::record_reference_t(action->record);
    auto storageTable = mainRecord.getTable();

    if (action->type == TXN_LOG_TYPE::INSERT) {
      storageTable->rollback_create(mainRecord.operator->());
    } else if (action->type == TXN_LOG_TYPE::UPDATE) {
      auto upd_action = reinterpret_cast<UpdateLog*>(action);
      storageTable->rollback_update(mainRecord.operator->(), upd_action->prev_value, upd_action->attribute_index);

    } else {
      CHECK(false) << "what kind of log type?";
    }
  }
}

}  // namespace dcds::txn
