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

#include <benchmark/benchmark.h>

#include <barrier>
#include <dcds/dcds.hpp>
#include <dcds/util/thread-runner.hpp>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "dcds/util/profiling.hpp"
#include "dcds/util/timing.hpp"

constexpr size_t num_threads = 4;
constexpr size_t num_columns = 1;
constexpr size_t num_records = 1_M;  // 10;
const std::string item_name = "YCSB_ITEM";
const auto item_type = dcds::valueType::INT64;

// static void CHECK_TRUE(std::any v) { CHECK(std::any_cast<bool>(v)); }
// static void CHECK_FALSE(std::any v) { CHECK_EQ(std::any_cast<bool>(v), false); }

static auto generateYCSB_Item(const std::shared_ptr<dcds::Builder>& builder) {
  auto item_builder = builder->createType(item_name);
  // builder->addHint(dcds::hints::BuilderHints::SINGLE_THREADED);

  for (size_t i = 0; i < num_columns; i++) {
    item_builder->addAttribute("column_" + std::to_string(i), item_type, UINT64_C(88));
  }

  {
      //    auto get_fn = item_builder->createFunction("get_value", item_type);
      //    get_fn->addArgument("column_index", dcds::valueType::INT64);
      //    get_fn->setAlwaysInline(true);
      //
      //    auto sb = get_fn->getStatementBuilder();
      //    get_fn->addTempVariable("tmp", item_type);
      //
      //    sb->addReadStatement(attribute, "tmp");
      //
      //    sb->addReturnStatement("tmp");
  }

  // read_all
  {
    // void get_record(column_1 *, column_2 *, column_2 *,...)
    auto get_all_fn = item_builder->createFunction("get_record", dcds::valueType::VOID);
    for (size_t i = 0; i < num_columns; i++) {
      get_all_fn->addArgument("col_" + std::to_string(i), item_type, true);
    }
    get_all_fn->setAlwaysInline(true);

    auto sb = get_all_fn->getStatementBuilder();
    for (size_t i = 0; i < num_columns; i++) {
      auto attribute_name = "column_" + std::to_string(i);
      auto arg_name = "col_" + std::to_string(i);
      sb->addReadStatement(item_builder->getAttribute(attribute_name), get_all_fn->getArgument(arg_name));
    }

    sb->addReturnVoidStatement();
  }

  // update_all
  {
    // void update_record(column_1 *, column_2 *, column_2 *,...)

    auto upd_all_fn = item_builder->createFunction("update_record", dcds::valueType::VOID);
    for (size_t i = 0; i < num_columns; i++) {
      upd_all_fn->addArgument("col_" + std::to_string(i), item_type);
    }
    upd_all_fn->setAlwaysInline(true);

    auto sb = upd_all_fn->getStatementBuilder();
    for (size_t i = 0; i < num_columns; i++) {
      auto attribute_name = "column_" + std::to_string(i);
      auto arg_name = "col_" + std::to_string(i);
      sb->addUpdateStatement(item_builder->getAttribute(attribute_name), upd_all_fn->getArgument(arg_name));
    }

    sb->addReturnVoidStatement();
  }

  return item_builder;
}

static void generateUpdateFunction(const std::shared_ptr<dcds::Builder>& builder) {
  // void update(key, val_1, val_2, ...)
  auto fn = builder->createFunction("update", dcds::valueType::VOID);
  auto key_arg = fn->addArgument("key", dcds::valueType::INT64);

  for (size_t i = 0; i < num_columns; i++) {
    fn->addArgument("val_" + std::to_string(i), item_type);
  }

  auto sb = fn->getStatementBuilder();

  auto rec_attribute = builder->getAttribute("records");
  auto rec = fn->addTempVariable("rec", dcds::valueType::RECORD_PTR);

  sb->addReadStatement(rec_attribute, rec, key_arg);

  // update-all-one-go
  std::vector<std::shared_ptr<dcds::expressions::Expression>> args;
  for (size_t i = 0; i < num_columns; i++) {
    args.push_back(fn->getArgument("val_" + std::to_string(i)));
  }

  sb->addMethodCall(builder->getRegisteredType(item_name), rec, "update_record", args);

  sb->addReturnVoidStatement();
}

