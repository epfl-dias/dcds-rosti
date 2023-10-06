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

int printc(char* X) {
  printf("[printc:] Generated code -- char read: %c\n", X[0]);
  return 0;
}

int prints(char* X) {
  printf("[prints:] Generated code -- string read: %s\n", X);
  return 0;
}

void* getTableRegistry() {
  auto& tableRegistry = dcds::storage::TableRegistry::getInstance();
  return &tableRegistry;
}

uint doesTableExists(const char* table_name) {
  auto& tableRegistry = dcds::storage::TableRegistry::getInstance();
  return tableRegistry.exists(table_name);
}

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
  LOG(INFO) << "[createTablesInternal]: num_attributes: " << num_attributes;
  LOG(INFO) << "[createTablesInternal]: table_name: " << table_name;
  for (auto i = 0; i < num_attributes; i++) {
    LOG(INFO) << "[createTablesInternal]: attributeTypes["<<i<<"]: " << attributeTypes[i];
  }

  LOG(INFO) << reinterpret_cast<void*>(attributeNames);
  auto *x = reinterpret_cast<void*>(attributeNames);
  LOG(INFO) << reinterpret_cast<char*>(x);
  LOG(INFO) << reinterpret_cast<char*>(x)+5;

  // HACK: as we are getting pointer to first character of first attribute instead of ptr.
  auto *startPtr = reinterpret_cast<char*>(x);;
  std::vector<std::string> actual_attr_names;
  actual_attr_names.reserve(num_attributes);
  for (auto i = 0; i < num_attributes; i++) {
    auto sln = strlen(startPtr);
    LOG(INFO) << "[createTablesInternal]: attributeNames["<<i<<"]: " << startPtr;
    actual_attr_names.emplace_back(startPtr);
    startPtr += sln+1;
  }



  for (auto i = 0; i < num_attributes; i++) {
    auto type = attributeTypes[i];
    auto name = actual_attr_names[i];
    LOG(INFO) << "Loading attribute: " << name << " | type: " << type;

    int64_t sizeVar = 0;
    if (type == dcds::valueType::INTEGER)
      sizeVar = sizeof(int64_t);
    else if (type == dcds::valueType::RECORD_PTR)
      sizeVar = sizeof(void*);
    else
      assert(false && "unknown?");
    columns.emplace_back(name, type, sizeVar);
  }


  // CRITICAL SECTION: so that if two DS instances are getting initialized together,
  // we don't create the same table twice.
  dcds::storage::Table *ret_table_ptr = nullptr;
  {
    std::unique_lock<std::mutex> lk(create_table_m);
    if(tableRegistry.exists(table_name)){
      auto tableSharedPtr = tableRegistry.getTable(table_name);
      ret_table_ptr = tableSharedPtr.get();
    } else {
      auto createdTable = tableRegistry.createTable(table_name, columns);
      ret_table_ptr = createdTable.get();
    }

    assert(ret_table_ptr);
  }


  LOG(INFO) << "[createTablesInternal] DONE: " << ret_table_ptr;
  return ret_table_ptr;
}

void* getTxnManager(const char* txn_namespace) {
  auto x = dcds::txn::NamespaceRegistry::getInstance().getOrCreate(txn_namespace);
  LOG(INFO) << "[getTxnManager]: txn_namespace: " << txn_namespace;
  LOG(INFO) << "[getTxnManager]: returning: " << x.get();
  return x.get();
}

void* getTable(char* table_name) {
  auto& tableRegistry = dcds::storage::TableRegistry::getInstance();
  return tableRegistry.getTable(table_name).get();
}

void* beginTxn(void* txnManager) {
  LOG(INFO) << "beginTxn: args:" << txnManager;
  auto txnPtr = static_cast<dcds::txn::TransactionManager*>(txnManager)->beginTransaction(false);
  LOG(INFO) << "beginTxn ret: " << txnPtr;
  return txnPtr;
}

bool commitTxn(void* txnManager, void* txnPtr) {
  LOG(INFO) << "commitTxn: args:" << txnManager << " | " << txnPtr;
  auto x = static_cast<dcds::txn::TransactionManager*>(txnManager)->commitTransaction(static_cast<dcds::txn::Txn*>(txnPtr));
  LOG(INFO) << "commitTxn: " << x ;
  return x;
}

uintptr_t insertMainRecord(void* table, void* txn, void* data) {
  LOG(INFO) << "insertMainRecord: args:" << table << " | " << txn << " | " << data;

  struct __attribute__((packed)) test_st {
    uint8_t * next;
    uint64_t payload;
  };

  test_st *s = reinterpret_cast<test_st *>(data);

  LOG(INFO) << "value1: " << s->next;
  LOG(INFO) << "value2: " << s->payload;

  LOG(INFO) << "0.0: " << &(s->next);
  LOG(INFO) << "0: " << &(s->payload);
  LOG(INFO) << "1: " << reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(data)+sizeof(void*));


  auto x = static_cast<dcds::storage::Table*>(table)->insertRecord(static_cast<dcds::txn::Txn*>(txn), data);
  LOG(INFO) << "insertMainRecord: " << x.getBase();
  return x.getBase();
}