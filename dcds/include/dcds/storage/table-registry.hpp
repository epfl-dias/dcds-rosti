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

#ifndef DCDS_TABLE_REGISTRY_HPP
#define DCDS_TABLE_REGISTRY_HPP

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/StringMap.h>

#include <mutex>
#include <shared_mutex>

#include "dcds/common/common.hpp"
#include "dcds/storage/table.hpp"
#include "dcds/util/singleton.hpp"

namespace dcds::storage {

class TableRegistry : public dcds::Singleton<TableRegistry> {
  friend class dcds::Singleton<TableRegistry>;

 public:
  bool exists(table_id_t tableId);
  bool exists(const std::string& name);

  Table* getTable(table_id_t tableId);
  Table* getTable(const std::string& name);

  //  void registerTable();
  //  void unregisterTable();

  Table* createTable(const std::string& name, const std::vector<AttributeDef>& columns, bool multi_version = false);
  void dropTable();  // how to drop if it is a sharedPtr, someone might be holding reference to it?

 private:
  oneapi::tbb::rw_mutex registry_lk{};
  llvm::DenseMap<table_id_t, Table*> tables;

  llvm::StringMap<table_id_t> table_name_map;
  alignas(64) std::atomic<table_id_t> table_id_generator;

 private:
  ~TableRegistry() {
    // also clear up the tables if remaining.
    LOG(INFO) << "TableRegistry::~TableRegistry()";
  }
  TableRegistry() = default;
};

}  // namespace dcds::storage

#endif  // DCDS_TABLE_REGISTRY_HPP
