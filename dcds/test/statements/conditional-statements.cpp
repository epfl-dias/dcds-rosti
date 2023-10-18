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

#include <gtest/gtest.h>

#include <dcds/dcds.hpp>

TEST(ConditionalStatementTest, IfAndElse) {
  std::string name = "ConditionalStatementTest_IfAndElse";
  auto op_name = name + "_op";

  auto builder = std::make_shared<dcds::Builder>(name);
  builder->addHint(dcds::hints::SINGLE_THREADED);

  // -- function create
  auto fn = builder->createFunction(op_name, dcds::BOOL);   // returns bool
  auto argOne = fn->addArgument("arg_one", dcds::INTEGER);  // function takes one argument

  // FIXME: return constant expressions instead of temp variables.
  auto tmpBoolTrue = fn->addTempVariable("temp_bool_true", dcds::BOOL, true);
  auto tmpBoolFalse = fn->addTempVariable("temp_bool_false", dcds::BOOL, false);

  auto sb = fn->getStatementBuilder();
  auto conditionalBlocks = sb->addConditionalBranch(new dcds::expressions::IsEvenExpression{argOne});

  conditionalBlocks.ifBlock->addReturnStatement("temp_bool_true");
  conditionalBlocks.elseBlock->addReturnStatement("temp_bool_false");

  // -- function end

  builder->build();
  auto instance = builder->createInstance();

  EXPECT_TRUE(std::any_cast<bool>(instance->op(op_name, 2)));
  EXPECT_FALSE(std::any_cast<bool>(instance->op(op_name, 3)));
  EXPECT_TRUE(std::any_cast<bool>(instance->op(op_name, 4)));
  EXPECT_FALSE(std::any_cast<bool>(instance->op(op_name, 5)));
}

TEST(ConditionalStatementTest, OnlyIf) {
  std::string name = "ConditionalStatementTest_OnlyIf";
  auto op_name = name + "_op";

  auto builder = std::make_shared<dcds::Builder>(name);
  builder->addHint(dcds::hints::SINGLE_THREADED);

  // -- function create
  auto fn = builder->createFunction(op_name, dcds::BOOL);   // returns bool
  auto argOne = fn->addArgument("arg_one", dcds::INTEGER);  // function takes one argument

  // FIXME: return constant expressions instead of temp variables.
  auto tmpBoolTrue = fn->addTempVariable("temp_bool_true", dcds::BOOL, true);
  auto tmpBoolFalse = fn->addTempVariable("temp_bool_false", dcds::BOOL, false);

  auto sb = fn->getStatementBuilder();
  auto conditionalBlocks = sb->addConditionalBranch(new dcds::expressions::IsEvenExpression{argOne});
  conditionalBlocks.ifBlock->addReturnStatement("temp_bool_true");

  sb->addReturnStatement("temp_bool_false");
  // -- function end

  builder->build();
  auto instance = builder->createInstance();

  EXPECT_TRUE(std::any_cast<bool>(instance->op(op_name, 2)));
  EXPECT_FALSE(std::any_cast<bool>(instance->op(op_name, 3)));
  EXPECT_TRUE(std::any_cast<bool>(instance->op(op_name, 4)));
  EXPECT_FALSE(std::any_cast<bool>(instance->op(op_name, 5)));
}

TEST(ConditionalStatementTest, MultipleBranches) {
  std::string name = "ConditionalStatementTest_MultipleBranches";
  auto op_name = name + "_op";

  auto builder = std::make_shared<dcds::Builder>(name);
  builder->addHint(dcds::hints::SINGLE_THREADED);

  // -- function create
  auto fn = builder->createFunction(op_name, dcds::BOOL);  // returns bool
  auto arg1 = fn->addArgument("arg_one", dcds::INTEGER);
  auto arg2 = fn->addArgument("arg_two", dcds::INTEGER);

  // FIXME: return constant expressions instead of temp variables.
  auto tmpBoolTrue = fn->addTempVariable("temp_bool_true", dcds::BOOL, true);
  auto tmpBoolFalse = fn->addTempVariable("temp_bool_false", dcds::BOOL, false);

  auto sb = fn->getStatementBuilder();

  auto cond1 = sb->addConditionalBranch(new dcds::expressions::IsEvenExpression{arg1});
  cond1.ifBlock->addLogStatement("1: It is even branch");
  cond1.elseBlock->addLogStatement("1: It is odd branch");

  auto cond2 = sb->addConditionalBranch(new dcds::expressions::IsEvenExpression{arg2});
  cond2.ifBlock->addReturnStatement("temp_bool_true");
  cond2.elseBlock->addReturnStatement("temp_bool_false");

  // -- function end

  builder->build();
  auto instance = builder->createInstance();

  EXPECT_TRUE(std::any_cast<bool>(instance->op(op_name, 2, 2)));
  EXPECT_FALSE(std::any_cast<bool>(instance->op(op_name, 2, 3)));
  EXPECT_TRUE(std::any_cast<bool>(instance->op(op_name, 3, 2)));
  EXPECT_FALSE(std::any_cast<bool>(instance->op(op_name, 5, 5)));
}

TEST(ConditionalStatementTest, NestedBranches) {
  std::string name = "ConditionalStatementTest_NestedBranches";
  auto op_name = name + "_op";

  auto builder = std::make_shared<dcds::Builder>(name);
  builder->addHint(dcds::hints::SINGLE_THREADED);

  // -- function create
  auto fn = builder->createFunction(op_name, dcds::BOOL);  // returns bool
  auto arg1 = fn->addArgument("arg_one", dcds::INTEGER);
  auto arg2 = fn->addArgument("arg_two", dcds::INTEGER);

  // FIXME: return constant expressions instead of temp variables.
  auto tmpBoolTrue = fn->addTempVariable("temp_bool_true", dcds::BOOL, true);
  auto tmpBoolFalse = fn->addTempVariable("temp_bool_false", dcds::BOOL, false);

  auto sb = fn->getStatementBuilder();

  auto cond1 = sb->addConditionalBranch(new dcds::expressions::IsEvenExpression{fn->getArgument("arg_one")});
  cond1.ifBlock->addLogStatement("1: arg_one is even");

  auto cond2 = cond1.ifBlock->addConditionalBranch(new dcds::expressions::IsEvenExpression{fn->getArgument("arg_two")});
  cond2.ifBlock->addLogStatement("arg_two is even");
  cond2.ifBlock->addReturnStatement("temp_bool_true");

  cond2.elseBlock->addLogStatement("arg_two is odd");
  cond2.elseBlock->addReturnStatement("temp_bool_false");

  cond1.elseBlock->addLogStatement("1: arg_one is odd");
  cond1.elseBlock->addReturnStatement("temp_bool_false");

  // -- function end

  builder->build();
  auto instance = builder->createInstance();

  // cond1::if
  EXPECT_TRUE(std::any_cast<bool>(instance->op(op_name, 2, 2)));
  EXPECT_FALSE(std::any_cast<bool>(instance->op(op_name, 2, 3)));

  // cond1::else
  EXPECT_FALSE(std::any_cast<bool>(instance->op(op_name, 3, 2)));
  EXPECT_FALSE(std::any_cast<bool>(instance->op(op_name, 5, 5)));
}