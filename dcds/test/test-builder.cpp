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

// TEST(ExampleTest, DoesNothing) { EXPECT_EQ(1, 1); }

TEST(BuilderTest, NoAttributeBuild) {
  std::string name = "BuilderTest_NoAttributeBuild";
  auto op_name = name + "_op";

  auto builder = std::make_shared<dcds::Builder>(name);
  builder->addHint(dcds::hints::SINGLE_THREADED);

  // -- function create
  auto fn = builder->createFunction(op_name, dcds::BOOL);  // returns bool
  auto argOne = fn->addArgument("arg_one", dcds::INT64);   // function takes one argument

  auto sb = fn->getStatementBuilder();
  sb->addReturnStatement(std::make_shared<dcds::expressions::BoolConstant>(true));
  // -- function end

  builder->build();
  auto instance = builder->createInstance();

  EXPECT_TRUE(std::any_cast<bool>(instance->op(op_name, 1)));
  EXPECT_TRUE(std::any_cast<bool>(instance->op(op_name, 2)));
  EXPECT_TRUE(std::any_cast<bool>(instance->op(op_name, 3)));
  EXPECT_TRUE(std::any_cast<bool>(instance->op(op_name, 4)));
  EXPECT_TRUE(std::any_cast<bool>(instance->op(op_name, 5)));
}