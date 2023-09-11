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

      This code is distribu.ted in the hope that it will be useful, but
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

    // TODO: Add assertion to see that the user provides an arg with `addArgVar` if the function expects arguments.
    dsBuilder.addArgVar("counter_value_update_variable", dcds::valueType::INTEGER, update_fn);
    dsBuilder.addTempVar("compare_variable", dcds::valueType::INTEGER, 5, update_fn);
    dsBuilder.addTempVar("temp_read_variable", dcds::valueType::INTEGER, 0, update_fn);

    auto read_statement =
        dsBuilder.createReadStatement(dsBuilder.getAttribute("counter_value"), "counter_value_read_variable");
    auto read_statement2 = dsBuilder.createReadStatement(dsBuilder.getAttribute("counter_value"), "temp_read_variable");
    auto cond = dcds::ConditionBuilder(dcds::CmpIPredicate::neq, "counter_value_update_variable", "compare_variable");

    std::vector<std::shared_ptr<dcds::StatementBuilder>> ifStatements, elseStatements;
    auto ifStatement1 = dsBuilder.createTempVarAddStatement("counter_value_update_variable", "compare_variable",
                                                            "counter_value_update_variable");
    auto elseStatement1 = dsBuilder.createTempVarAddStatement(
        "counter_value_update_variable", "counter_value_update_variable", "counter_value_update_variable");
    ifStatements.emplace_back(ifStatement1);
    elseStatements.emplace_back(elseStatement1);

    auto cond_statement = dsBuilder.createConditionStatement(cond, ifStatements, elseStatements);
    auto temp_add_statement = dsBuilder.createTempVarAddStatement("counter_value_update_variable", "temp_read_variable",
                                                                  "counter_value_update_variable");
    auto write_statement =
        dsBuilder.createUpdateStatement(dsBuilder.getAttribute("counter_value"), "counter_value_update_variable");
    auto return_statement = dsBuilder.createReturnStatement("counter_value_read_variable");

    dsBuilder.addStatement(read_statement, read_fn);
    dsBuilder.addStatement(return_statement, read_fn);

    dsBuilder.addStatement(read_statement2, update_fn);
    dsBuilder.addStatement(cond_statement, update_fn);
    dsBuilder.addStatement(temp_add_statement, update_fn);
    dsBuilder.addStatement(write_statement, update_fn);

    dsBuilder.addFunction(read_fn);
    dsBuilder.addFunction(update_fn);

    visitor = dsBuilder.codegen();

    built_read_fn = visitor->getBuiltFunctionInt64ReturnType("read_function");
    built_update_fn = visitor->getBuiltFunctionVoidReturnTypeIntArg1("update_function");
  }

  auto read() { return built_read_fn(storageObjectPtr); }

  auto update(int64_t *update_value_half) { return built_update_fn(storageObjectPtr, update_value_half); }

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

int main() {
  DCDSCounter::initialize();

  DCDSCounter counter1(1);

  int64_t val1 = 7;
  int64_t val2 = 5;

  LOG(INFO) << counter1.read();
  counter1.update(&val1);
  LOG(INFO) << counter1.read();
  counter1.update(&val2);
  LOG(INFO) << counter1.read();
}
