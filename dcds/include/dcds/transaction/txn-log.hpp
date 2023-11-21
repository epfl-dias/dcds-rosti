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

#ifndef DCDS_TXN_LOG_HPP
#define DCDS_TXN_LOG_HPP

#include "dcds/common/common.hpp"

namespace dcds::txn {

enum class TXN_LOG_TYPE { INSERT, READ, UPDATE, DELETE };

// TODO: use a allocator template!

class TransactionLogItem {
 public:
  explicit TransactionLogItem(TXN_LOG_TYPE _type, uintptr_t _record) : type(_type), record(_record) {}

 protected:
  TXN_LOG_TYPE type;
  const uintptr_t record;

  friend class TransactionLog;
};

class UpdateLog : public TransactionLogItem {
 public:
  inline UpdateLog(uintptr_t _record, column_id_t _attribute_idx, void* value, size_t _len)
      : TransactionLogItem(TXN_LOG_TYPE::UPDATE, _record),
        attribute_index(_attribute_idx),
        prev_value(value),
        len(_len) {}

 private:
  column_id_t attribute_index;
  void* prev_value;
  size_t len;
  // so that it can be restored back. however, this would require a malloc which does not seem efficient.
  // can we do a create() function which allocates memory for both, updateLog, and the data memory.

  friend class TransactionLog;
};

class InsertLog : public TransactionLogItem {
 public:
  explicit InsertLog(uintptr_t _record) : TransactionLogItem(TXN_LOG_TYPE::INSERT, _record) {}

  friend class TransactionLog;
};

// class DeleteLog :  public TransactionLogItem{
//  public:
//   explicit DeleteLog(): TransactionLogItem(TXN_LOG_TYPE::DELETE){
//     assert(false && "TODO");
//   }
//
//   // Two options, we keep all the record info here, as the record itself is anyway a void*, and if needed to
//   restored,
//   // can be reinserted in the deque, or can it? it might cause index issues if there is.
//
//   // other option is deferred deletes, and perform delete only when a txn is committing.
//
//  private:
//   record_id_t record;
//
// };

class TransactionLog {
 public:
  explicit TransactionLog() = default;

  void addUpdateLog(uintptr_t record, column_id_t attribute_idx, void* prev_value, size_t len);
  void addInsertLog(uintptr_t record);

  void rollback();

 private:
  std::vector<TransactionLogItem*> log;
};

}  // namespace dcds::txn

#endif  // DCDS_TXN_LOG_HPP
