//
// Created by prathamesh on 11/8/23.
//

#include <dcds/builder/storage.hpp>
#include <dcds/storage/table-registry.hpp>
#include <dcds/transaction/transaction-namespaces.hpp>
#include <dcds/util/logging.hpp>

void dcds::StorageLayer::initTables(const std::string& txn_namespace, const std::string& dsTableName) {
  auto& tableRegistry = dcds::storage::TableRegistry::getInstance();

  std::vector<dcds::storage::AttributeDef> columns;
  for (const auto& [name, attribute] : this->attributes) {
    int64_t sizeVar = 0;
    if (attribute->type == dcds::valueType::INTEGER)
      sizeVar = sizeof(int64_t);
    else if (attribute->type == dcds::valueType::RECORD_PTR)
      sizeVar = sizeof(void*);
    columns.emplace_back(name, attribute->type, sizeVar);
  }
  tableRegistry.createTable(dsTableName, columns);

  // LOG(INFO) << "Created table: " << dsTableName;
  // LOG(INFO) << "alignTable: " << alignof(dcds::storage::Table);
  // LOG(INFO) << "sizeTable: " << sizeof(dcds::storage::Table);
}

void dcds::StorageLayer::init(const std::string& txnNamespace) {
  // LOG(INFO) << "StorageLayer::init ns: " << txnNamespace;
  const std::string dsTableName = txnNamespace + "::" + "StorageLayer";

  // serializing initialization so that two datastructures do not initialize the tables together.
  static std::mutex initLock;
  std::unique_lock<std::mutex> lk(initLock);

  // create table if not exists within the namespace.
  // should initialize the mainRecord
  auto& tableRegistry = dcds::storage::TableRegistry::getInstance();
  txnManager = dcds::txn::NamespaceRegistry::getInstance().getOrCreate(txnNamespace);

  if (!tableRegistry.exists(dsTableName)) {
    dcds::StorageLayer::initTables(txnNamespace, dsTableName);
  }

  this->dsTable = tableRegistry.getTable(dsTableName);
  // LOG(INFO) << "DS Table: " << this->dsTable->name();

  auto txn = txnManager->beginTransaction(false);
  auto mainTable = tableRegistry.getTable(dsTableName);
  assert(!(mainRecord.valid()));

  // insert a default record for referencing the data structure
  std::vector<DSValueType> tmp;  // This might become tuple with data structures having attributes of multiple types.
  tmp.emplace_back(this->attributes.begin()->second->initVal);

  auto tmpMainRecord = mainTable->insertRecord(txn, &tmp);
  this->mainRecord = tmpMainRecord;
  assert(mainRecord.valid());
  // LOG(INFO) << "MainRecord created";

  txnManager->commitTransaction(txn);
  assert(mainRecord.valid());

  // LOG(INFO) << "Initialization done";
}

void dcds::StorageLayer::read(void* readVariable, uint64_t attributeIndex, void* txnPtr) {
  auto txn = reinterpret_cast<dcds::txn::txn_ptr_t*>(txnPtr);

  // Can there be a deadlock?
  mainRecord->readWithLatch(
      [&](dcds::storage::record_metadata_t* dsRc) { dsTable->getAttribute(*txn, dsRc, readVariable, attributeIndex); });
}

void dcds::StorageLayer::write(void* writeVariable, uint64_t attributeIndex, void* txnPtr) {
  auto txn = reinterpret_cast<dcds::txn::txn_ptr_t*>(txnPtr);

  // Can there be a deadlock?
  mainRecord->writeWithLatch([&](dcds::storage::record_metadata_t* dsRc) {
    dsTable->updateAttribute(*txn, dsRc, writeVariable, attributeIndex);
  });
}

void* dcds::StorageLayer::beginTxn() {
  //    return reinterpret_cast<void*>(txnManager->beginTransaction(false).get());
}

void dcds::StorageLayer::commitTxn(void* txn_) {
  //  auto txn = reinterpret_cast<dcds::txn::txn_ptr_t*>(txn_);
  //  txnManager->commitTransaction(*txn);
}
