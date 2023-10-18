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

#include <utility>

#include "dcds/codegen/codegen.hpp"
#include "dcds/codegen/llvm-codegen/functions.hpp"
#include "dcds/storage/table-registry.hpp"
#include "dcds/transaction/transaction.hpp"

namespace dcds {

class Builder;

class JitContainer {
 public:
  struct dcds_jit_container_t {
    // txnManager, mainRecord
    dcds::txn::TransactionManager *txnManager;
    uintptr_t mainRecord;  // can it be directly record_reference_t?
  };

 private:
  explicit JitContainer(dcds_jit_container_t *container) : _container(container) {}

  explicit JitContainer(dcds::txn::TransactionManager *txnManager, uintptr_t mainRecord) {
    _container = new dcds_jit_container_t();
    _container->txnManager = txnManager;
    _container->mainRecord = mainRecord;
  }

  void setCodegenEngine(std::shared_ptr<Codegen> codegenEngine) { codegen_engine = std::move(codegenEngine); }

 public:
  template <typename... Args>
  std::any op(const std::string &op_name, Args... args) {
    LOG(INFO) << "OP CALLED: " << op_name;
    auto fn = codegen_engine->getAvailableFunctions()[op_name];
    // FIXME: verify if the number of args is same as the args required for the function.
    switch (fn->returnType) {
      case INT64:
      case RECORD_PTR: {
        auto ret = reinterpret_cast<uint64_t (*)(void *, uintptr_t, ...)>(const_cast<void *>(fn->address))(
            _container->txnManager, _container->mainRecord, std::forward<Args>(args)...);
        LOG(INFO) << "RET: " << ret;
        return {ret};
      }
      case INT32: {
        auto ret = reinterpret_cast<uint32_t (*)(void *, uintptr_t, ...)>(const_cast<void *>(fn->address))(
            _container->txnManager, _container->mainRecord, std::forward<Args>(args)...);
        LOG(INFO) << "RET INT32: " << ret;
        return {ret};
      }
      case VOID: {
        reinterpret_cast<uint64_t (*)(void *, uintptr_t, ...)>(const_cast<void *>(fn->address))(
            _container->txnManager, _container->mainRecord, std::forward<Args>(args)...);
        LOG(INFO) << "RET VOID: ";
        return {};
      }
      case BOOL: {
        auto ret = reinterpret_cast<bool (*)(void *, uintptr_t, ...)>(const_cast<void *>(fn->address))(
            _container->txnManager, _container->mainRecord, std::forward<Args>(args)...);
        LOG(INFO) << "RET BOOL: " << ret;
        return {ret};
      }
      case DOUBLE: {
        auto ret = reinterpret_cast<double (*)(void *, uintptr_t, ...)>(const_cast<void *>(fn->address))(
            _container->txnManager, _container->mainRecord, std::forward<Args>(args)...);
        LOG(INFO) << "RET DOUBLE: " << ret;
        return {ret};
      }
      case FLOAT: {
        auto ret = reinterpret_cast<float (*)(void *, uintptr_t, ...)>(const_cast<void *>(fn->address))(
            _container->txnManager, _container->mainRecord, std::forward<Args>(args)...);
        LOG(INFO) << "RET FLOAT: " << ret;
        return {ret};
      }
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
  dcds_jit_container_t *_container;
  std::shared_ptr<Codegen> codegen_engine;

  friend class Builder;
  friend void * ::createDsContainer(void *, uintptr_t);
};

}  // namespace dcds

#endif  // DCDS_JIT_CONTAINER_HPP
