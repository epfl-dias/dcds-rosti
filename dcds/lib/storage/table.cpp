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
#include "dcds/util/logging.hpp"
#include "llvm/ADT/DenseMap.h"

using namespace dcds::storage;

Table* RecordReference::getTable() {
  static thread_local auto* registry = &TableRegistry::getInstance();
  auto tableId = record_metadata_ptr.getData();

  // No-cache
  //    return registry->getTable(static_cast<table_id_t>(tableId));

  // std::unordered_map cache
  //  static thread_local std::unordered_map<table_id_t, Table*> cache_map;
  //  if ((cache_map.contains(tableId))) {
  //    return cache_map.at(tableId);
  //  } else {
  //    auto tablePtr = registry->getTable(static_cast<table_id_t>(tableId));
  //    cache_map.emplace(tableId, tablePtr);
  //    return tablePtr;
  //  }

  // llvm::SmallDenseMap cache --> Best-so-far
  static thread_local llvm::SmallDenseMap<table_id_t, Table*> cache_map;
  auto ret = cache_map.lookup(tableId);
  if (unlikely(ret == nullptr)) {
    auto tablePtr = registry->getTable(static_cast<table_id_t>(tableId));
    cache_map.try_emplace(tableId, tablePtr);
    return tablePtr;
  } else {
    return ret;
  }
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

SingleVersionRowStore::SingleVersionRowStore(table_id_t tableId, const std::string& table_name, size_t recordSize,
                                             std::vector<AttributeDef> attributes)
    : Table(tableId, table_name, recordSize, std::move(attributes), false) {}

SingleVersionRowStore::~SingleVersionRowStore() {
  allocation_lock.acquire();
  for (auto& m : memory_allocations) {
    free(m);
  }
  memory_allocations.clear();
  allocation_lock.release();
}

void* SingleVersionRowStore::allocateRecordMemory(size_t n_records) {
  // TODO: use some sort of caching or allocate more and then return from the allocations.
  // FIXME: what about alignments?
  void* mem = malloc(record_size * n_records);
  {
    allocation_lock.acquire();
    memory_allocations.insert(mem);
    allocation_lock.release();
  }
  return mem;
}
void SingleVersionRowStore::freeRecordMemory(void* mem) {
  {
    allocation_lock.acquire();
    memory_allocations.erase(mem);
    allocation_lock.release();
  }
  free(mem);
}

record_reference_t SingleVersionRowStore::insertRecord(dcds::txn::Txn* txn, const void* data) {
  // txn can be null as we generate single-threaded DS also.
  CHECK(!txn || (txn && !txn->read_only)) << "RO txn inserting ???";

  void* mem = allocateRecordMemory();
  //  auto* meta = new (mem) record_metadata_t(txn ? txn->txnTs.start_time : 0);
  auto* meta = new (mem) record_metadata_t(0);
  void* dataMemory = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(mem) + sizeof(record_metadata_t));

  memcpy(dataMemory, data, record_size_data_only);

  auto rec = record_reference_t{this->table_id, meta};
  if (likely(txn != nullptr)) {
    txn->getLog().addInsertLog(rec.getBase());
  }
  return rec;
}

record_reference_t SingleVersionRowStore::insertNRecord(txn::Txn* txn, size_t N, const void* data) {
  CHECK(!txn || (txn && !txn->read_only)) << "RO txn inserting ???";

  void* mem = allocateRecordMemory(N);
  auto mem_p = reinterpret_cast<uintptr_t>(mem);
  record_reference_t ret;

  for (size_t i = 0; i < N; i++) {
    void* base = reinterpret_cast<void*>(mem_p + (record_size * i));

    auto* meta = new (base) record_metadata_t(0);
    if (likely(data != nullptr)) {
      void* dataMemory = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(base) + sizeof(record_metadata_t));
      memcpy(dataMemory, data, record_size_data_only);
    }

    if (i == 0) ret = record_reference_t{this->table_id, meta};
  }
  return ret;
}

