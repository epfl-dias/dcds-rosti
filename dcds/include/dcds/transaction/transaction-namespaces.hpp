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

#ifndef DCDS_TRANSACTION_NAMESPACES_HPP
#define DCDS_TRANSACTION_NAMESPACES_HPP

#include <deque>
#include <mutex>
#include <unordered_map>
#include <shared_mutex>

#include "dcds/common/common.hpp"
#include "dcds/common/exceptions/storage-exceptions.hpp"
#include "dcds/transaction/transaction-manager.hpp"

namespace dcds::txn {

class NamespaceRegistry {
 public:
  // Singleton
  static inline NamespaceRegistry &getInstance() {
    static NamespaceRegistry instance;
    return instance;
  }

  NamespaceRegistry(NamespaceRegistry &&) = delete;
  NamespaceRegistry &operator=(NamespaceRegistry &&) = delete;
  NamespaceRegistry(const NamespaceRegistry &) = delete;
  NamespaceRegistry &operator=(const NamespaceRegistry &) = delete;

 private:
  NamespaceRegistry() {
    std::unique_lock<std::shared_mutex> lk(registry_lk);

    // create a default namespace.

    auto defaultNS = std::make_shared<TransactionManager>("default");
    namespaces.emplace_back(defaultNS);
    namespace_map.emplace("default", defaultNS);
    default_namespace = defaultNS;
  }

  ~NamespaceRegistry() {
    std::unique_lock<std::shared_mutex> lk(registry_lk);

    namespaces.clear();
    namespace_map.clear();
    default_namespace.reset();
  }

 public:

  auto getDefaultNamespace(){
    return default_namespace;
  }

  auto get(const std::string& key) {
    std::shared_lock<std::shared_mutex> lk(registry_lk);
    if (namespace_map.contains(key)) {
      return namespace_map.at(key);
    } else {
      throw dcds::exceptions::namespace_not_found();
    }
  }

  auto getOrCreate(const std::string& key){
    std::unique_lock<std::shared_mutex> lk(registry_lk);
    if (!namespace_map.contains(key)) {
      auto ns = std::make_shared<TransactionManager>(key);
      namespaces.emplace_back(ns);
      namespace_map.emplace(key, ns);
      return ns;
    } else {
      return namespace_map.at(key);
    }
  }

  auto create(const std::string& key) {
    std::unique_lock<std::shared_mutex> lk(registry_lk);

    if (namespace_map.contains(key)) {
      throw dcds::exceptions::duplicate_namespace_key();
    }

    auto ns = std::make_shared<TransactionManager>(key);
    namespaces.emplace_back(ns);
    namespace_map.emplace(key, ns);

    return ns;
  }

  auto remove(std::string){
    throw std::runtime_error("unimplemented");
  }

 private:
  std::deque<std::shared_ptr<TransactionManager>> namespaces;
  std::unordered_map<std::string, std::shared_ptr<TransactionManager>> namespace_map;
  std::shared_ptr<TransactionManager> default_namespace;

  std::shared_mutex registry_lk;
};

}  // namespace dcds::txn

#endif  // DCDS_TRANSACTION_NAMESPACES_HPP
