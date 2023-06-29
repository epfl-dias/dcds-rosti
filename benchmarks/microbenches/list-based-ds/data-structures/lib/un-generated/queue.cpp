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

#include "un-generated/queue.hpp"

#include <benchmark/benchmark.h>

#include <dcds/util/logging.hpp>
#include <dcds/util/profiling.hpp>
#include <iostream>

#include "dcds/storage/table-registry.hpp"
#include "dcds/transaction/transaction-namespaces.hpp"

using namespace dcds;
using namespace dcds::storage;

void UnGeneratedQueue::initTables(const std::string& txn_namespace) {
  const std::string dsName = txn_namespace + "::" + "UnGeneratedQueue";
  const std::string dsName_node = txn_namespace + "::" + "UnGeneratedQueue_node";

  auto& tableRegistry = TableRegistry::getInstance();

  // Table1: RECORD_PTR, RECORD_PTR, INTEGER
  // Table2: RECORD_PTR, PAYLOAD

  // In actual codegen/parser, this attribute def should throw out a struct which can be used by the application, maybe.
  std::vector<AttributeDef> table_1_columns;
  std::vector<AttributeDef> table_2_columns;

  // const std::string &name, valueType dType, size_t width
  table_1_columns.emplace_back("head", valueType::RECORD_PTR, sizeof(uintptr_t));
  table_1_columns.emplace_back("tail", valueType::RECORD_PTR, sizeof(uintptr_t));
  table_1_columns.emplace_back("size", valueType::INTEGER, sizeof(size_t));

  // const std::string &name, valueType dType, size_t width
  table_2_columns.emplace_back("next", valueType::RECORD_PTR, sizeof(uintptr_t));
  table_2_columns.emplace_back("payload", valueType::INTEGER, sizeof(size_t));

  tableRegistry.createTable(dsName, table_1_columns);
  tableRegistry.createTable(dsName_node, table_2_columns);

  LOG(INFO) << "Created table: " << dsName;
  LOG(INFO) << "Created table: " << dsName_node;

  LOG(INFO) << "alignTable: " << alignof(Table);
  LOG(INFO) << "sizeTable: " << sizeof(Table);
}

void UnGeneratedQueue::init(const std::string& txn_namespace) {
  LOG(INFO) << "UnGeneratedQueue::init ns: " << txn_namespace;
  const std::string dsName = txn_namespace + "::" + "UnGeneratedQueue";
  // serializing initialization so that two datastructures does not initialize the tables together.
  static std::mutex initLock;
  std::unique_lock<std::mutex> lk(initLock);

  // create table if not exists within the namespace.
  // should initialize the mainRecord

  auto& tableRegistry = TableRegistry::getInstance();
  txnManager = txn::NamespaceRegistry::getInstance().getOrCreate(txn_namespace);

  if (!(tableRegistry.exists(dsName))) {
    initTables(txn_namespace);
  }

  this->listTable = tableRegistry.getTable(txn_namespace + "::" + "UnGeneratedQueue");
  this->listNodeTable = tableRegistry.getTable(txn_namespace + "::" + "UnGeneratedQueue_node");
  LOG(INFO) << "listTable: " << listTable->name();
  LOG(INFO) << "listNodeTable: " << listNodeTable->name();

  auto txn = txnManager->beginTransaction(false);
  auto mainTable = tableRegistry.getTable(dsName);

  struct list_record_st tmp {};

  assert(!(mainRecord.valid()));
  // insert a default record for referencing the data structure
  auto tmpMainRecord = mainTable->insertRecord(txn, &tmp);
  this->mainRecord = tmpMainRecord;
  assert(mainRecord.valid());
  // this->mainRecord = mainTable->insertRecord(txn, &tmp);
  LOG(INFO) << "MainRecord created";

  //---
  // LOG(INFO) << "sizeof(dcds::storage::record_reference_t ): " << sizeof(dcds::storage::record_reference_t );
  //  mainRecord->withLatch([&](record_metadata_t* rc){
  //    LOG(INFO) << "Latched mainRecord";
  //
  //    auto head = listTable->getRefTypeData(rc, 0);
  //    LOG(INFO) << " head isValid: " << head.valid();
  //    auto tail = listTable->getRefTypeData(rc, 1);
  //    LOG(INFO) << "got ref to tail";
  //    if(tail.valid()){
  //      LOG(INFO) << "Tail Valid";
  //      tail.print();
  //    } else {
  //      LOG(INFO) << "Tail Invalid";
  //    }
  //
  //  });
  //---

  txnManager->commitTransaction(txn);

  assert(mainRecord.valid());

  LOG(INFO) << "Initialization done";
  //  assert(mainRecord());
}

