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

#include <llvm/ADT/APFloat.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>

#include <any>
#include <dcds/builder/builder.hpp>
#include <dcds/builder/function-builder.hpp>
#include <dcds/builder/statement-builder.hpp>
#include <dcds/context/DCDSContext.hpp>
#include <dcds/exporter/jit-container.hpp>
#include <dcds/util/logging.hpp>
#include <dcds/util/profiling.hpp>
#include <iostream>
#include <libcuckoo/cuckoohash_map.hh>

#include "dcds/builder/expressions/expressions.hpp"
#include "dcds/builder/expressions/unary-expressions.hpp"

static bool generateLinkedListNode(const std::shared_ptr<dcds::Builder>& builder) {
  // FIXME: create addAttribute without initial value also.

  auto payloadAttribute = builder->addAttribute("payload", dcds::INTEGER, UINT64_C(0));
  auto nextAttribute = builder->addAttribute("next", dcds::RECORD_PTR, nullptr);
  builder->generateGetter(payloadAttribute);  // get_payload
  builder->generateSetter(payloadAttribute);  // set_payload

  builder->generateGetter(nextAttribute);  // get_next
  builder->generateSetter(nextAttribute);  // set_next

  return true;
}

// static void addPopFront(dcds::Builder &builder){
//
// }
//
static void addFront(std::shared_ptr<dcds::Builder>& builder) {
  // get the first element from the head (does not pop)

  auto fn = builder->createFunction("front", dcds::valueType::INTEGER);

  fn->addTempVariable("tmp_head", builder->getAttribute("head")->type);

  auto stmtBuilder = fn->getStatementBuilder();
  stmtBuilder->addReadStatement(builder->getAttribute("head"), "tmp_head");
  // fn->addLogStatement("log from inside addFront");
  fn->addTempVariable("tmp_payload_ret", dcds::INTEGER);
  stmtBuilder->addMethodCall(builder->getRegisteredType("LL_NODE"), "tmp_head", "get_payload", "tmp_payload_ret");
  stmtBuilder->addReturnStatement("tmp_payload_ret");
}

static void addPushFront(std::shared_ptr<dcds::Builder>& builder) {
  // FIXME: Can argument to a method call be a constant value or constant expression?
  // FIXME: micro-opt: we dont need a txn starting until reading head but that wouldn't be the case.
  // TODO: Add tail-checks, and then write code, and try dead/write-only attribute.
  // TODO: add condition, if tail attribute exists, then update tail if necessary.

  /*
   * void push_front(uint64_t value){
   *     auto newNode = node(val)
   *     auto tmp = head;
   *     if(tmp != nullptr)
   *        tmp.next = newNode
   *     head = newNode
   *     return;
   * */

  // declare void push_front(uint64_t value)
  auto fn = builder->createFunction("push_front", dcds::valueType::INTEGER);
  auto nodeType = builder->getRegisteredType("LL_NODE");
  fn->addArgument("push_value", dcds::INTEGER);

  auto stmtBuilder = fn->getStatementBuilder();

  // gen: auto newNode = node(val)
  stmtBuilder->addInsertStatement(nodeType, "tmp_node");
  stmtBuilder->addMethodCall(nodeType, "tmp_node", "set_payload", "", std::string{"push_value"});

  // gen: auto tmp = head;
  fn->addTempVariable("tmp_head", builder->getAttribute("head")->type);
  stmtBuilder->addReadStatement(builder->getAttribute("head"), "tmp_head");

  // gen: if(tmp != nullptr)
  auto conditionalBlocks =
      stmtBuilder->addConditionalBranch(new dcds::expressions::IsNotNullExpression{fn->getTempVariable("tmp_head")});

  // gen: tmp.next = newNode
  conditionalBlocks.ifBlock->addMethodCall(nodeType, "tmp_head", "set_next", "", std::string{"tmp_node"});

  // gen: head = newNode
  stmtBuilder->addUpdateStatement(builder->getAttribute("head"), "tmp_node");

  // gen: return;
  stmtBuilder->addReturnVoidStatement();
}

