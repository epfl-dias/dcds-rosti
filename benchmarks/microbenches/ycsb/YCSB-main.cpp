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

#include <dcds/dcds.hpp>
#include <random>

constexpr size_t num_columns = 1;
constexpr size_t num_records = 1_M;  // 10;
constexpr size_t num_ops_per_txn = 10;
constexpr size_t num_txn_per_thread = 1_M;
constexpr size_t num_threads = 4;
const std::string item_name = "YCSB_ITEM";
const auto item_type = dcds::valueType::INT64;

static auto generateYCSB_Item(const std::shared_ptr<dcds::Builder>& builder) {
  auto item_builder = builder->createType(item_name);

  for (size_t i = 0; i < num_columns; i++) {
    item_builder->addAttribute("column_" + std::to_string(i), item_type, UINT64_C(88));
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

static void generateLookupOneFunction(const std::shared_ptr<dcds::Builder>& builder) {
  // getByKey

  auto fn = builder->createFunction("lookup", dcds::valueType::VOID);
  auto key_arg = fn->addArgument("key", dcds::valueType::INT64);

  for (size_t i = 0; i < num_columns; i++) {
    fn->addArgument("column_" + std::to_string(i) + "_ret", item_type, true);
  }

  auto sb = fn->getStatementBuilder();

  // for the given key, get the record
  auto rec_attribute = builder->getAttribute("records");
  auto rec = fn->addTempVariable("rec", dcds::valueType::RECORD_PTR);

  // TODO: bound checking

  // attribute, destination, key
  sb->addReadStatement(rec_attribute, rec, key_arg);

  // read-all-one-go
  std::vector<std::shared_ptr<dcds::expressions::Expression>> args;
  for (size_t i = 0; i < num_columns; i++) {
    args.push_back(fn->getArgument("column_" + std::to_string(i) + "_ret"));
  }

  // "rec" contains the ptr
  sb->addMethodCall(builder->getRegisteredType(item_name), rec, "get_record", args);

  sb->addReturnVoidStatement();
}

static void generateLookupNFunction(const std::shared_ptr<dcds::Builder>& builder) {
  // getByKey

  auto fn = builder->createFunction("lookup_n", dcds::valueType::VOID);
  auto key_arg = fn->addArgument("key", dcds::valueType::INT64);
  auto sb = fn->getStatementBuilder();

  // We need a better way to read the record. maybe a void ptr where all can be read.
  for (size_t n = 0; n < num_ops_per_txn; n++) {
    for (size_t i = 0; i < num_columns; i++) {
      fn->addArgument("column_" + std::to_string(n) + "_" + std::to_string(i) + "_ret", item_type, true);
    }
  }

  // for the given key, get the record
  auto rec_attribute = builder->getAttribute("records");
  auto rec = fn->addTempVariable("rec", dcds::valueType::RECORD_PTR);

  for (size_t n = 0; n < num_ops_per_txn; n++) {
    // attribute, destination, key
    sb->addReadStatement(rec_attribute, rec, key_arg);

    // read-all-one-go
    std::vector<std::shared_ptr<dcds::expressions::Expression>> args;
    for (size_t i = 0; i < num_columns; i++) {
      args.push_back(fn->getArgument("column_" + std::to_string(n) + "_" + std::to_string(i) + "_ret"));
    }

    // "rec" contains the ptr
    sb->addMethodCall(builder->getRegisteredType(item_name), rec, "get_record", args);
  }

  sb->addReturnVoidStatement();
}

static void printThroughput(size_t runtime_ms, size_t n_threads) {
  auto runtime_s = ((static_cast<double>(runtime_ms)) / 1000);
  auto throughput = ((num_txn_per_thread * n_threads) / runtime_s);
  auto avg_per_thread = (throughput / n_threads) / 1000000;
  LOG(INFO) << "Threads: " << n_threads << ": Throughput: " << ((num_txn_per_thread * n_threads) / runtime_s) / 1000000
            << " MTPS"
            << " | avg-per-thread: " << avg_per_thread << " MTPS"
            << " | total_time: " << runtime_ms << "ms";
}

static void test_MT_lookup_random(dcds::JitContainer* instance, size_t n_threads) {
  auto thr = dcds::ThreadRunner(n_threads);

  static thread_local std::mt19937 engine(std::random_device{}());
  static thread_local std::uniform_real_distribution<double> dist{0.0, 1.0};

  //  dcds::profiling::ProfileRegion my_region("YCSB_MT_lookup_random");
  auto runtime_ms = thr([&]() {
    int64_t one = 99;
    for (size_t i = 0; i < num_txn_per_thread; i++) {
      instance->op("lookup", (static_cast<size_t>(dist(engine) * num_records)) % num_records, &one);
    }
  });
  printThroughput(runtime_ms, n_threads);
}

static void test_MT_lookup_sequential(dcds::JitContainer* instance, size_t n_threads) {
  auto thr = dcds::ThreadRunner(n_threads);

  //  dcds::profiling::ProfileRegion my_region("YCSB_MT_lookup_sequential");
  auto runtime_ms = thr([&]() {
    int64_t one = 99;

    for (size_t i = 0; i < num_txn_per_thread; i++) {
      instance->op("lookup", i % num_records, &one);
    }
  });
  printThroughput(runtime_ms, n_threads);
}

static void test_MT_lookup_sequential_n(dcds::JitContainer* instance, size_t n_threads) {
  auto thr = dcds::ThreadRunner(n_threads);

  //  dcds::profiling::ProfileRegion my_region("YCSB_MT_lookup_sequential");
  auto runtime_ms = thr([&]() {
    auto* ret = reinterpret_cast<int64_t*>(malloc(sizeof(int64_t) * num_columns * num_ops_per_txn));

    for (size_t i = 0; i < num_txn_per_thread; i++) {
      instance->op("lookup_n", i % num_records, ret, (ret + 1), (ret + 2), (ret + 3), (ret + 4), (ret + 5), (ret + 6),
                   (ret + 7), (ret + 8), (ret + 9));
    }
  });

  printThroughput(runtime_ms, n_threads);
}

int main(int argc, char** argv) {
  dcds::InitializeLog(argc, argv);
  LOG(INFO) << "YCSB";

  dcds::ScopedAffinityManager scopedAffinity(dcds::Core{0});

  auto builder = std::make_shared<dcds::Builder>("YCSB");
  //   builder->addHint(dcds::hints::BuilderHints::SINGLE_THREADED);
  auto ycsb_item = generateYCSB_Item(builder);

  // Attribute array is integer-indexed.
  // Attribute IndexedList is key-based index.

  builder->addAttributeArray("records", ycsb_item, num_records);

  generateLookupOneFunction(builder);
  generateLookupNFunction(builder);
  generateUpdateFunction(builder);

  //  builder->dump();
  builder->injectCC();
  LOG(INFO) << "######################";
  builder->dump();
  LOG(INFO) << "######################";

  builder->build();

  auto instance = builder->createInstance();
  instance->listAllAvailableFunctions();

  test_MT_lookup_sequential(instance, 8);  // warm-up

  LOG(INFO) << "-----test_MT_lookup_sequential (1 op/txn)----";
  dcds::profiling::Profile::resume();
  for (size_t i = 1; i <= num_threads; i *= 2) {
    test_MT_lookup_sequential(instance, i);
  }
  dcds::profiling::Profile::pause();

  LOG(INFO) << "-----test_MT_lookup_random (1 op/txn)----";

  dcds::profiling::Profile::resume();
  for (size_t i = 1; i <= num_threads; i *= 2) {
    test_MT_lookup_random(instance, i);
  }
  dcds::profiling::Profile::pause();

  LOG(INFO) << "-----test_MT_lookup_sequential-N  (" << num_ops_per_txn << " op/txn)---";
  dcds::profiling::Profile::resume();
  for (size_t i = 1; i <= num_threads; i *= 2) {
    test_MT_lookup_sequential_n(instance, i);
  }
  dcds::profiling::Profile::pause();

  return 0;
}
