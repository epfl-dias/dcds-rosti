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
constexpr size_t initial_value = 100;
static std::string name = "Counter";
static auto op_name = name + "_fetch_add";
static auto op_get = name + "_get";

constexpr size_t iterations = 5;
const size_t num_threads = std::thread::hardware_concurrency();

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
  size_t val;
  size_t verifier = init_value;

  for (size_t i = 0; i < iterations; i++) {
    val = doOp(instance);
    LOG_IF(INFO, verifier != val) << "Value mismatch: val: " << val << " | verifier: " << verifier;
    verifier++;
  }

  return verifier;
}

static size_t test_MT(dcds::JitContainer* instance, size_t n_threads) {
  auto thr = dcds::ThreadRunner(n_threads);

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
  ctr->dump();
  auto instance = ctr->createInstance();
  size_t current_value;
  size_t expected_value = initial_value;

  current_value = test_ST(instance, expected_value);
  expected_value += iterations;
  CHECK(current_value == expected_value) << "current_value != expected_value :: " << current_value
                                       << " != " << expected_value;

  current_value = test_ST(instance, expected_value);
  expected_value += iterations;
  CHECK(current_value == expected_value) << "current_value != expected_value :: " << current_value
                                       << " != " << expected_value;


  current_value = test_MT(instance, num_threads);
  expected_value += (iterations * num_threads);
  CHECK(current_value == expected_value) << "(multi-threaded) current_value != expected_value :: " << current_value
                                       << " != " << expected_value;

  return 0;
}
