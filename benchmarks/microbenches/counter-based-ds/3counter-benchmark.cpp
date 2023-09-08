//
// Created by prathamesh on 28/8/23.
//

#include <benchmark/benchmark.h>

#include <dcds/builder/builder.hpp>
#include <dcds/context/DCDSContext.hpp>

class DCDSCounter {
 public:
  DCDSCounter(int counterInitialValue1 = 0, int counterInitialValue2 = 0, int counterInitialValue3 = 0) {
    dcds::DCDSContext context(false, false);
    dcds::Builder dsBuilder(context, "Counter");

    dsBuilder.addAttribute("counter_value", dcds::INTEGER, counterInitialValue1);
    dsBuilder.addAttribute("counter_value2", dcds::INTEGER, counterInitialValue2);
    dsBuilder.addAttribute("counter_value3", dcds::INTEGER, counterInitialValue3);

    storageObject = dsBuilder.initializeStorage();
    storageObjectPtr = reinterpret_cast<void *>(storageObject.get());
  }

  static void initialize() {
    dcds::DCDSContext context(false, false);
    dcds::Builder dsBuilder(context, "Counter");

    dsBuilder.addAttribute("counter_value", dcds::INTEGER, 0);
    dsBuilder.addAttribute("counter_value2", dcds::INTEGER, 0);
    dsBuilder.addAttribute("counter_value3", dcds::INTEGER, 0);

    auto read_fn = dsBuilder.createFunction("read_function", dcds::valueType::INTEGER);
    auto read_fn2 = dsBuilder.createFunction("read_function2", dcds::valueType::INTEGER);
    auto read_fn3 = dsBuilder.createFunction("read_function3", dcds::valueType::INTEGER);
    auto write_fn = dsBuilder.createFunction("write_function", dcds::valueType::VOID, dcds::valueType::INTEGER,
                                             dcds::valueType::INTEGER, dcds::valueType::INTEGER);

    dsBuilder.addTempVar("counter_value_read_variable", dcds::valueType::INTEGER, 0, read_fn);
    dsBuilder.addTempVar("counter_value_read_variable2", dcds::valueType::INTEGER, 0, read_fn2);
    dsBuilder.addTempVar("counter_value_read_variable3", dcds::valueType::INTEGER, 0, read_fn3);

    dsBuilder.addTempVar("one_1", dcds::valueType::INTEGER, 1, read_fn);
    dsBuilder.addTempVar("one_2", dcds::valueType::INTEGER, 1, read_fn2);
    dsBuilder.addTempVar("one_3", dcds::valueType::INTEGER, 1, read_fn3);

    // TODO: Add assertion to see that the user provides an arg with `addArgVar` if the function expects arguments.
    dsBuilder.addArgVar("counter_value_write_variable", dcds::valueType::INTEGER, write_fn);
    dsBuilder.addArgVar("counter_value_write_variable2", dcds::valueType::INTEGER, write_fn);
    dsBuilder.addArgVar("counter_value_write_variable3", dcds::valueType::INTEGER, write_fn);

    auto read_statement =
        dsBuilder.createReadStatement(dsBuilder.getAttribute("counter_value"), "counter_value_read_variable");
    auto read_statement2 =
        dsBuilder.createReadStatement(dsBuilder.getAttribute("counter_value2"), "counter_value_read_variable2");
    auto read_statement3 =
        dsBuilder.createReadStatement(dsBuilder.getAttribute("counter_value3"), "counter_value_read_variable3");

    auto add_statement =
        dsBuilder.createTempVarAddStatement("counter_value_read_variable", "one_1", "counter_value_read_variable");
    auto add_statement2 =
        dsBuilder.createTempVarAddStatement("counter_value_read_variable2", "one_2", "counter_value_read_variable2");
    auto add_statement3 =
        dsBuilder.createTempVarAddStatement("counter_value_read_variable3", "one_3", "counter_value_read_variable3");

    auto write_statement_rf1 =
        dsBuilder.createUpdateStatement(dsBuilder.getAttribute("counter_value"), "counter_value_read_variable");
    auto write_statement2_rf2 =
        dsBuilder.createUpdateStatement(dsBuilder.getAttribute("counter_value2"), "counter_value_read_variable2");
    auto write_statement3_rf3 =
        dsBuilder.createUpdateStatement(dsBuilder.getAttribute("counter_value3"), "counter_value_read_variable3");

    auto write_statement =
        dsBuilder.createUpdateStatement(dsBuilder.getAttribute("counter_value"), "counter_value_write_variable");
    auto write_statement2 =
        dsBuilder.createUpdateStatement(dsBuilder.getAttribute("counter_value2"), "counter_value_write_variable2");
    auto write_statement3 =
        dsBuilder.createUpdateStatement(dsBuilder.getAttribute("counter_value3"), "counter_value_write_variable3");
    auto return_statement = dsBuilder.createReturnStatement("counter_value_read_variable");
    auto return_statement2 = dsBuilder.createReturnStatement("counter_value_read_variable2");
    auto return_statement3 = dsBuilder.createReturnStatement("counter_value_read_variable3");

    dsBuilder.addStatement(read_statement, read_fn);
    dsBuilder.addStatement(add_statement, read_fn);
    dsBuilder.addStatement(write_statement_rf1, read_fn);
    dsBuilder.addStatement(return_statement, read_fn);

    dsBuilder.addStatement(read_statement2, read_fn2);
    dsBuilder.addStatement(add_statement2, read_fn2);
    dsBuilder.addStatement(write_statement2_rf2, read_fn2);
    dsBuilder.addStatement(return_statement2, read_fn2);

    dsBuilder.addStatement(read_statement3, read_fn3);
    dsBuilder.addStatement(add_statement3, read_fn3);
    dsBuilder.addStatement(write_statement3_rf3, read_fn3);
    dsBuilder.addStatement(return_statement3, read_fn3);

    dsBuilder.addStatement(write_statement, write_fn);
    dsBuilder.addStatement(write_statement2, write_fn);
    dsBuilder.addStatement(write_statement3, write_fn);

    dsBuilder.addFunction(read_fn);
    dsBuilder.addFunction(read_fn2);
    dsBuilder.addFunction(read_fn3);
    dsBuilder.addFunction(write_fn);

    visitor = dsBuilder.codegen();

    built_read_fn = visitor->getBuiltFunctionInt64ReturnType("read_function");
    built_read_fn2 = visitor->getBuiltFunctionInt64ReturnType("read_function2");
    built_read_fn3 = visitor->getBuiltFunctionInt64ReturnType("read_function3");
    built_write_fn = visitor->getBuiltFunctionVoidReturnTypeIntArg3("write_function");
  }

