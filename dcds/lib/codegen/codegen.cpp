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

//
// Created by prathamesh on 4/8/23.
//

#include <dcds/builder/storage.hpp>
#include <dcds/codegen/codegen.hpp>

extern "C" {
void read_data_structure_attribute(void *dsRecord, int64_t attributeIndex, void *readVariable, int64_t attributeTy = -1,
                                   void *txnPtr = nullptr) {
  auto dsRecordContainer = reinterpret_cast<dcds::StorageLayer *>(dsRecord);

  if (attributeTy == 0) {
    auto readVariableContainer = reinterpret_cast<int64_t *>(readVariable);
    dsRecordContainer->read(readVariableContainer, attributeIndex, txnPtr);
  } else if (attributeTy == 1) {
    // read void ptr
    dsRecordContainer->read(readVariable, attributeIndex, txnPtr);
  }
}

void write_data_structure_attribute(void *dsRecord, int64_t attributeIndex, void *writeVariable,
                                    int64_t attributeTy = -1, void *txnPtr = nullptr) {
  auto dsRecordContainer = reinterpret_cast<dcds::StorageLayer *>(dsRecord);

  if (attributeTy == 0) {
    auto writeVariableContainer = reinterpret_cast<int64_t *>(writeVariable);
    dsRecordContainer->write(writeVariableContainer, attributeIndex, txnPtr);
  } else if (attributeTy == 1) {
    // write void ptr
    dsRecordContainer->write(&writeVariable, attributeIndex, txnPtr);
  }
}

void *begin_txn(void *dsRecord) {
  auto dsRecordContainer = reinterpret_cast<dcds::StorageLayer *>(dsRecord);

  return dsRecordContainer->beginTxn();
}

void end_txn(void *dsRecord, void *txn) {
  auto dsRecordContainer = reinterpret_cast<dcds::StorageLayer *>(dsRecord);

  dsRecordContainer->commitTxn(txn);
}
}
