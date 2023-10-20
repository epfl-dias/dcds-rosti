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

std::string test_name_prefix = "FunctionBuilderTest_";

TEST(FunctionBuilderTest, Basic) {
  std::string name = test_name_prefix + "Basic";
  auto op_name = name + "_op";
  uint64_t initial_value = 100;

  auto builder = std::make_shared<dcds::Builder>(name);
  builder->addHint(dcds::hints::BuilderHints::SINGLE_THREADED);

  auto int64_attr = builder->addAttribute("int64_attribute", dcds::valueType::INT64, initial_value);

  // -- function create
  auto fn = builder->createFunction(op_name, dcds::valueType::INT64);
  auto sb = fn->getStatementBuilder();
  auto tmpVar = fn->addTempVariable("tmp", dcds::valueType::INT64);

  sb->addReadStatement(int64_attr, tmpVar);
  sb->addReturnStatement(tmpVar);

  // -- function end

  builder->build();
  auto instance = builder->createInstance();

  EXPECT_EQ(std::any_cast<uint64_t>(instance->op(op_name)), initial_value);
}

TEST(FunctionBuilderTest, NonVoidReturn) {
  std::string name = test_name_prefix + "PtrTypeArgument";
  auto op_name = name + "_op";
  uint64_t initial_value = 0;

  auto builder = std::make_shared<dcds::Builder>(name);
  builder->addHint(dcds::hints::BuilderHints::SINGLE_THREADED);

  auto int64_attr = builder->addAttribute("int64_attribute", dcds::valueType::INT64, initial_value);

  // -- function create
  auto fn = builder->createFunction(op_name, dcds::valueType::INT64);
  auto argOne = fn->addArgument("arg_one", dcds::valueType::INT64);

  auto sb = fn->getStatementBuilder();
  auto tmpVar = fn->addTempVariable("tmp", dcds::valueType::INT64);

  sb->addReadStatement(int64_attr, tmpVar);
  sb->addUpdateStatement(int64_attr, "arg_one");

  sb->addReturnStatement(tmpVar);

  // -- function end

  builder->build();
  auto instance = builder->createInstance();

  EXPECT_EQ(std::any_cast<uint64_t>(instance->op(op_name, 10)), 0);
  EXPECT_EQ(std::any_cast<uint64_t>(instance->op(op_name, 20)), 10);
  EXPECT_EQ(std::any_cast<uint64_t>(instance->op(op_name, 30)), 20);
  EXPECT_EQ(std::any_cast<uint64_t>(instance->op(op_name, 40)), 30);
  EXPECT_EQ(std::any_cast<uint64_t>(instance->op(op_name, 50)), 40);
}

TEST(FunctionBuilderTest, PtrTypeArgumentUpdate) {
  std::string name = test_name_prefix + "PtrTypeArgumentUpdate";
  auto op_name = name + "_op";
  uint64_t initial_value = 50;

  auto builder = std::make_shared<dcds::Builder>(name);
  builder->addHint(dcds::hints::BuilderHints::SINGLE_THREADED);

  auto int64_attr = builder->addAttribute("int64_attribute", dcds::valueType::INT64, initial_value);

  // -- function create
  auto fn = builder->createFunction(op_name, dcds::valueType::INT64);
  auto argOne = fn->addArgument("arg_one", dcds::valueType::INT64, true);

  auto sb = fn->getStatementBuilder();
  auto tmpVar = fn->addTempVariable("tmp", dcds::valueType::INT64);

  sb->addReadStatement(int64_attr, argOne);
  sb->addReadStatement(int64_attr, tmpVar);
  sb->addReturnStatement("tmp");

  // -- function end

  builder->build();
  auto instance = builder->createInstance();

  uint64_t input = 0;
  EXPECT_EQ(input, 0);
  EXPECT_EQ(std::any_cast<uint64_t>(instance->op(op_name, &input)), initial_value);
  EXPECT_EQ(input, initial_value);
}

TEST(FunctionBuilderTest, PtrTypeArgumentRead) {
  std::string name = test_name_prefix + "PtrTypeArgumentRead";
  auto op_name = name + "_op";
  uint64_t initial_value = 0;

  auto builder = std::make_shared<dcds::Builder>(name);
  builder->addHint(dcds::hints::BuilderHints::SINGLE_THREADED);

  auto int64_attr = builder->addAttribute("int64_attribute", dcds::valueType::INT64, initial_value);

  // -- function create
  auto fn = builder->createFunction(op_name, dcds::valueType::INT64);
  auto argOne = fn->addArgument("arg_one", dcds::valueType::INT64, true);

  auto sb = fn->getStatementBuilder();
  auto tmpVar = fn->addTempVariable("tmp", dcds::valueType::INT64);

  sb->addReadStatement(int64_attr, tmpVar);
  sb->addUpdateStatement(int64_attr, "arg_one");

  sb->addReturnStatement(tmpVar);

  // -- function end

  builder->build();
  auto instance = builder->createInstance();

  uint64_t x = 10;
  EXPECT_EQ(std::any_cast<uint64_t>(instance->op(op_name, &x)), UINT64_C(0));
  x = 20;
  EXPECT_EQ(std::any_cast<uint64_t>(instance->op(op_name, &x)), UINT64_C(10));
  x = 30;
  EXPECT_EQ(std::any_cast<uint64_t>(instance->op(op_name, &x)), UINT64_C(20));
  x = 40;
  EXPECT_EQ(std::any_cast<uint64_t>(instance->op(op_name, &x)), UINT64_C(30));
  x = 50;
  EXPECT_EQ(std::any_cast<uint64_t>(instance->op(op_name, &x)), UINT64_C(40));
}