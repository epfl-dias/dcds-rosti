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
#include <unordered_map>
#include <utility>
#include <vector>

#include "dcds/common/common.hpp"
#include "dcds/storage/attribute-def.hpp"
#include "dcds/transaction/concurrency-control/record-metadata.hpp"
#include "dcds/util/packed-ptr.hpp"

namespace dcds::storage {

// class Row{
//   txn::cc::RecordMetaData metadata;
//   uint8_t data;
// };

using record_metadata_t = txn::cc::RecordMetaData_SingleVersion;

class Table;
class SingleVersionRowStore;

class RecordRefV2 {
 public:
  RecordRefV2() : record_metadata_ptr() {}
  RecordRefV2(table_id_t tableId) : record_metadata_ptr(0, tableId) {}

  //  RecordRefV2 &operator=(RecordRefV2 const &other) = default;
  //  RecordRefV2 &operator=(RecordRefV2 &&other) = default;

  auto operator->() { return reinterpret_cast<record_metadata_t *>(record_metadata_ptr.operator->()); }
  auto operator()() { return record_metadata_ptr; }

  bool valid() { return (record_metadata_ptr.operator->() != nullptr); }

  void print() { record_metadata_ptr.print(); }

  std::shared_ptr<Table> getTable();

 private:
  static_assert(sizeof(table_id_t) == packed_ptr_t::DATA_SZ_MAX,
                "Invalid size of table_id_t to be used with packed_ptr_t");

  explicit RecordRefV2(table_id_t tableId, record_metadata_t *rc)
      : record_metadata_ptr(reinterpret_cast<uintptr_t>(rc), tableId) {}
  explicit RecordRefV2(record_metadata_t *rc) : record_metadata_ptr(reinterpret_cast<uintptr_t>(rc)) {}

 private:
  packed_ptr_t record_metadata_ptr;

  friend class Table;
  friend class SingleVersionRowStore;
};

class RecordRefV1 {
 public:
  explicit RecordRefV1() : record_metadata(nullptr), data(nullptr) {}
  RecordRefV1 &operator=(RecordRefV1 const &other) = default;
  //  RecordRef &operator=(RecordRef &&other) = default;

  bool isNull() { return (this->record_metadata == nullptr); }
  auto operator->() { return record_metadata; }

 private:
  explicit RecordRefV1(record_metadata_t *rcm_, void *data_) : record_metadata(rcm_), data(data_) {}

 private:
  record_metadata_t *record_metadata;
  void *data;

  friend class Table;
};

using record_reference_t = RecordRefV2;

// class RecordReference {
//  public:
//   auto operator()() { return record; }
//   auto operator->() { return record; }
//
//   explicit RecordReference() : record() {}
//
//   RecordReference &operator=(RecordReference other) {
//     this->record.swap(other.record);
//     return *this;
//   }
//
//  private:
//   explicit RecordReference(std::shared_ptr<record_metadata_t> r) : record(std::move(r)) {}
//
//   uintptr_t getRawPtr(){
//     return reinterpret_cast<uintptr_t>(record.get());
//   }
//
//  private:
//   std::shared_ptr<record_metadata_t> record;
//
//   friend class Table;
// };

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

  record_reference_t getNullReference() const { return record_reference_t{this->table_id}; }

  virtual record_reference_t insertRecord(const std::shared_ptr<txn::Txn> &txn, const void *data) = 0;

  virtual void updateAttribute(std::shared_ptr<txn::Txn> &txn, record_metadata_t *, void *value,
                               uint attribute_idx) = 0;

  // Another possible issue is packing of recordMetaData?
  //  RecordReference insertRecord(const txn::Txn &txn, const void *data);

  //  std::vector<record_metadata_t *> insertRecordBatch(const void *data, const txn::Txn &txn, int num_records);
  //
  //  void updateRecord(const txn::Txn &txn, record_metadata_t *, void *data, const column_id_t *column_indexes =
  //  nullptr,
  //                    const short num_columns = -1);
  //
  //  void deleteRecord(const txn::Txn &txn, record_metadata_t *);
  //
  //  void getRecord(const txn::Txn &txn, record_metadata_t *, void *destination,
  //                 const column_id_t *column_indexes = nullptr, const short num_columns = -1);
  //
  //  void *getRecordData(const txn::Txn &txn, record_metadata_t *);
  virtual void *getRecordData(record_metadata_t *rc) = 0;

  virtual record_reference_t getRefTypeData(record_metadata_t *rc, uint attribute_idx) = 0;
  virtual void updateRefTypeData(std::shared_ptr<txn::Txn> &, record_metadata_t *, record_reference_t &value,
                                 uint attribute_idx) = 0;

//  virtual void getAttribute(std::shared_ptr<txn::Txn> &, record_metadata_t *rc, uint attribute_idx);
  virtual void getData(std::shared_ptr<txn::Txn> &, record_metadata_t *rc, void* dst, size_t offset, size_t len) = 0;
  virtual void getAttribute(std::shared_ptr<txn::Txn> &, record_metadata_t *rc, void* dst, uint attribute_idx) = 0;



  virtual size_t size() = 0;
  virtual size_t capacity() = 0;
  virtual bool empty() = 0;
  virtual void reserve(size_t) = 0;

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
  ~SingleVersionRowStore() override = default;

 public:
  record_reference_t getRefTypeData(record_metadata_t *rc, uint attribute_idx) override;
  // record_reference_t
  void updateRefTypeData(std::shared_ptr<txn::Txn> &, record_metadata_t *, record_reference_t &value,
                         uint attribute_idx) override;

  inline void *getRecordData(record_metadata_t *rc) override {
    return reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(rc) + sizeof(record_metadata_t));
  }

  record_reference_t insertRecord(const std::shared_ptr<txn::Txn> &txn, const void *data) override;
  void updateAttribute(std::shared_ptr<txn::Txn> &, record_metadata_t *, void *value, uint attribute_idx) override;


  void getData(std::shared_ptr<txn::Txn> &, record_metadata_t *rc, void* dst, size_t offset, size_t len) override;
  void getAttribute(std::shared_ptr<txn::Txn> &, record_metadata_t *rc, void* dst, uint attribute_idx) override;

 public:
  size_t size() override { return records_data.size(); }
  size_t capacity() override { return records_data.size(); }
  bool empty() override { return records_data.empty(); }
  void reserve(size_t) override { throw std::runtime_error("unimplemented"); }

 private:
  std::deque<void *> records_data;

 private:
  void *allocateRecordMemory() const;
  void freeRecordMemory(void *);
};

}  // namespace dcds::storage

#endif  // DCDS_TABLE_HPP
