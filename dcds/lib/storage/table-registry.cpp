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

#include "dcds/storage/table-registry.hpp"

namespace dcds::storage {

bool TableRegistry::exists(table_id_t tableId) {
  std::shared_lock lk(this->registry_lk);

  return (tables.lookup(tableId) != nullptr);  // llvm::DenseMap
  // return tables.contains(tableId);          // std::map
}
bool TableRegistry::exists(const std::string &name) {
  std::shared_lock lk(this->registry_lk);
  return (table_name_map.find(name) != table_name_map.end());
}

Table *TableRegistry::createTable(const std::string &name, const std::vector<AttributeDef> &columns,
                                  bool multi_version) {
  size_t record_size = 0;
  for (const auto &c : columns) {
    record_size += c.getSize();
  }

  std::unique_lock lk(this->registry_lk);
  assert((table_name_map.find(name) == table_name_map.end()) && "table already exists");

  auto tableId = table_id_generator.fetch_add(1);
  table_name_map.try_emplace(name, tableId);

  if (multi_version) {
    throw std::runtime_error("unimplemented MV");
  } else {
    auto tablePtr = new SingleVersionRowStore(tableId, name, record_size, columns);
    // assert(tables.insert(tableId, tablePtr)); // cuckoo::map
    // tables.emplace(tableId, tablePtr); // std::map
    tables.try_emplace(tableId, tablePtr);  // llvm::DenseMap
    return tablePtr;
  }
}

void TableRegistry::clear() {
  std::unique_lock lk(this->registry_lk);
  for (auto &t : this->tables) {
    // t.second->~Table();
    delete t.second;
  }
  this->tables.clear();
  this->table_name_map.clear();
}

Table *TableRegistry::getTable(table_id_t tableId) {
  //  return tables.find(tableId); // cuckoo::map

  std::shared_lock lk(this->registry_lk);

  // std::map
  //    if (tables.contains(tableId)) {
  //      return tables.at(tableId);
  //    } else {
  //      return {};
  //    }

  // llvm::DenseMap
  return tables.lookup(tableId);
}
Table *TableRegistry::getTable(const std::string &name) {
  //  if (table_name_map.contains(name)) {
  //    return tables.find(table_name_map.find(name));
  //  }
  //  return {};

  std::shared_lock lk(this->registry_lk);

  if (auto iter = table_name_map.find(name); likely(iter != table_name_map.end())) {
    return tables.lookup(iter->second);
  }
  return {};
}

}  // namespace dcds::storage