static void generateLinkedList() {
  auto builder = std::make_shared<dcds::Builder>("LinkedList");
  generateLinkedListNode(builder->createType("LL_NODE"));

  // list will have a node type, and two attribute of head/tail.

  // This should be named, and attribute. what about initial value of this attribute?
  // builder.addAttribute("head", nodeType); this is not type nodeType! This is a reference to it!!

  // TODO: how do we ensure the initial value provide is legal for the given type.
  builder->addAttribute("head", dcds::RECORD_PTR, nullptr);  // initially, the list is empty, so pointing null.
  builder->addAttribute("tail", dcds::RECORD_PTR, nullptr);  // initially, the list is empty, so pointing null.

  // builder->addAttributePointerType("LL_NODE", "head"); // does not work at the moment.
  // builder->addAttributePointerType("LL_NODE", "tail");

  addPushFront(builder);
  addFront(builder);

  LOG(INFO) << "generateLinkedList -- build-before";
  builder->build();
  LOG(INFO) << "generateLinkedList -- build-after";

  LOG(INFO) << "generateLinkedList -- create-instance-before";
  auto instance = builder->createInstance();
  LOG(INFO) << "generateLinkedList -- create-instance-after";

  instance->listAllAvailableFunctions();

  //  auto res = instance->op("get_payload");
  //  instance->op("set_payload", 10);
  //  instance->op("get_payload");
  instance->op("push_front", 99);
  //  instance->op("push_front", 11);
  //  instance->op("push_front", 12);

  instance->op("front");

  LOG(INFO) << "generateLinkedList -- DONE";
}

static void generateNode() {
  auto builder = std::make_shared<dcds::Builder>("LL_NODE");
  generateLinkedListNode(builder);
  //  builder->addHint(dcds::hints::SINGLE_THREADED);

  LOG(INFO) << "generateNode -- build-before";
  builder->build();
  LOG(INFO) << "generateNode -- build-after";

  auto instance = builder->createInstance();
  instance->listAllAvailableFunctions();

  auto res = instance->op("get_payload");
  instance->op("set_payload", 10);
  instance->op("get_payload");
}

static void testCondBr() {
  auto builder = std::make_shared<dcds::Builder>("TEST_COND_BR");
  builder->addHint(dcds::hints::SINGLE_THREADED);

  // FIXME: adding attribute as it expects at least one attr
  //  do the case where there is no attribute, hence, no sync anyway between stuff. then whats the point of dcds?
  builder->addAttribute("payload", dcds::INTEGER, UINT64_C(0));

  // -- function create

  //  testConditionBr

  auto fn = builder->createFunction("testConditionBr");
  fn->addArgument("arg_one", dcds::INTEGER);
  auto sb = fn->getStatementBuilder();

  auto conditionalBlocks =
      sb->addConditionalBranch(new dcds::expressions::IsEvenExpression{fn->getArgument("arg_one")});

  conditionalBlocks.ifBlock->addLogStatement("It is even branch");
  conditionalBlocks.elseBlock->addLogStatement("It is odd branch");
  sb->addReturnVoidStatement();

  //  fn->addTempVariable("tmp_head", builder->getAttribute("head")->type);
  //
  //  auto stmtBuilder = fn->getStatementBuilder();
  //  stmtBuilder->addReadStatement(builder->getAttribute("head"), "tmp_head");
  //  // fn->addLogStatement("log from inside addFront");
  //  fn->addTempVariable("tmp_payload_ret", dcds::INTEGER);
  //  stmtBuilder->addMethodCall(builder->getRegisteredType("LL_NODE"), "tmp_head", "get_payload", "tmp_payload_ret");
  //  stmtBuilder->addReturnStatement("tmp_payload_ret");

  // -- function end

  LOG(INFO) << "testCondBr -- build-before";
  builder->build();
  LOG(INFO) << "testCondBr -- build-after";

  auto instance = builder->createInstance();
  instance->listAllAvailableFunctions();

  auto res = instance->op("testConditionBr");
  LOG(INFO) << "xxxx";
  instance->op("testConditionBr", 3);
  instance->op("testConditionBr", 4);
}
static void testOnlyIfBranch() {
  auto builder = std::make_shared<dcds::Builder>("TEST_COND_BR_MULTIPLE");
  builder->addHint(dcds::hints::SINGLE_THREADED);

  // FIXME: adding attribute as it expects at least one attr
  //  do the case where there is no attribute, hence, no sync anyway between stuff. then whats the point of dcds?
  builder->addAttribute("payload", dcds::INTEGER, UINT64_C(0));

  auto fn = builder->createFunction("testConditionBr");
  fn->addArgument("arg_one", dcds::INTEGER);
  fn->addArgument("arg_two", dcds::INTEGER);
  auto sb = fn->getStatementBuilder();

  auto conditionalBlocks =
      sb->addConditionalBranch(new dcds::expressions::IsEvenExpression{fn->getArgument("arg_one")});
  conditionalBlocks.ifBlock->addLogStatement("1: It is the if branch");

  sb->addLogStatement("Merge branch, returning now");

  sb->addReturnVoidStatement();

  LOG(INFO) << __FUNCTION__ << " -- build-before";
  builder->build();
  LOG(INFO) << __FUNCTION__ << " -- build-before";

  auto instance = builder->createInstance();
  instance->listAllAvailableFunctions();

  LOG(INFO) << __FUNCTION__ << " -- tests";
  instance->op("testConditionBr", 1, 2);
  instance->op("testConditionBr", 3, 4);
}