  auto read() { return built_read_fn(storageObjectPtr); }

  auto read2() { return built_read_fn2(storageObjectPtr); }

  auto read3() { return built_read_fn3(storageObjectPtr); }

  auto write(int64_t *counterWriteValue, int64_t *p, int64_t *q) {
    built_write_fn(storageObjectPtr, counterWriteValue, p, q);
  }

 private:
  static int64_t (*built_read_fn)(void *);
  static int64_t (*built_read_fn2)(void *);
  static int64_t (*built_read_fn3)(void *);
  static void (*built_write_fn)(void *, int64_t *, int64_t *, int64_t *);
  static std::shared_ptr<dcds::Visitor> visitor;
  std::shared_ptr<dcds::StorageLayer> storageObject;
  void *storageObjectPtr;
};

int64_t (*DCDSCounter::built_read_fn)(void *);
int64_t (*DCDSCounter::built_read_fn2)(void *);
int64_t (*DCDSCounter::built_read_fn3)(void *);
void (*DCDSCounter::built_write_fn)(void *, int64_t *, int64_t *, int64_t *);
std::shared_ptr<dcds::Visitor> DCDSCounter::visitor;

static void benchmark_dcds_counter(benchmark::State &state) {
  DCDSCounter dcds_counter(0, 0, 0);
  int64_t record_var1 = 0, record_var2 = 0, record_var = 0;

  for (auto _ : state) {
    for (int i = 0; i < state.range(0); ++i) {
#pragma clang optimize off

      record_var += dcds_counter.read() + i;
      record_var1 += record_var + dcds_counter.read2() + i;
      record_var2 += record_var1 + dcds_counter.read3() + i;
      dcds_counter.write(&record_var, &record_var1, &record_var2);

#pragma clang optimize on
    }
  }
}

class counter_class {
 public:
  counter_class(int64_t counter_var1_, int64_t counter_var2_, int64_t counter_var3_)
      : counter_var1(counter_var1_), counter_var2(counter_var2_), counter_var3(counter_var3_) {}

  auto getCounterVar1() {
    m.lock();
    auto c = ++counter_var1;
    m.unlock();
    return c;
  }

  auto getCounterVar2() {
    m.lock();
    auto c = ++counter_var2;
    m.unlock();
    return c;
  }

  auto getCounterVar3() {
    m.lock();
    auto c = ++counter_var3;
    m.unlock();
    return c;
  }

  auto setCounterVar(int64_t c1, int64_t c2, int64_t c3) {
    m.lock();
    counter_var1 = c1;
    m.unlock();

    m.lock();
    counter_var2 = c2;
    m.unlock();

    m.lock();
    counter_var3 = c3;
    m.unlock();
  }

 private:
  int64_t counter_var1, counter_var2, counter_var3;
  std::mutex m;
};

static void benchmark_normal_counter(benchmark::State &state) {
  counter_class counter(0, 0, 0);
  int64_t record_var = 0, record_var1 = 0, record_var2 = 0;

  for (auto _ : state) {
    for (int i = 0; i < state.range(0); ++i) {
#pragma clang optimize off

      record_var += counter.getCounterVar1() + i;
      record_var1 += record_var + counter.getCounterVar2() + i;
      record_var2 += record_var1 + counter.getCounterVar3() + i;

      counter.setCounterVar(record_var, record_var1, record_var2);

#pragma clang optimize on
    }
  }
}

static void assertBothCountersProduceEquivalentResults() {
  counter_class counter(0, 0, 0);
  DCDSCounter dcds_counter(0, 0, 0);

  int64_t record_var = 0, record_var1 = 0, record_var2 = 0;
  int64_t record_var1_dcds = 0, record_var2_dcds = 0, record_var_dcds = 0;

  for (int i = 0; i < 1000; ++i) {
    record_var_dcds += dcds_counter.read() + i;
    record_var1_dcds += record_var_dcds + dcds_counter.read2() + i;
    record_var2_dcds += record_var1_dcds + dcds_counter.read3() + i;
    dcds_counter.write(&record_var_dcds, &record_var1_dcds, &record_var2_dcds);

    record_var += counter.getCounterVar1() + i;
    record_var1 += record_var + counter.getCounterVar2() + i;
    record_var2 += record_var1 + counter.getCounterVar3() + i;
    counter.setCounterVar(record_var, record_var1, record_var2);

    assert(dcds_counter.read() == counter.getCounterVar1());
    assert(dcds_counter.read2() == counter.getCounterVar2());
    assert(dcds_counter.read3() == counter.getCounterVar3());
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
