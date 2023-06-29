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

#include <map>
#include <mutex>
#include <shared_mutex>

#include "dcds/common/common.hpp"
#include "dcds/storage/table.hpp"
#include "dcds/util/singleton.hpp"

// do we need schema if the reference would be returned back?

namespace dcds::storage {

class TableRegistry : public dcds::Singleton<TableRegistry> {
  friend class dcds::Singleton<TableRegistry>;

 public:
  bool exists(table_id_t tableId);
  bool exists(const std::string& name);

  //  std::shared_ptr<Table> create_table(
  //      const std::string &name, layout_type layout, const TableDef &columns,
  //      uint64_t initial_num_records = 10000000, bool indexed = true,
  //      bool partitioned = true, int numa_idx = -1,
  //      size_t max_partition_size = 0);

  // Do we need create table or register table?
  // or what about having single vs multiple txnManager? we store it in schema or table class a reference to what txnMgr
  // to use.

  std::shared_ptr<Table> getTable(table_id_t tableId);
  std::shared_ptr<Table> getTable(const std::string& name);

  //  void registerTable();
  //  void unregisterTable();

  std::shared_ptr<Table> createTable(const std::string& name, const std::vector<AttributeDef>& columns,
                                     bool multi_version = false);
  void dropTable();  // how to drop if it is a sharedPtr, someone might be holding reference to it?

 private:
  std::shared_mutex registry_lk;
  std::map<table_id_t, std::shared_ptr<Table>> tables{};
  std::map<std::string, table_id_t> table_name_map{};

  std::atomic<table_id_t> table_id_generator;

 private:
  ~TableRegistry() {
    // also clear up the tables if remaining.
    LOG(INFO) << "TableRegistry::~TableRegistry()";
  }
  TableRegistry() = default;
};

}  // namespace dcds::storage

#endif  // DCDS_TABLE_REGISTRY_HPP