// pop from the front & returns value
size_t UnGeneratedQueue::popAndReturn() {}

// pop from the front
void UnGeneratedQueue::pop() {}

// insert at the end
void UnGeneratedQueue::push(queueValueType val) {
  auto txn = txnManager->beginTransaction(false);

  // Insert a new node
  struct node_record_st nodeTmp {};
  nodeTmp.payload = val;
  nodeTmp.next = listNodeTable->getNullReference();

  auto newNode = listNodeTable->insertRecord(txn, &nodeTmp);

  LOG(INFO) << "Inserted new node: " << newNode.valid();
  // FIXME: how does 2PL works here? we need to acquire the locks!

  // insert at the end
  // if(tail) tail->next = newNode
  // tail = newNode
  // size++;

  mainRecord->withLatch([&](record_metadata_t* rc) {
    LOG(INFO) << "Latched mainRecord";
    //    auto* listRecord = reinterpret_cast<struct list_record_st*>(listTable->getRecordData(rc));
    //    LOG(INFO) << "sizeBeforeIns: " << listRecord->size;
    size_t listSize = 0;
    listTable->getAttribute(txn, rc, &listSize, 2);
    LOG(INFO) << "sizeBeforeIns: " << listSize;

    if (listSize) {
      LOG(INFO) << "Tail isValid -> update existing tail";
      auto tail = listTable->getRefTypeData(rc, 1);
      assert(tail.valid());
      tail->readWithLatch(
          [&](record_metadata_t* tailNodeRc) { listNodeTable->updateRefTypeData(txn, tailNodeRc, newNode, 0); });
    }

    // tail = newNode
    LOG(INFO) << "updating tail ref";
    listTable->updateRefTypeData(txn, rc, newNode, 1);

    // updateSize!
    listSize++;
    listTable->updateAttribute(txn, rc, &listSize, 2);
  });

  txnManager->commitTransaction(txn);
  LOG(INFO) << "PushDone";
}

queueValueType UnGeneratedQueue::front() {
  queueValueType ret;
  mainRecord->readWithLatch([&](record_metadata_t* rc) {
    auto head = listTable->getRefTypeData(rc, 0);

    // Can there be a deadlock?
    head->readWithLatch([&](record_metadata_t* nodeRc) {
      auto* nodeData = reinterpret_cast<struct node_record_st*>(listNodeTable->getRecordData(nodeRc));
      ret = nodeData->payload;
    });
  });

  return ret;
}
queueValueType UnGeneratedQueue::back() {
  queueValueType ret;
  mainRecord->readWithLatch([&](record_metadata_t* rc) {
    auto tail = listTable->getRefTypeData(rc, 1);

    // Can there be a deadlock?
    tail->readWithLatch([&](record_metadata_t* nodeRc) {
      auto* nodeData = reinterpret_cast<struct node_record_st*>(listNodeTable->getRecordData(nodeRc));
      ret = nodeData->payload;
    });
  });

  return ret;
}

size_t UnGeneratedQueue::size() {
  // query the table -> mainRecord in transactional manner
  // FIXME: we do not need to access the record through table interface, it should be directly readable.
  //  maybe create a static interface in table, where one can pass in the recordRef and Txn. or w/o txn.

  size_t ret = 0;
  mainRecord->readWithLatch([&](record_metadata_t* rc) {
    auto* data = reinterpret_cast<struct list_record_st*>(listTable->getRecordData(rc));
    ret = data->size;
  });

  return ret;
}

void UnGeneratedQueue::printQueue() {
  LOG(INFO) << "[printQueue] begin";
  mainRecord->readWithLatch([&](record_metadata_t* rc) {
    auto* data = reinterpret_cast<struct list_record_st*>(listTable->getRecordData(rc));
    LOG(INFO) << "[printQueue] Size of list: " << data->size;

    auto tmp = data->head;
    for (size_t i = 0; i < data->size; i++) {
      if (tmp.valid()) {
        tmp->readWithLatch([&](record_metadata_t* nodeRc) {
          auto* nodeData = reinterpret_cast<struct node_record_st*>(listNodeTable->getRecordData(nodeRc));
          LOG(INFO) << "[printQueue] i:" << i << " | value: " << nodeData->payload;
          tmp = nodeData->next;
        });
      }
    }
  });
  LOG(INFO) << "[printQueue] end";
}
