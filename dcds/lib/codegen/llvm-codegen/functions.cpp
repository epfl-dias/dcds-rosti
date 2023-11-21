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

#include "dcds/codegen/llvm-codegen/functions.hpp"

#include "dcds/exporter/jit-container.hpp"
#include "dcds/storage/table-registry.hpp"
#include "dcds/transaction/transaction-manager.hpp"
#include "dcds/transaction/transaction-namespaces.hpp"

int printc(char* X) {
  printf("[printc:] %c\n", X[0]);
  return 0;
}

int prints(char* X) {
  printf("[prints:] %s\n", X);
  return 0;
}

void printPtr(void* X) { LOG(INFO) << "[printPtr:] " << X; }

void printUInt64(uint64_t X) { LOG(INFO) << "[printUInt64:] " << X; }

void* getTableRegistry() { return &(dcds::storage::TableRegistry::getInstance()); }

uint doesTableExists(const char* table_name) { return dcds::storage::TableRegistry::getInstance().exists(table_name); }

void* c1(char* table_name) {
  LOG(INFO) << "C1";
  LOG(INFO) << "arg: " << table_name;
  return nullptr;
}
void* c2(int num_attributes) {
  LOG(INFO) << "C2";
  LOG(INFO) << "arg: " << num_attributes;
  return nullptr;
}
void* c3(const dcds::valueType attributeTypes[]) {
  LOG(INFO) << "C3";
  return nullptr;
}
void* c4(char* attributeNames) {
  LOG(INFO) << "C4";
  return nullptr;
}

void* createTablesInternal(char* table_name, dcds::valueType attributeTypes[], char* attributeNames[],
                           int num_attributes) {
  static std::mutex create_table_m;

  // create a static lock here so that everything is safer.
  auto& tableRegistry = dcds::storage::TableRegistry::getInstance();

  std::vector<dcds::storage::AttributeDef> columns;

  LOG(INFO) << "[createTablesInternal] table_name: " << table_name;
  LOG(INFO) << "[createTablesInternal] num_attributes: " << num_attributes;

  // HACK: as we are getting pointer to first character of first attribute instead of ptr.
  auto* startPtr = reinterpret_cast<char*>(attributeNames);

  std::vector<std::string> actual_attr_names;
  actual_attr_names.reserve(num_attributes);
  for (auto i = 0; i < num_attributes; i++) {
    auto sln = strlen(startPtr);
    actual_attr_names.emplace_back(startPtr);
    startPtr += sln + 1;
  }

  for (auto i = 0; i < num_attributes; i++) {
    auto type = attributeTypes[i];
    auto name = actual_attr_names[i];
    LOG(INFO) << "[createTablesInternal] Loading attribute: " << name << " | type: " << type;

    int64_t attribute_size = 0;
    switch (type) {
      case dcds::valueType::INT64:
        attribute_size = sizeof(int64_t);
        break;
      case dcds::valueType::INT32:
        attribute_size = sizeof(int32_t);
        break;
      case dcds::valueType::FLOAT:
        attribute_size = sizeof(float);
        break;
      case dcds::valueType::DOUBLE:
        attribute_size = sizeof(double);
        break;
      case dcds::valueType::RECORD_PTR:
        attribute_size = sizeof(void*);
        break;
      case dcds::valueType::BOOL:
        attribute_size = sizeof(bool);
        break;
      case dcds::valueType::VOID:
        assert(false && "void type cannot be used as variable type");
    }
    columns.emplace_back(name, type, attribute_size);
  }

  // CRITICAL SECTION: so that if two DS instances are getting initialized together,
  // we don't create the same table twice.
  dcds::storage::Table* ret_table_ptr;
  {
    std::unique_lock<std::mutex> lk(create_table_m);
    if (tableRegistry.exists(table_name)) {
      ret_table_ptr = tableRegistry.getTable(table_name);
    } else {
      ret_table_ptr = tableRegistry.createTable(table_name, columns);
    }

    assert(ret_table_ptr);
  }

  return ret_table_ptr;
}

void* getTxnManager(const char* txn_namespace) {
  auto x = dcds::txn::NamespaceRegistry::getInstance().getOrCreate(txn_namespace);
  return x.get();
}

void* getTable(const char* table_name) { return dcds::storage::TableRegistry::getInstance().getTable(table_name); }

void* beginTxn(void* txnManager, bool isReadOnly) {
  auto txnPtr = static_cast<dcds::txn::TransactionManager*>(txnManager)->beginTransaction(false);
  return txnPtr;
}

bool endTxn(void* txnManager, void* txnPtr) {
  return static_cast<dcds::txn::TransactionManager*>(txnManager)->endTransaction(static_cast<dcds::txn::Txn*>(txnPtr));
}

uintptr_t insertMainRecord(void* table, void* txn, void* data) {
  auto x = static_cast<dcds::storage::Table*>(table)->insertRecord(static_cast<dcds::txn::Txn*>(txn), data);
  return x.getBase();
}

uintptr_t insertNRecords(void* table, void* txn, void* data, size_t N) {
  // LOG(INFO) << "insertNRecords[" << static_cast<dcds::storage::Table*>(table)->name() << "] : N: " << N;
  auto x = static_cast<dcds::storage::Table*>(table)->insertNRecord(static_cast<dcds::txn::Txn*>(txn), N, data);
  return x.getBase();
}

