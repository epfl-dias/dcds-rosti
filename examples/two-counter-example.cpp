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

//
// Created by prathamesh on 14/8/23.
//

#include "dcds/builder/builder.hpp"
#include "dcds/context/DCDSContext.hpp"

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
    dsBuilder.addStatement(return_statement, read_fn);

    dsBuilder.addStatement(read_statement2, read_fn2);
    dsBuilder.addStatement(return_statement2, read_fn2);

    dsBuilder.addStatement(read_statement3, read_fn3);
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

int main() {
  DCDSCounter::initialize();

  DCDSCounter counter1(1, 2, 3);
  DCDSCounter counter2(5, 6, 7);

  int64_t val1 = 5555;
  int64_t val2 = 7777;
  int64_t val3 = 9999;
  int64_t val4 = 8888;
  int64_t val5 = 2222;
  int64_t val6 = 1111;

  LOG(INFO) << counter1.read();
  LOG(INFO) << counter1.read2();
  LOG(INFO) << counter1.read3();
  counter1.write(&val1, &val2, &val3);
  LOG(INFO) << counter1.read();
  LOG(INFO) << counter1.read2();
  LOG(INFO) << counter1.read3();

  LOG(INFO) << counter2.read();
  LOG(INFO) << counter2.read2();
  LOG(INFO) << counter2.read3();
  counter2.write(&val4, &val5, &val6);
  LOG(INFO) << counter2.read();
  LOG(INFO) << counter2.read2();
  LOG(INFO) << counter2.read3();
}
