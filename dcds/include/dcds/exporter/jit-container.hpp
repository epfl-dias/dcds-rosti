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

#ifndef DCDS_JIT_CONTAINER_HPP
#define DCDS_JIT_CONTAINER_HPP


#include <dcds/storage/table-registry.hpp>

class JitContainer{


 private:
  struct dcds_jit_container_t {
    // txnManager, table, mainRecord
    dcds::txn::TransactionManager *txnManager;
    dcds::storage::Table *storageTable;
    uintptr_t mainRecord; // can it be directly record_reference_t?
  };

 private:
  dcds_jit_container_t * _container;
};

#endif  // DCDS_JIT_CONTAINER_HPP
