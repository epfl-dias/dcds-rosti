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

#include "dcds/codegen/codegen.hpp"
#include "dcds/codegen/llvm-codegen/functions.hpp"
#include "dcds/storage/table-registry.hpp"
#include "dcds/transaction/transaction.hpp"

namespace dcds {

class Builder;

class JitContainer {
 private:
  static void *create(void *txnManager, void *storageTable, uintptr_t mainRecord) {
    auto *ct = new JitContainer(reinterpret_cast<dcds::txn::TransactionManager *>(txnManager),
                                reinterpret_cast<dcds::storage::Table *>(storageTable), mainRecord);
    return ct;
  }

  static void *create(dcds::txn::TransactionManager *txnManager, dcds::storage::Table *storageTable,
                      uintptr_t mainRecord) {
    auto *ct = new JitContainer(txnManager, storageTable, mainRecord);
    return ct;
  }

  JitContainer(dcds::txn::TransactionManager *txnManager, dcds::storage::Table *storageTable, uintptr_t mainRecord) {
    _container = new dcds_jit_container_t();
    _container->txnManager = txnManager;
    _container->storageTable = storageTable;
    _container->mainRecord = mainRecord;
    LOG(INFO) << "TxnManager1: " << reinterpret_cast<void *>(this->_container->txnManager);
    LOG(INFO) << "storageTable: " << reinterpret_cast<void *>(this->_container->storageTable);
    LOG(INFO) << "mainRecord: " << this->_container->mainRecord;
  }

  void setCodegenEngine(std::shared_ptr<Codegen> codegenEngine) { codegen_engine = codegenEngine; }

 public:
  //  template <typename R, typename... Args>
  //  llvm::Value *gen_call(R (*func)(Args...), std::initializer_list<llvm::Value *> args) {
  //    return gen_call(func, args, toLLVM<std::remove_cv_t<R>>());
  //  }

  template <typename... Args>
  std::any op(const std::string &op_name, Args... args) {
    LOG(INFO) << "OP CALLED: " << op_name;
    auto fn = codegen_engine->getAvailableFunctions()[op_name];
    // FIXME: verify if the number of args is same as the args required for the function.
    switch (fn->returnType) {
      case INTEGER:
      case RECORD_PTR: {
        LOG(INFO) << "here";

        auto ret = reinterpret_cast<uint64_t (*)(void *, void *, uintptr_t, ...)>(const_cast<void *>(fn->address))(
            _container->txnManager, _container->storageTable, _container->mainRecord, std::forward<Args>(args)...);
        LOG(INFO) << "RET: " << ret;
        return {ret};
      }
      case VOID: {
        reinterpret_cast<uint64_t (*)(void *, void *, uintptr_t, ...)>(const_cast<void *>(fn->address))(
            _container->txnManager, _container->storageTable, _container->mainRecord, std::forward<Args>(args)...);
        LOG(INFO) << "RET VOID: ";
        return {};
      }
      case FLOAT:
      case RECORD_ID:
      case CHAR:
      default:
        assert(false);
        break;
    }
    assert(false && "how come here?");
  }

  void listAllAvailableFunctions() {
    auto functions = codegen_engine->getAvailableFunctions();
    for (auto &f : functions) {
      LOG(INFO) << f.first;
    }
  }

 private:
  struct dcds_jit_container_t {
    // txnManager, table, mainRecord
    dcds::txn::TransactionManager *txnManager;
    dcds::storage::Table *storageTable;
    uintptr_t mainRecord;  // can it be directly record_reference_t?
  };

 private:
  dcds_jit_container_t *_container;
  std::shared_ptr<Codegen> codegen_engine;

  friend class Builder;
  friend void * ::createDsContainer(void *, void *, uintptr_t);
};

}  // namespace dcds

#endif  // DCDS_JIT_CONTAINER_HPP
