//
// Created by prathamesh on 28/8/23.
//

#include <benchmark/benchmark.h>

#include <dcds/builder/builder.hpp>
#include <dcds/context/DCDSContext.hpp>

class DCDSCounter {
 public:
  DCDSCounter(int counterInitialValue1 = 0) {
    dcds::DCDSContext context(false, false);
    dcds::Builder dsBuilder(context, "Counter");

    dsBuilder.addAttribute("counter_value", dcds::INTEGER, counterInitialValue1);

    storageObject = dsBuilder.initializeStorage();
    storageObjectPtr = reinterpret_cast<void *>(storageObject.get());
  }

  static void initialize() {
    dcds::DCDSContext context(false, false);
    dcds::Builder dsBuilder(context, "Counter");

    dsBuilder.addAttribute("counter_value", dcds::INTEGER, 0);

    auto read_fn = dsBuilder.createFunction("read_function", dcds::valueType::INTEGER);
    auto update_fn = dsBuilder.createFunction("update_function", dcds::valueType::VOID, dcds::valueType::INTEGER);

    dsBuilder.addTempVar("counter_value_read_variable", dcds::valueType::INTEGER, 0, read_fn);
    dsBuilder.addTempVar("one_1", dcds::valueType::INTEGER, 1, read_fn);

    // TODO: Add assertion to see that the user provides an arg with `addArgVar` if the function expects arguments.
    dsBuilder.addArgVar("counter_value_update_variable", dcds::valueType::INTEGER, update_fn);

    auto read_statement =
        dsBuilder.createReadStatement(dsBuilder.getAttribute("counter_value"), "counter_value_read_variable");
    auto add_statement =
        dsBuilder.createTempVarAddStatement("counter_value_read_variable", "one_1", "counter_value_read_variable");
    auto write_statement =
        dsBuilder.createUpdateStatement(dsBuilder.getAttribute("counter_value"), "counter_value_update_variable");
    auto write_statement_rf1 =
        dsBuilder.createUpdateStatement(dsBuilder.getAttribute("counter_value"), "counter_value_read_variable");
    auto return_statement = dsBuilder.createReturnStatement("counter_value_read_variable");

    dsBuilder.addStatement(read_statement, read_fn);
    dsBuilder.addStatement(add_statement, read_fn);
    dsBuilder.addStatement(write_statement_rf1, read_fn);
    dsBuilder.addStatement(return_statement, read_fn);

    dsBuilder.addStatement(write_statement, update_fn);

    dsBuilder.addFunction(read_fn);
    dsBuilder.addFunction(update_fn);

    visitor = dsBuilder.codegen();

    built_read_fn = visitor->getBuiltFunctionInt64ReturnType("read_function");
    built_update_fn = visitor->getBuiltFunctionVoidReturnTypeIntArg1("update_function");
  }

  auto read() { return built_read_fn(storageObjectPtr); }

  auto update(int64_t *update_value) { return built_update_fn(storageObjectPtr, update_value); }

 private:
  static int64_t (*built_read_fn)(void *);
  static void (*built_update_fn)(void *, int64_t *);
  static std::shared_ptr<dcds::Visitor> visitor;
  std::shared_ptr<dcds::StorageLayer> storageObject;
  void *storageObjectPtr;
};

int64_t (*DCDSCounter::built_read_fn)(void *);
void (*DCDSCounter::built_update_fn)(void *, int64_t *);
std::shared_ptr<dcds::Visitor> DCDSCounter::visitor;

static void benchmark_dcds_counter(benchmark::State &state) {
  DCDSCounter dcds_counter(0);
  int64_t record_var = 0;

  for (auto _ : state) {
    for (int i = 0; i < state.range(0); ++i) {
#pragma clang optimize off
      record_var += dcds_counter.read() + i;
      dcds_counter.update(&record_var);
#pragma clang optimize on
    }
  }
}

class counter_class {
 public:
  counter_class(int counter_var_) : counter_var(counter_var_) {}

  auto getCounterVar() {
    m.lock();
    auto c = ++counter_var;
    m.unlock();
    return c;
  }

  auto setCounterVar(int64_t c) {
    m.lock();
    counter_var = c;
    m.unlock();
  }

 private:
  int64_t counter_var;
  std::mutex m;
};

static void benchmark_normal_counter(benchmark::State &state) {
  counter_class counter(0);
  int64_t record_var = 0;

  for (auto _ : state) {
    for (int i = 0; i < state.range(0); ++i) {
#pragma clang optimize off
      record_var += counter.getCounterVar() + i;
      counter.setCounterVar(record_var);
#pragma clang optimize on
    }
  }
}

static void assertBothCountersProduceEquivalentResults() {
  counter_class counter(0);
  DCDSCounter dcds_counter(0);

  int64_t record_var = 0;
  int64_t record_var1_dcds = 0;

  for (int i = 0; i < 1000; ++i) {
    record_var1_dcds += dcds_counter.read() + i;
    record_var += counter.getCounterVar() + i;

    assert(dcds_counter.read() == counter.getCounterVar());
  }
}

int main(int argc, char **argv) {
  DCDSCounter::initialize();

  BENCHMARK(benchmark_dcds_counter)->Arg(1)->Unit(benchmark::kMicrosecond);
  BENCHMARK(benchmark_normal_counter)->Arg(1)->Unit(benchmark::kMicrosecond);

  BENCHMARK(benchmark_dcds_counter)->Arg(4)->Unit(benchmark::kMicrosecond);
  BENCHMARK(benchmark_normal_counter)->Arg(4)->Unit(benchmark::kMicrosecond);

  BENCHMARK(benchmark_dcds_counter)->Arg(8)->Unit(benchmark::kMicrosecond);
  BENCHMARK(benchmark_normal_counter)->Arg(8)->Unit(benchmark::kMicrosecond);

  BENCHMARK(benchmark_dcds_counter)->Arg(16)->Unit(benchmark::kMicrosecond);
  BENCHMARK(benchmark_normal_counter)->Arg(16)->Unit(benchmark::kMicrosecond);

  assertBothCountersProduceEquivalentResults();

  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();
}
