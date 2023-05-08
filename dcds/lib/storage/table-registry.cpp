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
  std::shared_lock<std::shared_mutex> lk(this->registry_lk);
  return tables.contains(tableId);
}
bool TableRegistry::exists(const std::string &name) {
  std::shared_lock<std::shared_mutex> lk(this->registry_lk);
  return table_name_map.contains(name);
}

std::shared_ptr<Table> TableRegistry::createTable(const std::string& name,
                                                  const std::vector<AttributeDef>& columns,
                                                  bool multi_version) {
  size_t record_size = 0;
  for (const auto &c : columns) {
    LOG(INFO) << "size: " << c.getName() << " - " << c.getSize();
    record_size += c.getSize();
  }
  std::unique_lock<std::shared_mutex> lk(this->registry_lk);
  assert(!table_name_map.contains(name) && "table already exists");

  auto tableId = table_id_generator.fetch_add(1);
  table_name_map.emplace(name, tableId);

  LOG(INFO) << "record size: " << record_size;

  if(multi_version){
    throw std::runtime_error("unimplemented MV");
  } else {
    auto tablePtr = std::make_shared<SingleVersionRowStore>(tableId, name, record_size, columns);
    tables.emplace(tableId, tablePtr);
    return tablePtr;
  }

}

std::shared_ptr<Table> TableRegistry::getTable(table_id_t tableId) {
  std::shared_lock<std::shared_mutex> lk(this->registry_lk);
  if (tables.contains(tableId)) {
    return tables.at(tableId);
  } else {
    return {};
  }
}
std::shared_ptr<Table> TableRegistry::getTable(const std::string &name) {
  std::shared_lock<std::shared_mutex> lk(this->registry_lk);
  if (table_name_map.contains(name)) {
    LOG(INFO) << "Table does exists " << name << " || " << tables.at(table_name_map.at(name))->name();
    return tables.at(table_name_map.at(name));
  } else {
    LOG(INFO) << "Table does not exists!: " << name;
    return {};
  }
}

}  // namespace dcds::storage
