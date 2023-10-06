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

#include "dcds/transaction/transaction-namespaces.hpp"
#include "dcds/transaction/transaction-manager.hpp"
#include "dcds/storage/table-registry.hpp"

// NOTE: make sure to register the function in llvm-context.cpp

extern "C" void *getTableRegistry();
extern "C" void* getTable(char* table_name);
extern "C" uint doesTableExists(const char* table_name);
extern "C" void *createTablesInternal(char *table_name, dcds::valueType attributeTypes[],
                                      char *attributeNames[], int num_attributes);

extern "C" void *c1(char *table_name);
extern "C" void *c2(int num_attributes);
extern "C" void *c3(const dcds::valueType attributeTypes[]);
extern "C" void *c4(char *attributeNames);


extern "C" void *getTxnManager(const char* txn_namespace = "default_namespace");
extern "C" void *beginTxn(void *txnManager);
extern "C" bool commitTxn(void *txnManager, void* txnPtr);

extern "C" uintptr_t insertMainRecord(void* table, void* txn, void* data);


extern "C" int printc(char *X);
extern "C" int prints(char *X);


#endif  // DCDS_FUNCTIONS_HPP
