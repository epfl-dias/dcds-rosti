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

#ifndef DCDS_UN_GENERATED_QUEUE_HPP
#define DCDS_UN_GENERATED_QUEUE_HPP

#include <dcds/storage/table.hpp>
#include <iostream>
#include <utility>

using queueValueType = size_t;

class UnGeneratedQueue {
 public:
  explicit UnGeneratedQueue() {
    // LOG(INFO) << "initializing with default";
    init("default");
  }
  explicit UnGeneratedQueue(const std::string& txn_namespace) {
    // LOG(INFO) << "initializing with ns: " << txn_namespace;
    init(txn_namespace);
  }

  size_t popAndReturn();          // pop from the front & returns value
  void pop();                     // pop from the front
  void push(queueValueType val);  // insert at the end

  queueValueType front();
  queueValueType back();

  size_t size();

  void printQueue();

 private:
  void init(const std::string& txn_namespace);

 private:
  std::shared_ptr<dcds::txn::TransactionManager> txnManager;
  dcds::storage::record_reference_t mainRecord;
  std::shared_ptr<dcds::storage::Table> listTable;
  std::shared_ptr<dcds::storage::Table> listNodeTable;

 private:
  struct __attribute__((packed)) list_record_st {
    dcds::storage::record_reference_t head;
    dcds::storage::record_reference_t tail;
    size_t size{};
  };

  struct __attribute__((packed)) node_record_st {
    dcds::storage::record_reference_t next{};
    queueValueType payload{};
  };

  static void initTables(const std::string& txn_namespace);
};

#endif  // DCDS_UN_GENERATED_QUEUE_HPP
