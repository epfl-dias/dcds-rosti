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
// Created by prathamesh on 10/7/23.
//

#include "dcds/builder/builder.hpp"
#include "dcds/codegen/codegen.hpp"
#include "dcds/context/DCDSContext.hpp"

static void createCounter() {
  // context(enable_multi_threading_flag, allow_dangling_functions_flag);
  dcds::DCDSContext context(false, false);

  // dsBuilder(dcds_context, data_structure_name);
  dcds::Builder dsBuilder(context, "Counter");

  // TODO: Develop support for specifying default value with types other than int here.
  // addAttribute(attribute_name, attribute_type, attribute_initial_value);
  dsBuilder.addAttribute("counter_value", dcds::INTEGER, 2);

  // createFunction(function_name, function_return_type, function_arguments_type);
  auto usr_fn = dsBuilder.createFunction("update_and_read_fn", dcds::INTEGER);
  auto check_fn = dsBuilder.createFunction("check_function");

  int64_t initValueCounter = 0;
  int64_t counterValueWrite = 55;

  // addTempVar(temp_var_name, temp_var_type, temp_var_initial_value);
  dsBuilder.addTempVar("counter_value_read_variable", dcds::INTEGER, initValueCounter, usr_fn);
  dsBuilder.addTempVar("counter_value_write_variable", dcds::INTEGER, counterValueWrite, usr_fn);

  dsBuilder.addTempVar("dummy_var1", dcds::INTEGER, 10, usr_fn);
  dsBuilder.addTempVar("dummy_var2", dcds::INTEGER, 11, usr_fn);
  dsBuilder.addTempVar("dummy_var3", dcds::INTEGER, 100, usr_fn);

  // createReadStatement(source_attribute_from_where_value_will_be_read, temp_variable_storing_read_value);
  auto read_statement =
      dsBuilder.createReadStatement(dsBuilder.getAttribute("counter_value"), "counter_value_read_variable");

  // TODO: One statement can only be attached to one function atm. This can be solved once frontend support for this is
  // added, backend is expected to work with minor changes. Relatively easy thing to fix.
  auto read_statement_for_check_fn =
      dsBuilder.createReadStatement(dsBuilder.getAttribute("counter_value"), "counter_value_read_variable");

  // createUpdateStatement(source_attribute_which_will_be_updated, temp_variable_storing_write_value);
  auto update_statement =
      dsBuilder.createUpdateStatement(dsBuilder.getAttribute("counter_value"), "counter_value_write_variable");

  auto read_statement2 =
      dsBuilder.createReadStatement(dsBuilder.getAttribute("counter_value"), "counter_value_read_variable");

  // createTempVarAddStatement(left_operand, right_operand, result_storing_variable);
  auto tempVarAddStatement1 = dsBuilder.createTempVarAddStatement("dummy_var1", "dummy_var2", "dummy_var1");
  auto tempVarAddStatement2 = dsBuilder.createTempVarAddStatement("dummy_var1", "dummy_var2", "dummy_var3");

  // ConditionBuilder(condition_predicate, comparison_variable1, comparison_variable2);
  auto cond = dcds::ConditionBuilder(dcds::CmpIPredicate::neq, "dummy_var1", "dummy_var2");
  std::vector<std::shared_ptr<dcds::StatementBuilder>> ifResStatements, elseResStatements;

  ifResStatements.emplace_back(tempVarAddStatement1);
  ifResStatements.emplace_back(tempVarAddStatement2);
  elseResStatements.emplace_back(tempVarAddStatement2);

  // createConditionStatement(condition, if_block, else_block);
  auto conditionStatement = dsBuilder.createConditionStatement(cond, ifResStatements, elseResStatements);

  // createReturnStatement(name_of_temp_variable_to_be_returned);
  auto return_statement = dsBuilder.createReturnStatement("counter_value_read_variable");

  // addStatement(statement_shared_ptr, function_shared_ptr);
  dsBuilder.addStatement(read_statement, usr_fn);
  dsBuilder.addStatement(update_statement, usr_fn);
  dsBuilder.addStatement(read_statement2, usr_fn);
  dsBuilder.addStatement(tempVarAddStatement1, usr_fn);
  dsBuilder.addStatement(tempVarAddStatement2, usr_fn);
  dsBuilder.addStatement(conditionStatement, usr_fn);
  dsBuilder.addStatement(return_statement, usr_fn);

  // addFunction(function_shared_ptr);
  dsBuilder.addFunction(usr_fn);

  dsBuilder.addStatement(read_statement_for_check_fn, check_fn);
  dsBuilder.addFunction(check_fn);

  // Function to automatically initialise storage layer from the user supplied information.
  auto storageObject = dsBuilder.initializeStorage();

  // codegen(builder_object);
  auto visitor = codegen(dsBuilder);
  visitor->build();

  // TODO: Develop support for custom user arguments to data structure functions.
  auto built_fn = visitor->getBuiltFunctionInt64ReturnType("update_and_read_fn");
  auto storageObjectPtr = reinterpret_cast<void *>(storageObject);

  // LOG(INFO) << built_fn(storageObjectPtr);
}

int main() { createCounter(); }

// TODO: This is outdated at the moment.
//// Expected program in C++ realm
//
// int counter_value = 2;
//
// int update_and_read_fn()
//{
//    int counter_value_read_varaiable = 0;
//    int counter_value_write_variable = 1;
//
//    counter_value_read_varaiable = counter_value; // First read statement
//    counter_value = counter_value_write_variable; // Update/Write statement
//    counter_value_write_variable = counter_value; // Second read statement
//
//    return counter_value_read_varaiable; // Return statement
//}
//
// update_and_read_fn();
