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

#ifndef DCDS_TABLE_HPP
#define DCDS_TABLE_HPP

#include <deque>
#include <iostream>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "dcds/common/common.hpp"
#include "dcds/storage/attribute-def.hpp"
#include "dcds/transaction/concurrency-control/record-metadata.hpp"
#include "dcds/transaction/transaction.hpp"
#include "dcds/util/locks/spin-lock.hpp"
#include "dcds/util/packed-ptr.hpp"

namespace dcds::storage {

// class Row{
//   txn::cc::RecordMetaData metadata;
//   uint8_t data;
// };

using record_metadata_t = txn::cc::RecordMetaData_SingleVersion;

class Table;
class SingleVersionRowStore;

class RecordReference {
 public:
  RecordReference() : record_metadata_ptr() {}
  explicit RecordReference(uintptr_t unsafe_raw_address) : record_metadata_ptr(unsafe_raw_address) {}
  //  RecordRefV2(table_id_t tableId) : record_metadata_ptr(0, tableId) {}
  //  RecordRefV2 &operator=(RecordRefV2 const &other) = default;
  //  RecordRefV2 &operator=(RecordRefV2 &&other) = default;

  inline auto operator->() { return reinterpret_cast<record_metadata_t *>(record_metadata_ptr.operator->()); }
  auto operator()() { return record_metadata_ptr; }

  bool valid() { return (record_metadata_ptr.operator->() != nullptr); }

  void dump() { record_metadata_ptr.dump(); }

  uintptr_t getBase() { return record_metadata_ptr.getPtr(); }

  Table *getTable();

 private:
  static_assert(sizeof(table_id_t) == packed_ptr_t::DATA_SZ_MAX,
                "Invalid size of table_id_t to be used with packed_ptr_t");

  explicit RecordReference(table_id_t tableId, record_metadata_t *rc)
      : record_metadata_ptr(reinterpret_cast<uintptr_t>(rc), tableId) {}
  explicit RecordReference(record_metadata_t *rc) : record_metadata_ptr(reinterpret_cast<uintptr_t>(rc)) {}

 private:
  packed_ptr_t record_metadata_ptr;

  friend class Table;
  friend class SingleVersionRowStore;
};

using record_reference_t = RecordReference;

// Row store (single version)
class Table {
 public:
  // No copy/move constructors.
  Table(Table &&) = delete;
  Table &operator=(Table &&) = delete;
  Table(const Table &) = delete;
  Table &operator=(const Table &) = delete;

  // we would need index attribute also, otherwise on what attribute the index is created on? rowId?
  Table(table_id_t tableId, std::string table_name, size_t recordSize, std::vector<AttributeDef> attributes,
        bool is_multi_versioned = false);
  virtual ~Table() = default;

  auto name() { return this->table_name; }
  auto id() const { return this->table_id; }

  // record_reference_t getNullReference() const { return record_reference_t{}; }

  virtual record_reference_t insertRecord(txn::Txn *txn, const void *data) = 0;
  virtual record_reference_t insertNRecord(txn::Txn *txn, size_t N, const void *data) = 0;

  virtual void updateAttribute(txn::Txn *txn, record_metadata_t *, void *value, uint attribute_idx) = 0;
  virtual void updateNthRecord(txn::Txn *txn, record_metadata_t *, void *value, uint record_offset,
                               uint attribute_idx) = 0;

  //  virtual record_reference_t getRefTypeData(record_metadata_t *rc, uint attribute_idx) = 0;
  //  virtual void updateRefTypeData(txn::Txn &, record_metadata_t *, record_reference_t &value, uint attribute_idx) =
  //  0;

  //  virtual void *getRecordData(record_metadata_t *rc) = 0;
  virtual void getData(txn::Txn *txn, record_metadata_t *rc, void *dst, size_t offset, size_t len) = 0;
  virtual void getAttribute(txn::Txn *txn, record_metadata_t *rc, void *dst, uint attribute_idx) = 0;

  virtual record_reference_t getNthRecordReference(txn::Txn *txn, record_metadata_t *rc, uint record_offset) = 0;

  virtual void getNthRecord(txn::Txn *txn, record_metadata_t *rc, void *dst, uint record_offset,
                            uint attribute_idx) = 0;

  virtual size_t size() = 0;
  virtual size_t capacity() = 0;
  virtual bool empty() = 0;
  virtual void reserve(size_t) = 0;

  virtual void rollback_update(record_metadata_t *, void *prev_value, uint attribute_idx) = 0;
  virtual void rollback_create(record_metadata_t *) = 0;

 protected:
  std::atomic<size_t> record_id_gen{};

 protected:
  const table_id_t table_id;
  const std::string table_name;

  const size_t record_size;
  const size_t record_size_data_only;

  std::vector<AttributeDef> columns;

  // Dictionaries if any.
  //  std::unordered_map<column_id_t, void*> dictionary_mappings;

  // define d-string dictionary type.
  //  it will have key(id) and value (string) but what about reverse mapping to check for existence?

 protected:
  std::vector<std::pair<uint16_t, uint16_t>> column_size_offset_pairs{};
  std::vector<uint16_t> column_size_offsets{};
  std::vector<uint16_t> column_size{};

 protected:
  const bool is_multiversion;
};

class SingleVersionRowStore : public Table {
 public:
  SingleVersionRowStore(table_id_t tableId, const std::string &table_name, size_t recordSize,
                        std::vector<AttributeDef> attributes);
  ~SingleVersionRowStore() override;

 public:
  //  record_reference_t getRefTypeData(record_metadata_t *rc, uint attribute_idx) override;
  //  void updateRefTypeData(txn::Txn &, record_metadata_t *, record_reference_t &value, uint attribute_idx) override;

  record_reference_t insertRecord(txn::Txn *txn, const void *data) override;
  record_reference_t insertNRecord(txn::Txn *txn, size_t N, const void *data) override;

  void updateAttribute(txn::Txn *txn, record_metadata_t *, void *value, uint attribute_idx) override;
  void updateNthRecord(txn::Txn *txn, record_metadata_t *, void *value, uint record_offset,
                       uint attribute_idx) override;

  //  inline void *getRecordData(record_metadata_t *rc) override {
  //    return reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(rc) + sizeof(record_metadata_t));
  //  }
  void getData(txn::Txn *txn, record_metadata_t *rc, void *dst, size_t offset, size_t len) override;
  void getAttribute(txn::Txn *txn, record_metadata_t *rc, void *dst, uint attribute_idx) override;

  record_reference_t getNthRecordReference(txn::Txn *txn, record_metadata_t *rc, uint record_offset) override;

  void getNthRecord(txn::Txn *txn, record_metadata_t *rc, void *dst, uint record_offset, uint attribute_idx) override;

  void rollback_update(record_metadata_t *rc, void *prev_value, uint attribute_idx) override;
  void rollback_create(record_metadata_t *rc) override;

 public:
  size_t size() override { return records_data.size(); }
  size_t capacity() override { return records_data.size(); }
  bool empty() override { return records_data.empty(); }
  void reserve(size_t) override { throw std::runtime_error("unimplemented"); }

 private:
  std::deque<void *> records_data;

  dcds::utils::locks::SpinLock allocation_lock;
  std::set<void *> memory_allocations;

 private:
  void *allocateRecordMemory(size_t n_records = 1);
  void freeRecordMemory(void *);
};

}  // namespace dcds::storage

#endif  // DCDS_TABLE_HPP