static void testMultipleBranches() {
  auto builder = std::make_shared<dcds::Builder>("TEST_COND_BR_MULTIPLE");
  builder->addHint(dcds::hints::SINGLE_THREADED);

  // FIXME: adding attribute as it expects at least one attr
  //  do the case where there is no attribute, hence, no sync anyway between stuff. then whats the point of dcds?
  builder->addAttribute("payload", dcds::INTEGER, UINT64_C(0));

  auto fn = builder->createFunction("testConditionBr");
  fn->addArgument("arg_one", dcds::INTEGER);
  fn->addArgument("arg_two", dcds::INTEGER);
  auto sb = fn->getStatementBuilder();

  auto conditionalBlocks =
      sb->addConditionalBranch(new dcds::expressions::IsEvenExpression{fn->getArgument("arg_one")});
  conditionalBlocks.ifBlock->addLogStatement("1: It is even branch");
  conditionalBlocks.elseBlock->addLogStatement("1: It is odd branch");

  auto cond2 = sb->addConditionalBranch(new dcds::expressions::IsEvenExpression{fn->getArgument("arg_two")});
  cond2.ifBlock->addLogStatement("2: It is even branch");
  cond2.elseBlock->addLogStatement("2: It is odd branch");

  sb->addReturnVoidStatement();

  LOG(INFO) << __FUNCTION__ << " -- build-before";
  builder->build();
  LOG(INFO) << __FUNCTION__ << " -- build-before";

  auto instance = builder->createInstance();
  instance->listAllAvailableFunctions();

  LOG(INFO) << __FUNCTION__ << " -- tests";
  instance->op("testConditionBr", 1, 2);
  instance->op("testConditionBr", 3, 4);
}

static void testNestedBranches() {
  auto builder = std::make_shared<dcds::Builder>("TEST_COND_BR_NESTED");
  builder->addHint(dcds::hints::SINGLE_THREADED);

  builder->addAttribute("payload", dcds::INTEGER, UINT64_C(0));

  auto fn = builder->createFunction("testConditionBr");
  fn->addArgument("arg_one", dcds::INTEGER);
  fn->addArgument("arg_two", dcds::INTEGER);
  auto sb = fn->getStatementBuilder();

  auto conditionalBlocks =
      sb->addConditionalBranch(new dcds::expressions::IsEvenExpression{fn->getArgument("arg_one")});
  conditionalBlocks.ifBlock->addLogStatement("1: It is even branch");
  conditionalBlocks.elseBlock->addLogStatement("1: It is odd branch");

  auto cond2 = conditionalBlocks.ifBlock->addConditionalBranch(
      new dcds::expressions::IsEvenExpression{fn->getArgument("arg_two")});
  cond2.ifBlock->addLogStatement("2: It is even branch");
  cond2.elseBlock->addLogStatement("2: It is odd branch");

  sb->addLogStatement("Merge branch, returning now");
  sb->addReturnVoidStatement();

  LOG(INFO) << __FUNCTION__ << " -- build-before";
  builder->build();
  LOG(INFO) << __FUNCTION__ << " -- build-before";

  auto instance = builder->createInstance();
  instance->listAllAvailableFunctions();

  LOG(INFO) << __FUNCTION__ << " -- tests";
  instance->op("testConditionBr", 1, 2);  // odd then even
  instance->op("testConditionBr", 1, 3);  // odd then odd
  instance->op("testConditionBr", 2, 4);  // even then even
  instance->op("testConditionBr", 2, 3);  // even then odd
}

int main() {
  LOG(INFO) << "main() -- start";
  generateLinkedList();
  // generateNode();
  //  testCondBr();
  //  testMultipleBranches();
  //  testOnlyIfBranch();
  //  testNestedBranches();

  LOG(INFO) << "main() -- done";
  return 0;
}
