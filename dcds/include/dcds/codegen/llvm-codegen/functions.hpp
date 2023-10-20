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

#ifndef DCDS_FUNCTIONS_HPP
#define DCDS_FUNCTIONS_HPP

#include <iostream>

#include "dcds/common/types.hpp"

// NOTE: make sure to register the function in llvm-context.cpp

// TODO: make always inline when registering.
extern "C" void* getTableRegistry();
// TODO: make always inline when registering.
extern "C" void* getTable(const char* table_name);
// TODO: make always inline when registering.
extern "C" uint doesTableExists(const char* table_name);
extern "C" void* createTablesInternal(char* table_name, dcds::valueType attributeTypes[], char* attributeNames[],
                                      int num_attributes);

extern "C" void* c1(char* table_name);
extern "C" void* c2(int num_attributes);
extern "C" void* c3(const dcds::valueType attributeTypes[]);
extern "C" void* c4(char* attributeNames);

extern "C" void* getTxnManager(const char* txn_namespace = "default_namespace");
extern "C" void* beginTxn(void* txnManager, bool isReadOnly);
extern "C" bool commitTxn(void* txnManager, void* txnPtr);

extern "C" uintptr_t insertMainRecord(void* table, void* txn, void* data);

extern "C" int printc(char* X);
extern "C" int prints(char* X);
extern "C" void printPtr(void* X);
extern "C" void printUInt64(uint64_t X);

// TODO: make always inline when registering.
extern "C" void* createDsContainer(void* txnManager, uintptr_t data);
extern "C" uintptr_t extractRecordFromDsContainer(void* container);

extern "C" void table_read_attribute(void* _txnManager, uintptr_t _mainRecord, void* txn, void* dst,
                                     size_t attributeIdx);
extern "C" void table_write_attribute(void* _txnManager, uintptr_t _mainRecord, void* txnPtr, void* src,
                                      uint attributeIdx);

#endif  // DCDS_FUNCTIONS_HPP
