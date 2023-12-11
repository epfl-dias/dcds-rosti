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

#ifndef DCDS_YCSB_HPP
#define DCDS_YCSB_HPP

#include <dcds/dcds.hpp>
#include <random>

#include "dcds/util/bench-utils/zipf-generator.hpp"

constexpr size_t num_txn_per_thread = 5_M;
constexpr size_t num_ops_per_txn = 10;

const std::string item_name = "YCSB_ITEM";
const auto item_type = dcds::valueType::INT64;

class YCSB {
 private:
  auto generateYCSB_Item() {
    auto item_builder = this->_builder->createType(item_name);

    for (size_t i = 0; i < n_columns; i++) {
      item_builder->addAttribute("column_" + std::to_string(i), item_type, UINT64_C(88));
    }

    // read_all
    {
      // void get_record(column_1 *, column_2 *, column_2 *,...)
      auto get_all_fn = item_builder->createFunction("get_record", dcds::valueType::VOID);
      for (size_t i = 0; i < n_columns; i++) {
        get_all_fn->addArgument("col_" + std::to_string(i), item_type, true);
      }
      get_all_fn->setAlwaysInline(true);

      auto sb = get_all_fn->getStatementBuilder();
      for (size_t i = 0; i < n_columns; i++) {
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
      for (size_t i = 0; i < n_columns; i++) {
        upd_all_fn->addArgument("col_" + std::to_string(i), item_type);
      }
      upd_all_fn->setAlwaysInline(true);

      auto sb = upd_all_fn->getStatementBuilder();
      for (size_t i = 0; i < n_columns; i++) {
        auto attribute_name = "column_" + std::to_string(i);
        auto arg_name = "col_" + std::to_string(i);
        sb->addUpdateStatement(item_builder->getAttribute(attribute_name), upd_all_fn->getArgument(arg_name));
      }

      sb->addReturnVoidStatement();
    }

    return item_builder;
  }

  void generateUpdateFunction() {
    // void update(key, val_1, val_2, ...)
    auto fn = _builder->createFunction("update", dcds::valueType::VOID);
    auto key_arg = fn->addArgument("key", dcds::valueType::INT64);

    for (size_t i = 0; i < n_columns; i++) {
      fn->addArgument("val_" + std::to_string(i), item_type);
    }

    auto sb = fn->getStatementBuilder();

    auto rec_attribute = _builder->getAttribute("records");
    auto rec = fn->addTempVariable("rec", dcds::valueType::RECORD_PTR);

    sb->addReadStatement(rec_attribute, rec, key_arg);

    // update-all-one-go
    std::vector<std::shared_ptr<dcds::expressions::Expression>> args;
    for (size_t i = 0; i < n_columns; i++) {
      args.push_back(fn->getArgument("val_" + std::to_string(i)));
    }

    sb->addMethodCall(_builder->getRegisteredType(item_name), rec, "update_record", args);

    sb->addReturnVoidStatement();
  }

  void generateLookupFunction() {
    // getByKey

    auto fn = _builder->createFunction("lookup", dcds::valueType::VOID);
    auto key_arg = fn->addArgument("key", dcds::valueType::INT64);

    for (size_t i = 0; i < n_columns; i++) {
      fn->addArgument("column_" + std::to_string(i) + "_ret", item_type, true);
    }

    auto sb = fn->getStatementBuilder();

    // for the given key, get the record
    auto rec_attribute = _builder->getAttribute("records");
    auto rec = fn->addTempVariable("rec", dcds::valueType::RECORD_PTR);

    // TODO: bound checking

    // attribute, destination, key
    sb->addReadStatement(rec_attribute, rec, key_arg);

    // read-all-one-go
    std::vector<std::shared_ptr<dcds::expressions::Expression>> args;
    for (size_t i = 0; i < n_columns; i++) {
      args.push_back(fn->getArgument("column_" + std::to_string(i) + "_ret"));
    }

    // "rec" contains the ptr
    sb->addMethodCall(_builder->getRegisteredType(item_name), rec, "get_record", args);

    sb->addReturnVoidStatement();
  }

  void generateLookupNFunction(size_t n_ops) {
    if (_n_ops == 0) {
      _n_ops = n_ops;
    } else {
      assert(n_ops == _n_ops);
    }

    auto fn = _builder->createFunction("lookup_n", dcds::valueType::VOID);
    auto key_arg = fn->addArgument("key", dcds::valueType::INT64);
    auto sb = fn->getStatementBuilder();

    // We need a better way to read the record. maybe a void ptr where all can be read.
    for (size_t n = 0; n < _n_ops; n++) {
      for (size_t i = 0; i < n_columns; i++) {
        fn->addArgument("column_" + std::to_string(n) + "_" + std::to_string(i) + "_ret", item_type, true);
      }
    }

    // for the given key, get the record
    auto rec_attribute = _builder->getAttribute("records");
    auto rec = fn->addTempVariable("rec", dcds::valueType::RECORD_PTR);

    for (size_t n = 0; n < _n_ops; n++) {
      // attribute, destination, key
      sb->addReadStatement(rec_attribute, rec, key_arg);

      // read-all-one-go
      std::vector<std::shared_ptr<dcds::expressions::Expression>> args;
      for (size_t i = 0; i < n_columns; i++) {
        args.push_back(fn->getArgument("column_" + std::to_string(n) + "_" + std::to_string(i) + "_ret"));
      }

      // "rec" contains the ptr
      sb->addMethodCall(_builder->getRegisteredType(item_name), rec, "get_record", args);
    }

    sb->addReturnVoidStatement();
  }

 public:
  inline auto test_MT_lookup_random(size_t n_threads, bool print_res = true) {
    assert(instance);
    auto thr = dcds::ThreadRunner(n_threads);

    //  dcds::profiling::ProfileRegion my_region("YCSB_MT_lookup_random");
    auto runtime_ms = thr(
        [](dcds::JitContainer* _instance, size_t _nr) {
          int64_t o1 = 99;
          int64_t o2 = 99;
          int64_t o3 = 99;
          int64_t o4 = 99;
          int64_t o5 = 99;
          int64_t o6 = 99;
          int64_t o7 = 99;
          int64_t o8 = 99;
          int64_t o9 = 99;
          int64_t o10 = 99;
          std::mt19937 engine(std::random_device{}());
          std::uniform_real_distribution<double> dist{0.0, 1.0};
          // dcds::profiling::Profile::resume();
          for (size_t i = 0; i < num_txn_per_thread; i++) {
            _instance->op("lookup", (static_cast<size_t>(dist(engine) * _nr)) % _nr, &o1, &o2, &o3, &o4, &o5, &o6, &o7,
                          &o8, &o9, &o10);
          }
          // dcds::profiling::Profile::pause();
        },
        instance, n_records);

    if (print_res) printThroughput(runtime_ms, n_threads, " (Random)");
    return runtime_ms;
  }

  inline auto test_MT_lookup_sequential(size_t n_threads, bool print_res = true) {
    assert(instance);
    auto thr = dcds::ThreadRunner(n_threads);

    //  dcds::profiling::ProfileRegion my_region("YCSB_MT_lookup_sequential");
    auto runtime_ms = thr(
        [](dcds::JitContainer* _instance, size_t _nr) {
          int64_t o1 = 99;
          int64_t o2 = 99;
          int64_t o3 = 99;
          int64_t o4 = 99;
          int64_t o5 = 99;
          int64_t o6 = 99;
          int64_t o7 = 99;
          int64_t o8 = 99;
          int64_t o9 = 99;
          int64_t o10 = 99;

          for (size_t i = 0; i < num_txn_per_thread; i++) {
            _instance->op("lookup", i % _nr, &o1, &o2, &o3, &o4, &o5, &o6, &o7, &o8, &o9, &o10);
          }
        },
        instance, n_records);

    if (print_res) printThroughput(runtime_ms, n_threads, " (Sequential)");
    return runtime_ms;
  }

  inline auto test_MT_rw_random(size_t n_threads, const uint write_ratio = 50, bool print_res = true) {
    assert(instance);
    assert(write_ratio >= 0 && write_ratio <= 100);
    auto thr = dcds::ThreadRunner(n_threads);

    //  dcds::profiling::ProfileRegion my_region("YCSB_MT_lookup_random");
    auto runtime_ms = thr(
        [write_ratio](dcds::JitContainer* _instance, const size_t _nr) {
          int64_t o1 = 99;
          int64_t o2 = 99;
          int64_t o3 = 99;
          int64_t o4 = 99;
          int64_t o5 = 99;
          int64_t o6 = 99;
          int64_t o7 = 99;
          int64_t o8 = 99;
          int64_t o9 = 99;
          int64_t o10 = 99;

          std::mt19937 engine(std::random_device{}());
          std::uniform_int_distribution<size_t> dist(0, _nr - 1);

          if (write_ratio == 0) {
            for (size_t i = 0; i < num_txn_per_thread; i++) {
              // read-op
              _instance->op("lookup", dist(engine), &o1, &o2, &o3, &o4, &o5, &o6, &o7, &o8, &o9, &o10);
            }
          } else {
            std::mt19937 rw_gen(std::random_device{}());
            std::uniform_int_distribution<size_t> rw_dist(0, 100);

            for (size_t i = 0; i < num_txn_per_thread; i++) {
              if (rw_dist(rw_gen) < write_ratio) {
                // write-op
                _instance->op("update", dist(engine), &o1, &o2, &o3, &o4, &o5, &o6, &o7, &o8, &o9, &o10);
              } else {
                // read-op
                _instance->op("lookup", dist(engine), &o1, &o2, &o3, &o4, &o5, &o6, &o7, &o8, &o9, &o10);
              }
            }
          }
        },
        instance, n_records);

    if (print_res) printThroughput(runtime_ms, n_threads, " (Random)(R/W: " + std::to_string(write_ratio) + " )");
    return runtime_ms;
  }

  inline auto test_MT_rw_zipf(size_t n_threads, double zipf_theta = 0, const uint write_ratio = 50,
                              bool print_res = true) {
    //    LOG_FIRST_N(INFO, 1) << "dcds::ZipfianGenerator: " << sizeof(dcds::ZipfianGenerator);
    //    LOG_FIRST_N(INFO, 1) << "dcds::zip2: " << sizeof(dcds::zip2);

    assert(instance);
    assert(write_ratio >= 0 && write_ratio <= 100);
    auto thr = dcds::ThreadRunner(n_threads);

    //  dcds::profiling::ProfileRegion my_region("YCSB_MT_lookup_random");
    auto runtime_ms = thr(
        [write_ratio, zipf_theta](dcds::JitContainer* _instance, const size_t _nr) {
          int64_t o1 = 99;
          int64_t o2 = 99;
          int64_t o3 = 99;
          int64_t o4 = 99;
          int64_t o5 = 99;
          int64_t o6 = 99;
          int64_t o7 = 99;
          int64_t o8 = 99;
          int64_t o9 = 99;
          int64_t o10 = 99;

          auto zipf = dcds::ZipfianGenerator(_nr, zipf_theta);
          // auto zipf = dcds::zip2(_nr, zipf_theta);
          // auto zipf = dcds::absl_zipf(_nr, zipf_theta);

          if (write_ratio == 0) {
            for (size_t i = 0; i < num_txn_per_thread; i++) {
              // read-op
              _instance->op("lookup", zipf(), &o1, &o2, &o3, &o4, &o5, &o6, &o7, &o8, &o9, &o10);
            }
          } else {
            std::mt19937 rw_gen(std::random_device{}());
            std::uniform_int_distribution<size_t> rw_dist(0, 100);

            for (size_t i = 0; i < num_txn_per_thread; i++) {
              if (rw_dist(rw_gen) < write_ratio) {
                // write-op
                _instance->op("update", zipf(), &o1, &o2, &o3, &o4, &o5, &o6, &o7, &o8, &o9, &o10);
              } else {
                // read-op
                _instance->op("lookup", zipf(), &o1, &o2, &o3, &o4, &o5, &o6, &o7, &o8, &o9, &o10);
              }
            }
          }
        },
        instance, n_records);

    if (print_res)
      printThroughput(runtime_ms, n_threads,
                      " (zipf: " + std::to_string(zipf_theta) + ")(R/W: " + std::to_string(write_ratio) + " )");
    return runtime_ms;
  }

  //  void test_MT_lookup_sequential_n(size_t n_threads) {
  //    assert(false && "fix the keys for each access");
  //    assert(instance);
  //    auto thr = dcds::ThreadRunner(n_threads);
  //
  //    //  dcds::profiling::ProfileRegion my_region("YCSB_MT_lookup_sequential");
  //    auto runtime_ms = thr([&]() {
  //      auto* ret = reinterpret_cast<int64_t*>(malloc(sizeof(int64_t) * n_columns * num_ops_per_txn));
  //
  //      for (size_t i = 0; i < num_txn_per_thread; i++) {
  //        instance->op("lookup_n", i % n_records, ret, (ret + 1), (ret + 2), (ret + 3), (ret + 4), (ret + 5), (ret +
  //        6),
  //                     (ret + 7), (ret + 8), (ret + 9));
  //      }
  //    });
  //
  //    printThroughput(runtime_ms, n_threads);
  //  }

  static void printThroughput(size_t runtime_ms, size_t n_threads, const std::string& prefix = "") {
    auto runtime_s = ((static_cast<double>(runtime_ms)) / 1000);
    auto throughput = ((num_txn_per_thread * n_threads) / runtime_s);
    auto avg_per_thread = (throughput / n_threads) / 1000000;
    LOG(INFO) << "Threads: " << n_threads << prefix
              << ": Throughput: " << ((num_txn_per_thread * n_threads) / runtime_s) / 1000000 << " MTPS"
              << " | avg-per-thread: " << avg_per_thread << " MTPS"
              << " | total_time: " << runtime_ms << "ms";
  }

  explicit YCSB(size_t num_columns = 1, size_t num_records = 16_M)
      : n_columns(num_columns), n_records(num_records), _n_ops(0), _builder(std::make_shared<dcds::Builder>("YCSB")) {
    auto ycsb_item = this->generateYCSB_Item();

    _builder->addAttributeArray("records", ycsb_item, num_records);

    this->generateUpdateFunction();
    this->generateLookupFunction();
    // this->generateLookupNFunction(num_ops_per_txn);

    // _builder->dump();
    _builder->injectCC();
    //    static bool printed = false;
    //    if(!printed){
    //      _builder->dump();
    //      printed = true;
    //    }

    //    LOG(INFO) << "######################--Building";
    _builder->build();

    instance = _builder->createInstance();
    // LOG(INFO) << "Instance: " << instance;
    // instance->listAllAvailableFunctions();
    //    LOG(INFO) << "warmup--";
    test_MT_lookup_sequential(1, false);
  }
  ~YCSB() {
    delete instance;
    _builder.reset();
  }

 private:
  const size_t n_columns;
  const size_t n_records;
  size_t _n_ops;
  std::shared_ptr<dcds::Builder> _builder;
  dcds::JitContainer* instance;
};

#endif  // DCDS_YCSB_HPP