static void generateLookupFunction(const std::shared_ptr<dcds::Builder>& builder) {
  // Lookup function to return entire data.
  // lookupByKey

  // what does it return? what type basically.
  // maybe bool lookup(key, column_1* column_2*,...);
  // where bool denotes if it is present or not.
  auto fn = builder->createFunction("lookup", dcds::valueType::BOOL);
  auto key_arg = fn->addArgument("key", dcds::valueType::INT64);

  for (size_t i = 0; i < num_columns; i++) {
    fn->addArgument("column_" + std::to_string(i) + "_ret", item_type, true);
  }

  auto sb = fn->getStatementBuilder();

  // for the given key, get the record
  auto rec_attribute = builder->getAttribute("records");
  auto rec = fn->addTempVariable("rec", dcds::valueType::RECORD_PTR);

  // bound checking.

  // this get the item (record-ptr) for that index as this array is of complex type

  // attribute, destination, key
  sb->addReadStatement(rec_attribute, rec, key_arg);

  // sb->addLogStatement("RECORD READ DONE");

  // Ideally: rec = rec_attribute[key_arg];
  // tempVar = attribute[localVarExpression]
  // but then needs bound checking or we don't do that?

  // then for each column, we perform the read.
  //  for (size_t i = 0; i < num_columns; i++) {
  //    sb->addMethodCall(builder->getRegisteredType(item_name), rec, "get_value", fn->getArgument("column_" +
  //    std::to_string(i) + "_ret"),
  //                      {std::make_shared<dcds::expressions::Int64Constant>(i)});
  //  }

  // read-all-one-go
  std::vector<std::shared_ptr<dcds::expressions::Expression>> args;
  for (size_t i = 0; i < num_columns; i++) {
    args.push_back(fn->getArgument("column_" + std::to_string(i) + "_ret"));
  }

  // "rec" contains the ptr
  sb->addMethodCall(builder->getRegisteredType(item_name), rec, "get_record", args);

  sb->addReturnStatement(std::make_shared<dcds::expressions::BoolConstant>(true));
}

static void callLookup2(dcds::JitContainer* instance) {
  int64_t one = 99;
  int64_t two = 100;
  for (size_t i = 0; i < 10; i++) {
    instance->op("lookup", i, &one, &two);
  }
}

static void callLookup(dcds::JitContainer* instance) {
  int64_t one = 99;
  int64_t two = 100;
  size_t key = 3;
  LOG(INFO) << "Calling loookup";

  // key, then 2 columns
  LOG(INFO) << "One before: " << one;
  LOG(INFO) << "Two before: " << two;
  instance->op("lookup", key, &one, &two);
  LOG(INFO) << "One: " << one;
  LOG(INFO) << "Two: " << two;

  instance->op("update", key, 102, 102);

  instance->op("lookup", key, &one, &two);
  LOG(INFO) << "One: " << one;
  LOG(INFO) << "Two: " << two;

  instance->op("lookup", 1, &one, &two);
  LOG(INFO) << "One: " << one;
  LOG(INFO) << "Two: " << two;
}

static void testST(dcds::JitContainer* instance) {
  callLookup(instance);
  {
    time_block t("Trad_clust_p1:");
    callLookup2(instance);
  }
}

static void test_MT_lookup_random(dcds::JitContainer* instance, size_t n_threads) {
  auto thr = ThreadRunner(n_threads);

  static thread_local std::mt19937 engine(std::random_device{}());
  static thread_local std::uniform_real_distribution<double> dist{0.0, 1.0};

  //  dcds::profiling::ProfileRegion my_region("YCSB_MT_lookup_random");
  thr([&]() {
    int64_t one = 99;
    for (size_t i = 0; i < 1_M; i++) {
      instance->op("lookup", (static_cast<size_t>(dist(engine) * num_records)) % num_records, &one);
    }
  });
}

static void test_MT_lookup_sequential(dcds::JitContainer* instance, size_t n_threads) {
  auto thr = ThreadRunner(n_threads);

  //  dcds::profiling::ProfileRegion my_region("YCSB_MT_lookup_sequential");
  thr([&]() {
    int64_t one = 99;
    for (size_t i = 0; i < 1_M; i++) {
      instance->op("lookup", i, &one);
    }
  });
}

int main(int argc, char** argv) {
  dcds::InitializeLog(argc, argv);
  LOG(INFO) << "YCSB";

  auto builder = std::make_shared<dcds::Builder>("YCSB");
  auto ycsb_item = generateYCSB_Item(builder);

  // Attribute array is integer-indexed.
  // Attribute IndexedList is key-based index.

  builder->addAttributeArray("records", ycsb_item, num_records);

  generateLookupFunction(builder);
  generateUpdateFunction(builder);

  builder->dump();
  // builder->injectCC();
  builder->dump();
  builder->build();

  auto instance = builder->createInstance();
  instance->listAllAvailableFunctions();
  dcds::profiling::Profile::resume();
  testST(instance);
  dcds::profiling::Profile::pause();

  LOG(INFO) << "-----test_MT_lookup_sequential----";
  dcds::profiling::Profile::resume();
  for (size_t i = 1; i <= num_threads; i *= 2) {
    LOG(INFO) << "-----: " << i;
    test_MT_lookup_sequential(instance, i);
    LOG(INFO) << "-----";
  }
  dcds::profiling::Profile::pause();

  LOG(INFO) << "-----test_MT_lookup_random----";

  dcds::profiling::Profile::resume();
  for (size_t i = 1; i <= num_threads; i *= 2) {
    LOG(INFO) << "-----: " << i;
    test_MT_lookup_random(instance, i);
    LOG(INFO) << "-----";
  }

  dcds::profiling::Profile::pause();

  return 0;
}
