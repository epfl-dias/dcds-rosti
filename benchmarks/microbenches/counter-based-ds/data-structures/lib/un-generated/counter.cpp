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

#include "un-generated/counter.hpp"

#include <benchmark/benchmark.h>

#include <dcds/util/logging.hpp>
#include <dcds/util/profiling.hpp>
#include <iostream>

#include "dcds/storage/table-registry.hpp"
#include "dcds/transaction/transaction-namespaces.hpp"

using namespace dcds;
using namespace dcds::storage;

void UnGeneratedCounter::initTables(const std::string& txn_namespace) {
  const std::string dsName = txn_namespace + "::" + "UnGeneratedCounter";
  auto& tableRegistry = TableRegistry::getInstance();

  // Table1: COUNTER_VALUE

  // In actual codegen/parser, this attribute def should throw out a struct which can be used by the application, maybe.
  std::vector<AttributeDef> table_1_columns;

  // const std::string &name, valueType dType, size_t width
  table_1_columns.emplace_back("value", valueType::INTEGER, sizeof(size_t));
  tableRegistry.createTable(dsName, table_1_columns);

  // LOG(INFO) << "Created table: " << dsName;
  // LOG(INFO) << "alignTable: " << alignof(Table);
  // LOG(INFO) << "sizeTable: " << sizeof(Table);
}

void UnGeneratedCounter::init(const std::string& txn_namespace) {
  //   LOG(INFO) << "UnGeneratedCounter::init ns: " << txn_namespace;
  //  const std::string dsName = txn_namespace + "::" + "UnGeneratedCounter";
  //  // serializing initialization so that two datastructures do not initialize the tables together.
  //  static std::mutex initLock;
  //  std::unique_lock<std::mutex> lk(initLock);

  //  // create table if not exists within the namespace.
  //  // should initialize the mainRecord
  //
  //  auto& tableRegistry = TableRegistry::getInstance();
  //  txnManager = txn::NamespaceRegistry::getInstance().getOrCreate(txn_namespace);
  //
  //  if (!(tableRegistry.exists(dsName))) {
  //    initTables(txn_namespace);
  //  }
  //
  //  this->counterTable = tableRegistry.getTable(txn_namespace + "::" + "UnGeneratedCounter");
  //   LOG(INFO) << "counterTable: " << counterTable->name();
  //
  //  auto txn = txnManager->beginTransaction(false);
  //  auto mainTable = tableRegistry.getTable(dsName);
  //
  //  struct counter_record_st tmp {
  //      this->initialCounterValue
  //  };
  //
  //  assert(!(mainRecord.valid()));
  //  // insert a default record for referencing the data structure
  //  auto tmpMainRecord = mainTable->insertRecord(txn, &tmp);
  //  this->mainRecord = tmpMainRecord;
  //  assert(mainRecord.valid());
  //   LOG(INFO) << "MainRecord created";
  //
  //  txnManager->commitTransaction(txn);
  //  assert(mainRecord.valid());
  //
  //   LOG(INFO) << "Initialization done";
}

counterValueType UnGeneratedCounter::read() {
  counterValueType counterValue;

  // Can there be a deadlock?
  mainRecord->readWithLatch([&](record_metadata_t* countRc) {
    auto* counterData = reinterpret_cast<struct counter_record_st*>(counterTable->getRecordData(countRc));
    counterValue = counterData->value;
  });
  // LOG(INFO) << "Counter Value read";
  return counterValue;
}

void UnGeneratedCounter::update() {
  // Can there be a deadlock?
  mainRecord->readWithLatch([&](record_metadata_t* countRc) {
    auto* counterData = reinterpret_cast<struct counter_record_st*>(counterTable->getRecordData(countRc));
    counterData->value = counterData->value + counterStep;
  });
  // LOG(INFO) << "Counter value updated";
}

void UnGeneratedCounter::write(counterValueType writeVariable) {
  // Can there be a deadlock?
  mainRecord->readWithLatch([&](record_metadata_t* countRc) {
    auto* counterData = reinterpret_cast<struct counter_record_st*>(counterTable->getRecordData(countRc));
    counterData->value = writeVariable;
  });
  // LOG(INFO) << "Counter value updated";
}
