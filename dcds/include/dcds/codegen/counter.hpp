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

#ifndef DCDS_UN_GENERATED_COUNTER_HPP
#define DCDS_UN_GENERATED_COUNTER_HPP

#include <dcds/common/types.hpp>
#include <dcds/storage/table.hpp>
#include <iostream>
#include <utility>

using counterValueType = int64_t;

class UnGeneratedCounter {
 public:
  explicit UnGeneratedCounter(counterValueType initialValue = 0, counterValueType step = 1) {
    initialCounterValue = initialValue;
    counterStep = step;
    // LOG(INFO) << "initializing with default";
    init("default");
  }

  explicit UnGeneratedCounter(const std::string& txn_namespace, counterValueType initialValue = 0,
                              counterValueType step = 1) {
    initialCounterValue = initialValue;
    counterStep = step;
    // LOG(INFO) << "initializing with ns: " << txn_namespace;
    init(txn_namespace);
  }

  counterValueType read();                     // read the current counter value
  void update();                               // update the counter value by counterStep
  void write(counterValueType writeVariable);  // write custom value to counter

 private:
  std::shared_ptr<dcds::txn::TransactionManager> txnManager;
  dcds::storage::record_reference_t mainRecord;
  std::shared_ptr<dcds::storage::Table> counterTable;
  counterValueType initialCounterValue;
  counterValueType counterStep;

  struct __attribute__((packed)) counter_record_st {
    counterValueType value{};
  };

  void init(const std::string& txn_namespace);
  static void initTables(const std::string& txn_namespace);
};

#endif  // DCDS_UN_GENERATED_COUNTER_HPP
