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
#include <string>
#include <thread>
#include <vector>

constexpr size_t initial_value = 100;
static std::string name = "Counter";
static auto op_name = name + "_fetch_add";
static auto op_get = name + "_get";

constexpr size_t iterations = 5;

static std::shared_ptr<dcds::Builder> generateCounter() {
  auto builder = std::make_shared<dcds::Builder>(name);
  // builder->addHint(dcds::hints::BuilderHints::SINGLE_THREADED);

  auto ctr_attr = builder->addAttribute("ctr", dcds::valueType::INT64, initial_value);

  // FIXME: this gets destroyed then there is a dangling ptr in addExpr.
  auto x = std::make_shared<dcds::expressions::Int64Constant>(1);

  // -- function create
  {
    auto fn = builder->createFunction(op_name, dcds::valueType::INT64);
    auto sb = fn->getStatementBuilder();
    auto tmpVar = fn->addTempVariable("tmp", dcds::valueType::INT64);

    sb->addReadStatement(ctr_attr, tmpVar);

    auto addExpr = std::make_shared<dcds::expressions::AddExpression>(tmpVar, x);
    sb->addUpdateStatement(ctr_attr, addExpr);
    sb->addReturnStatement(tmpVar);
  }
  // -- function end

  // -- function create
  {
    auto fn = builder->createFunction(op_get, dcds::valueType::INT64);
    auto sb = fn->getStatementBuilder();
    auto tmpVar = fn->addTempVariable("tmp", dcds::valueType::INT64);
    sb->addReadStatement(ctr_attr, tmpVar);
    sb->addReturnStatement(tmpVar);
  }
  // -- function end
  builder->injectCC();
  builder->build();
  return builder;
}

static size_t doOp(dcds::JitContainer* instance) { return std::any_cast<uint64_t>(instance->op(op_name)); }

static size_t test_ST(dcds::JitContainer* instance, size_t init_value) {
  size_t val = 0;
  size_t verifier = init_value;

  for (size_t i = 0; i < iterations; i++) {
    val = doOp(instance);
    LOG_IF(INFO, verifier != val) << "Value mismatch: val: " << val << " | verifier: " << verifier;
    verifier++;
  }

  return verifier;
}

static size_t test_MT(dcds::JitContainer* instance, size_t n_threads) {
  std::vector<std::thread> runners;

  std::barrier<void (*)()> sync_point(n_threads, []() {});

  for (size_t x = 0; x < n_threads; x++) {
    runners.emplace_back([&]() {
      LOG(INFO) << "tid: " << std::this_thread::get_id();
      sync_point.arrive_and_wait();  // not required i guess.
      for (size_t i = 0; i < iterations; i++) {
        doOp(instance);
      }
      LOG(INFO) << "DoneXX";
      sync_point.arrive_and_wait();  // not required i guess.
      LOG(INFO) << "DoneXX2";
    });
  }

  for (auto& th : runners) {
    th.join();
  }
  return std::any_cast<uint64_t>(instance->op(op_get));
}

static size_t test_MT2(dcds::JitContainer* instance, size_t n_threads) {
  auto thr = ThreadRunner(n_threads);

  thr([&]() {
    for (size_t i = 0; i < iterations; i++) {
      doOp(instance);
    }
  });

  return std::any_cast<uint64_t>(instance->op(op_get));
}

int main(int argc, char** argv) {
  dcds::InitializeLog(argc, argv);
  LOG(INFO) << "Counter";

  auto ctr = generateCounter();
  auto instance = ctr->createInstance();
  size_t currVal = 0;
  size_t expected_val = initial_value;

  currVal = test_ST(instance, expected_val);
  expected_val += iterations;
  LOG(INFO) << "currVal1 = " << currVal << " | expected: " << expected_val;

  currVal = test_ST(instance, expected_val);
  expected_val += iterations;
  LOG(INFO) << "currVal2 = " << currVal << " | expected: " << expected_val;
  //
  size_t n_threads = 4;
  currVal = test_MT(instance, n_threads);
  expected_val += (iterations * n_threads);
  LOG(INFO) << "currValMR = " << currVal << " | expected: " << expected_val;

  currVal = test_MT2(instance, n_threads);
  expected_val += (iterations * n_threads);
  LOG(INFO) << "currValMR2 = " << currVal << " | expected: " << expected_val;

  LOG(INFO) << "------- DONE";
  return 0;
}
