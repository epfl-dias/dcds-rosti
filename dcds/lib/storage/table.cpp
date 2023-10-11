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

#include "dcds/storage/table.hpp"

#include <utility>

#include "dcds/storage/table-registry.hpp"

using namespace dcds::storage;

std::shared_ptr<Table> RecordRefV2::getTable() {
  return TableRegistry::getInstance().getTable(static_cast<table_id_t>(record_metadata_ptr.getData()));
}

Table::Table(table_id_t tableId, std::string tableName, size_t recordSize, std::vector<AttributeDef> attributes,
             bool is_multi_versioned)
    : table_id(tableId),
      table_name(std::move(tableName)),
      record_size(recordSize + sizeof(record_metadata_t)),
      record_size_data_only(recordSize),
      columns(std::move(attributes)),
      is_multiversion(is_multi_versioned) {
  // LOG(INFO) << "Table(): " << table_name;
  // LOG(INFO) << "sizeof(record_metadata_t): " << sizeof(record_metadata_t);

  size_t rec_size = 0;
  size_t col_offset = 0;

  for (const auto& a : columns) {
    auto col_width = a.getSize();

    column_size.push_back(col_width);
    column_size_offsets.push_back(col_offset);
    column_size_offset_pairs.emplace_back(col_width, col_offset);

    col_offset += col_width;
    rec_size += col_width;
  }

  assert(recordSize == rec_size);
}

void* SingleVersionRowStore::allocateRecordMemory() const {
  // TODO: use some sort of caching or allocate more and then return from the allocations.
  // LOG(INFO) << "allocateRecordMemory(): " << record_size << " | actualSize: " << record_size_data_only;
  return malloc(record_size);
}
void SingleVersionRowStore::freeRecordMemory(void* mem) { free(mem); }

record_reference_t SingleVersionRowStore::insertRecord(const txn::Txn& txn, const void* data) {
  //  return this->insertRecord(&txn, data);
  return {};
}

record_reference_t SingleVersionRowStore::insertRecord(dcds::txn::Txn* txn, const void* data) {
  assert(txn);
  assert(txn->read_only == false && "RO txn inserting ???");
  void* mem = allocateRecordMemory();
  auto* meta = new (mem) record_metadata_t(txn->txnTs.start_time);
  void* dataMemory = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(mem) + sizeof(record_metadata_t));

  memcpy(dataMemory, data, record_size_data_only);

  return record_reference_t{this->table_id, meta};
}

SingleVersionRowStore::SingleVersionRowStore(table_id_t tableId, const std::string& table_name, size_t recordSize,
                                             std::vector<AttributeDef> attributes)
    : Table(tableId, table_name, recordSize, std::move(attributes), false) {}

void SingleVersionRowStore::updateAttribute(txn::Txn& txn, record_metadata_t* rc, void* value, uint attribute_idx) {
  auto colWidthOffset = column_size_offset_pairs.at(attribute_idx);
  auto data_ptr =
      reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(rc) + sizeof(record_metadata_t) + colWidthOffset.second);
  memcpy(data_ptr, value, colWidthOffset.first);
}

record_reference_t SingleVersionRowStore::getRefTypeData(record_metadata_t* rc, uint attribute_idx) {
  auto data_ptr = reinterpret_cast<uintptr_t>(rc) + sizeof(record_metadata_t);
  data_ptr = data_ptr + column_size_offsets[attribute_idx];  // actual data_ptr

  return record_reference_t{*(reinterpret_cast<record_reference_t*>(data_ptr))};
}

void SingleVersionRowStore::updateRefTypeData(txn::Txn&, record_metadata_t* rc, record_reference_t& value,
                                              uint attribute_idx) {
  auto data_ptr = reinterpret_cast<record_reference_t*>(reinterpret_cast<uintptr_t>(rc) + sizeof(record_metadata_t) +
                                                        column_size_offsets[attribute_idx]);
  *data_ptr = value;
}

void SingleVersionRowStore::getData(txn::Txn&, record_metadata_t* rc, void* dst, size_t offset, size_t len) {
  assert(offset + len < record_size);
  auto data_ptr =
      reinterpret_cast<record_reference_t*>(reinterpret_cast<uintptr_t>(rc) + sizeof(record_metadata_t) + offset);
  memcpy(dst, data_ptr, len);
}
void SingleVersionRowStore::getAttribute(txn::Txn&, record_metadata_t* rc, void* dst, uint attribute_idx) {
  auto colWidthOffset = column_size_offset_pairs.at(attribute_idx);
  auto data_ptr = reinterpret_cast<record_reference_t*>(reinterpret_cast<uintptr_t>(rc) + sizeof(record_metadata_t) +
                                                        colWidthOffset.second);
  memcpy(dst, data_ptr, colWidthOffset.first);
}