void* createDsContainer(void* txnManager, uintptr_t data) {
  //  this should return dcds_jit_container_t only. not the full thing in my opinion.
  //  return dcds::JitContainer::create(txnManager, storageTable, data);

  auto container_ptr = new dcds::JitContainer::dcds_jit_container_t();
  container_ptr->mainRecord = data;
  container_ptr->txnManager = reinterpret_cast<dcds::txn::TransactionManager*>(txnManager);

  return container_ptr;
}

uintptr_t extractRecordFromDsContainer(void* container) {
  auto c = static_cast<dcds::JitContainer::dcds_jit_container_t*>(container);
  return c->mainRecord;
}

void table_read_attribute(void* _txnManager, uintptr_t _mainRecord, void* txnPtr, void* dst, size_t attributeIdx) {
  // auto txnManager = reinterpret_cast<dcds::txn::TransactionManager*>(_txnManager);
  auto mainRecord = dcds::storage::record_reference_t(_mainRecord);
  auto storageTable = mainRecord.getTable();
  auto* txn = reinterpret_cast<dcds::txn::Txn*>(txnPtr);

  storageTable->getAttribute(txn, mainRecord.operator->(), dst, attributeIdx);
}

void table_read_attribute_offset(void* _txnManager, uintptr_t _mainRecord, void* txnPtr, void* dst, size_t attributeIdx,
                                 size_t record_offset) {
  // auto txnManager = reinterpret_cast<dcds::txn::TransactionManager*>(_txnManager);
  auto mainRecord = dcds::storage::record_reference_t(_mainRecord);
  auto storageTable = mainRecord.getTable();
  auto* txn = reinterpret_cast<dcds::txn::Txn*>(txnPtr);

  storageTable->getNthRecord(txn, mainRecord.operator->(), dst, record_offset, attributeIdx);
}

void table_write_attribute(void* _txnManager, uintptr_t _mainRecord, void* txnPtr, void* src, uint attributeIdx) {
  // auto txnManager = reinterpret_cast<dcds::txn::TransactionManager*>(_txnManager);
  auto mainRecord = dcds::storage::record_reference_t(_mainRecord);
  auto storageTable = mainRecord.getTable();
  auto* txn = reinterpret_cast<dcds::txn::Txn*>(txnPtr);

  storageTable->updateAttribute(txn, mainRecord.operator->(), src, attributeIdx);
}

void table_write_attribute_offset(void* _txnManager, uintptr_t _mainRecord, void* txnPtr, void* src, uint attributeIdx,
                                  size_t record_offset) {
  // auto txnManager = reinterpret_cast<dcds::txn::TransactionManager*>(_txnManager);
  auto mainRecord = dcds::storage::record_reference_t(_mainRecord);
  auto storageTable = mainRecord.getTable();
  auto* txn = reinterpret_cast<dcds::txn::Txn*>(txnPtr);

  storageTable->updateNthRecord(txn, mainRecord.operator->(), src, record_offset, attributeIdx);
}

uintptr_t table_get_nth_record(void* _txnManager, uintptr_t _mainRecord, void* txnPtr, size_t record_offset) {
  // auto txnManager = reinterpret_cast<dcds::txn::TransactionManager*>(_txnManager);
  auto mainRecord = dcds::storage::record_reference_t(_mainRecord);
  auto storageTable = mainRecord.getTable();
  auto* txn = reinterpret_cast<dcds::txn::Txn*>(txnPtr);

  auto x = storageTable->getNthRecordReference(txn, mainRecord.operator->(), record_offset);
  return x.getBase();
}

bool lock_shared(void* _txnManager, void* txnPtr, uintptr_t record) {
  // LOG(WARNING) << "lock_shared: " << record;
  //  return lock_exclusive(_txnManager, txnPtr, record);

  auto* txn = static_cast<dcds::txn::Txn*>(txnPtr);
  auto mainRecord = dcds::storage::record_reference_t(record);

  if (unlikely(txn->exclusive_locks.contains(record))) {
    return true;
  } else {
    auto acquire_success = mainRecord.operator->()->lock_shared();
    if (acquire_success) {
      txn->shared_locks.insert(record);
      return true;
    } else {
      txn->status = dcds::txn::TXN_STATUS::ABORTED;
      // LOG(INFO) << "lock-failed";
      return false;
    }
  }
}
bool lock_exclusive(void* _txnManager, void* txnPtr, uintptr_t record) {
  // LOG(INFO) << "lock_exclusive: " << record;

  auto* txn = static_cast<dcds::txn::Txn*>(txnPtr);
  auto mainRecord = dcds::storage::record_reference_t(record);

  if (unlikely(txn->exclusive_locks.contains(record))) {
    return true;
  } else {
    if (unlikely(txn->shared_locks.contains(record))) {
      txn->shared_locks.erase(record);
      mainRecord->unlock_shared();
    }
    auto acquire_success = mainRecord.operator->()->lock_ex();
    //    auto acquire_success = mainRecord.operator->()->lock_exclusive();
    if (acquire_success) {
      txn->exclusive_locks.insert(record);
      return true;
    } else {
      txn->status = dcds::txn::TXN_STATUS::ABORTED;
      // LOG(INFO) << "lock-failed";
      return false;
    }
  }
}
// bool unlock_all(void* _txnManager, void* txnPtr) { return true; }

// bool unlock_shared(void* _txnManager, uintptr_t record, void* txnPtr){}
// bool unlock_exclusive(void* _txnManager, uintptr_t record, void* txnPtr){}