void SingleVersionRowStore::updateAttribute(txn::Txn* txn, record_metadata_t* rc, void* value, uint attribute_idx) {
  auto colWidthOffset = column_size_offset_pairs.at(attribute_idx);
  auto data_ptr =
      reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(rc) + sizeof(record_metadata_t) + colWidthOffset.second);
  if (likely(txn != nullptr)) {
    txn->getLog().addUpdateLog(record_reference_t{this->table_id, rc}.getBase(), attribute_idx, data_ptr,
                               colWidthOffset.first);
  }
  memcpy(data_ptr, value, colWidthOffset.first);
}

void SingleVersionRowStore::updateNthRecord(txn::Txn* txn, record_metadata_t* rc, void* value, uint record_offset,
                                            uint attribute_idx) {
  auto rd_rc = reinterpret_cast<uintptr_t>(rc) + (record_size * record_offset);
  return this->updateAttribute(txn, reinterpret_cast<record_metadata_t*>(rd_rc), value, attribute_idx);
}

// record_reference_t SingleVersionRowStore::getRefTypeData(record_metadata_t* rc, uint attribute_idx) {
//   auto data_ptr = reinterpret_cast<uintptr_t>(rc) + sizeof(record_metadata_t);
//   data_ptr = data_ptr + column_size_offsets[attribute_idx];  // actual data_ptr
//
//   return record_reference_t{*(reinterpret_cast<record_reference_t*>(data_ptr))};
// }
//
// void SingleVersionRowStore::updateRefTypeData(txn::Txn&, record_metadata_t* rc, record_reference_t& value,
//                                               uint attribute_idx) {
//   auto data_ptr = reinterpret_cast<record_reference_t*>(reinterpret_cast<uintptr_t>(rc) + sizeof(record_metadata_t) +
//                                                         column_size_offsets[attribute_idx]);
//   *data_ptr = value;
// }

void SingleVersionRowStore::getData(txn::Txn* txn, record_metadata_t* rc, void* dst, size_t offset, size_t len) {
  assert(offset + len < record_size);
  auto data_ptr =
      reinterpret_cast<record_reference_t*>(reinterpret_cast<uintptr_t>(rc) + sizeof(record_metadata_t) + offset);
  memcpy(dst, data_ptr, len);
}
void SingleVersionRowStore::getAttribute(txn::Txn* txn, record_metadata_t* rc, void* dst, uint attribute_idx) {
  assert(rc != nullptr);
  //  LOG(INFO) << rc << " | " << this->name();
  auto colWidthOffset = column_size_offset_pairs.at(attribute_idx);
  auto data_ptr = reinterpret_cast<record_reference_t*>(reinterpret_cast<uintptr_t>(rc) + sizeof(record_metadata_t) +
                                                        colWidthOffset.second);
  memcpy(dst, data_ptr, colWidthOffset.first);
}

void SingleVersionRowStore::getNthRecord(txn::Txn* txn, record_metadata_t* rc, void* dst, uint record_offset,
                                         uint attribute_idx) {
  // rc -- starting record
  auto rd_rc = reinterpret_cast<uintptr_t>(rc) + (record_size * record_offset);
  return this->getAttribute(txn, reinterpret_cast<record_metadata_t*>(rd_rc), dst, attribute_idx);
}

record_reference_t SingleVersionRowStore::getNthRecordReference(txn::Txn* txn, record_metadata_t* rc,
                                                                uint record_offset) {
  auto rd_rc = reinterpret_cast<uintptr_t>(rc) + (record_size * record_offset);

  return record_reference_t{this->table_id, reinterpret_cast<record_metadata_t*>(rd_rc)};
}

void SingleVersionRowStore::rollback_update(record_metadata_t* rc, void* prev_value, uint attribute_idx) {
  auto colWidthOffset = column_size_offset_pairs.at(attribute_idx);
  auto data_ptr =
      reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(rc) + sizeof(record_metadata_t) + colWidthOffset.second);

  memcpy(data_ptr, prev_value, colWidthOffset.first);
}
void SingleVersionRowStore::rollback_create(record_metadata_t* rc) { freeRecordMemory(rc); }