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

#include "parser-generated/doubly-linked-list.hpp"

using namespace dcds_generated;

static void EXPECT_TRUE(std::any v) { assert(std::any_cast<bool>(v)); }
static void EXPECT_FALSE(std::any v) { assert(std::any_cast<bool>(v) == false); }
template <typename A, typename B>
void EXPECT_EQ(A a, B b) {
  LOG(INFO) << "val: " << a << " | expected: " << b;
  assert(a == b);
}

DoublyLinkedList::DoublyLinkedList() {
  this->builder = std::make_shared<dcds::Builder>(ds_name);
  this->generateLinkedListNode();

  auto nodeType = builder->getRegisteredType(ds_node_name);

  builder->addAttributePtr("head", nodeType);
  builder->addAttributePtr("tail", nodeType);

  this->createFunction_empty();
  this->createFunction_pushFront();
  this->createFunction_popFront();
  //
  this->createFunction_pushBack();
  this->createFunction_popBack();
  //
  //  this->createFunction_extract();
  //  this->createFunction_touch();
}

void DoublyLinkedList::build() {
  LOG(INFO) << "DoublyLinkedList::build() -- start";
  builder->build();
  LOG(INFO) << "DoublyLinkedList::build() -- end";
}
void DoublyLinkedList::optimize() {
  LOG(INFO) << "DoublyLinkedList::optimize() -- start";
  dcds::BuilderOptPasses buildOptimizer(builder);
  buildOptimizer.runAll();
  LOG(INFO) << "DoublyLinkedList::optimize() -- end";
}
void DoublyLinkedList::generateLinkedListNode() {
  if (builder->hasRegisteredType(ds_node_name)) {
    return;
  }

  auto nodeBuilder = builder->createType(ds_node_name);
  auto payloadAttribute = nodeBuilder->addAttribute("payload", dcds::valueType::INT64, UINT64_C(0));
  auto nextAttribute = nodeBuilder->addAttribute("next", dcds::valueType::RECORD_PTR, nullptr);
  auto prevAttribute = nodeBuilder->addAttribute("prev", dcds::valueType::RECORD_PTR, nullptr);

  nodeBuilder->generateGetter(payloadAttribute);  // get_payload
  nodeBuilder->generateSetter(payloadAttribute);  // set_payload

  nodeBuilder->generateGetter(nextAttribute);  // get_next
  nodeBuilder->generateSetter(nextAttribute);  // set_next

  nodeBuilder->generateGetter(prevAttribute);  // get_prev
  nodeBuilder->generateSetter(prevAttribute);  // set_prev
}

void DoublyLinkedList::createFunction_empty() {
  // declare bool empty()
  auto fn = builder->createFunction("empty", dcds::valueType::BOOL);

  auto stmtBuilder = fn->getStatementBuilder();

  fn->addTempVariable("tmp_head", builder->getAttribute("head")->type);
  stmtBuilder->addReadStatement(builder->getAttribute("head"), "tmp_head");

  auto conditionalBlocks =
      stmtBuilder->addConditionalBranch(new dcds::expressions::IsNullExpression{fn->getTempVariable("tmp_head")});

  conditionalBlocks.ifBlock->addReturnStatement(std::make_shared<dcds::expressions::BoolConstant>(true));
  conditionalBlocks.elseBlock->addReturnStatement(std::make_shared<dcds::expressions::BoolConstant>(false));
}

void DoublyLinkedList::createFunction_pushFront() {
  // declare void push_front(uint64_t value)
  auto fn = builder->createFunction("push_front");
  auto nodeType = builder->getRegisteredType(ds_node_name);
  fn->addArgument("push_value", dcds::valueType::INT64);

  auto stmtBuilder = fn->getStatementBuilder();

  /*

    if(!head){
      tail = node;
    } else {
      node->next = head;
      head->prev = node;
    }
    head = node;

   * */

  stmtBuilder->addInsertStatement(nodeType, "tmp_node");
  stmtBuilder->addMethodCall(nodeType, "tmp_node", "set_payload", "", {"push_value"});

  fn->addTempVariable("tmp_head", builder->getAttribute("head")->type);
  stmtBuilder->addReadStatement(builder->getAttribute("head"), "tmp_head");

  auto isListEmpty =
      stmtBuilder->addConditionalBranch(new dcds::expressions::IsNullExpression{fn->getTempVariable("tmp_head")});

  {
    // isListEmpty:: IF TRUE
    //    tail = node;
    isListEmpty.ifBlock->addUpdateStatement(builder->getAttribute("tail"), "tmp_node");
  }
  {
    // isListEmpty:: IF FALSE
    //    node->next = head;
    //    head->prev = node;

    isListEmpty.elseBlock->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_node", "set_next",
                                         std::vector<std::string>{"tmp_head"});
    isListEmpty.elseBlock->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_head", "set_prev",
                                         std::vector<std::string>{"tmp_node"});
  }

  // head = node;
  stmtBuilder->addUpdateStatement(builder->getAttribute("head"), "tmp_node");
  stmtBuilder->addReturnVoidStatement();
}
void DoublyLinkedList::createFunction_popFront() {
  // declare bool pop_front(uint64_t *val)
  auto fn = builder->createFunction("pop_front", dcds::valueType::BOOL);
  fn->addArgument("pop_value", dcds::valueType::INT64, true);

  auto stmtBuilder = fn->getStatementBuilder();

  fn->addTempVariable("tmp_head", builder->getAttribute("head")->type);
  stmtBuilder->addReadStatement(builder->getAttribute("head"), "tmp_head");

  auto conditionalBlocks =
      stmtBuilder->addConditionalBranch(new dcds::expressions::IsNullExpression{fn->getTempVariable("tmp_head")});
  conditionalBlocks.ifBlock->addReturnStatement(std::make_shared<dcds::expressions::BoolConstant>(false));
  {
    // head not null, pop it

    // get pop value
    conditionalBlocks.elseBlock->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_head", "get_payload",
                                               "pop_value");

    // pop the ptr
    fn->addTempVariable("tmp_head_next", dcds::valueType::RECORD_PTR);
    conditionalBlocks.elseBlock->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_head", "get_next",
                                               "tmp_head_next");

    // head = tmp_head_next
    // if (tmp_head_next){
    //    tmp_head_next->prev = nullptr
    // } else {
    //    tail = nullptr;
    // }

    conditionalBlocks.elseBlock->addUpdateStatement(builder->getAttribute("head"), "tmp_head_next");
    auto cond2 = conditionalBlocks.elseBlock->addConditionalBranch(
        new dcds::expressions::IsNotNullExpression{fn->getTempVariable("tmp_head_next")});

    {
      // hacked: to be passed constant expression!
      fn->addTempVariable("nullptr_tmp", dcds::valueType::RECORD_PTR);
      cond2.ifBlock->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_head_next", "set_prev",
                                   std::vector<std::string>{"nullptr_tmp"});
    }

    {
      cond2.elseBlock->addUpdateStatement(builder->getAttribute("tail"),
                                          std::make_shared<dcds::expressions::NullPtrConstant>());
    }

    conditionalBlocks.elseBlock->addReturnStatement(std::make_shared<dcds::expressions::BoolConstant>(true));
  }
}

void DoublyLinkedList::createFunction_pushBack() {
  // declare void push_back(uint64_t value)
  auto fn = builder->createFunction("push_back");
  auto nodeType = builder->getRegisteredType(ds_node_name);
  fn->addArgument("push_value", dcds::valueType::INT64);

  auto stmtBuilder = fn->getStatementBuilder();

  /*

    if(!tail){
      head = node;
    } else {
      node->prev = tail;
      tail->next = node;
    }
    tail = node;

   * */

  stmtBuilder->addInsertStatement(nodeType, "tmp_node");
  stmtBuilder->addMethodCall(nodeType, "tmp_node", "set_payload", "", {"push_value"});

  fn->addTempVariable("tmp_tail", builder->getAttribute("tail")->type);
  stmtBuilder->addReadStatement(builder->getAttribute("tail"), "tmp_tail");

  auto isListEmpty =
      stmtBuilder->addConditionalBranch(new dcds::expressions::IsNullExpression{fn->getTempVariable("tmp_tail")});

  {
    // isListEmpty:: IF TRUE
    //    head = node;
    isListEmpty.ifBlock->addUpdateStatement(builder->getAttribute("head"), "tmp_node");
  }
  {
    // isListEmpty:: IF FALSE
    //    node->prev = tail;
    //    tail->next = node;

    isListEmpty.elseBlock->addLogStatement("[push_back] Tail: %llu\n", {fn->getTempVariable("tmp_tail")});
    isListEmpty.elseBlock->addLogStatement("[push_back] Tail-tmpNode: %llu\n", {fn->getTempVariable("tmp_node")});

    isListEmpty.elseBlock->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_node", "set_prev",
                                         std::vector<std::string>{"tmp_tail"});
    isListEmpty.elseBlock->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_tail", "set_next",
                                         std::vector<std::string>{"tmp_node"});
  }

  // tail = node;
  stmtBuilder->addUpdateStatement(builder->getAttribute("tail"), "tmp_node");
  stmtBuilder->addReturnVoidStatement();
}
void DoublyLinkedList::createFunction_popBack() {
  // declare bool pop_back(uint64_t *val)
  auto fn = builder->createFunction("pop_back", dcds::valueType::BOOL);
  fn->addArgument("pop_value", dcds::valueType::INT64, true);

  auto stmtBuilder = fn->getStatementBuilder();

  fn->addTempVariable("tmp_tail", builder->getAttribute("tail")->type);
  stmtBuilder->addReadStatement(builder->getAttribute("tail"), "tmp_tail");

  auto conditionalBlocks =
      stmtBuilder->addConditionalBranch(new dcds::expressions::IsNullExpression{fn->getTempVariable("tmp_tail")});
  {
    // empty list
    conditionalBlocks.ifBlock->addReturnStatement(std::make_shared<dcds::expressions::BoolConstant>(false));
  }

  {
    // tail not null, pop it
    conditionalBlocks.elseBlock->addLogStatement("[pop_back] Tail: %llu\n", {fn->getTempVariable("tmp_tail")});

    // get pop value
    conditionalBlocks.elseBlock->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_tail", "get_payload",
                                               "pop_value");
    conditionalBlocks.elseBlock->addLogStatement("[pop_back] tail-val is %llu\n", {fn->getArgument("pop_value")});

    // pop the ptr
    fn->addTempVariable("tmp_tail_prev", dcds::valueType::RECORD_PTR);
    conditionalBlocks.elseBlock->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_tail", "get_prev",
                                               "tmp_tail_prev");

    conditionalBlocks.elseBlock->addLogStatement("[pop_back] tail-prev is %llu\n",
                                                 {fn->getTempVariable("tmp_tail_prev")});

    // tail = tmp_tail_prev
    // if (tmp_tail_prev){
    //    tmp_tail_prev->next = nullptr
    // } else {
    //    head = nullptr;
    // }

    conditionalBlocks.elseBlock->addUpdateStatement(builder->getAttribute("tail"), "tmp_tail_prev");
    auto cond2 = conditionalBlocks.elseBlock->addConditionalBranch(
        new dcds::expressions::IsNotNullExpression{fn->getTempVariable("tmp_tail_prev")});

    {
      // hacked: to be passed constant expression!
      fn->addTempVariable("nullptr_tmp", dcds::valueType::RECORD_PTR);
      conditionalBlocks.elseBlock->addLogStatement("[pop_back] nulL-val is:  %llu\n",
                                                   {fn->getTempVariable("nullptr_tmp")});
      cond2.ifBlock->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_tail_prev", "set_next",
                                   std::vector<std::string>{"nullptr_tmp"});
    }

    {
      cond2.elseBlock->addUpdateStatement(builder->getAttribute("head"),
                                          std::make_shared<dcds::expressions::NullPtrConstant>());
    }

    conditionalBlocks.elseBlock->addReturnStatement(std::make_shared<dcds::expressions::BoolConstant>(true));
  }
}

void DoublyLinkedList::createFunction_extract() {}
void DoublyLinkedList::createFunction_touch() {}

static void testBackOps(dcds::JitContainer* instance) {
  LOG(INFO) << "DoublyLinkedList::testBackOps() -- start";

  EXPECT_TRUE(instance->op("empty"));
  instance->op("push_back", 11);
  EXPECT_FALSE(instance->op("empty"));
  instance->op("push_back", 22);
  instance->op("push_back", 33);
  instance->op("push_back", 44);
  EXPECT_FALSE(instance->op("empty"));

  // pop
  uint64_t val = 0;
  EXPECT_TRUE(instance->op("pop_back", &val));
  EXPECT_EQ(val, 44);
  EXPECT_TRUE(instance->op("pop_back", &val));
  EXPECT_EQ(val, 33);
  EXPECT_TRUE(instance->op("pop_back", &val));
  EXPECT_EQ(val, 22);
  EXPECT_TRUE(instance->op("pop_back", &val));
  EXPECT_EQ(val, 11);
  EXPECT_FALSE(instance->op("pop_back", &val));
  EXPECT_TRUE(instance->op("empty"));

  LOG(INFO) << "DoublyLinkedList::testBackOps() -- end";
}
static void testFrontOps(dcds::JitContainer* instance) {
  LOG(INFO) << "DoublyLinkedList::testFrontOps() -- start";

  EXPECT_TRUE(instance->op("empty"));
  instance->op("push_front", 11);
  EXPECT_FALSE(instance->op("empty"));
  instance->op("push_front", 22);
  instance->op("push_front", 33);
  instance->op("push_front", 44);
  EXPECT_FALSE(instance->op("empty"));

  // pop
  uint64_t val = 0;
  EXPECT_TRUE(instance->op("pop_front", &val));
  EXPECT_EQ(val, 44);
  EXPECT_TRUE(instance->op("pop_front", &val));
  EXPECT_EQ(val, 33);
  EXPECT_TRUE(instance->op("pop_front", &val));
  EXPECT_EQ(val, 22);
  EXPECT_TRUE(instance->op("pop_front", &val));
  EXPECT_EQ(val, 11);
  EXPECT_FALSE(instance->op("pop_front", &val));
  EXPECT_TRUE(instance->op("empty"));
  LOG(INFO) << "DoublyLinkedList::testFrontOps() -- end";
}

static void testFrontBackMixedOps(dcds::JitContainer* instance) {
  LOG(INFO) << "DoublyLinkedList::testFrontBackMixedOps() -- start";

  uint64_t val = 0;

  EXPECT_TRUE(instance->op("empty"));
  instance->op("push_front", 11);
  EXPECT_FALSE(instance->op("empty"));
  instance->op("push_front", 22);
  // 22->11

  instance->op("push_back", 33);
  instance->op("push_back", 44);
  // 22->11->33->44

  EXPECT_FALSE(instance->op("empty"));

  EXPECT_TRUE(instance->op("pop_back", &val));
  EXPECT_EQ(val, 44);
  LOG(INFO) << "here";

  EXPECT_TRUE(instance->op("pop_front", &val));
  EXPECT_EQ(val, 22);
  // 11->33

  instance->op("push_front", 66);
  // 66->11->33
  instance->op("push_back", 55);
  // 66->11->33->55

  EXPECT_TRUE(instance->op("pop_front", &val));
  EXPECT_EQ(val, 66);
  EXPECT_TRUE(instance->op("pop_back", &val));
  EXPECT_EQ(val, 55);

  EXPECT_TRUE(instance->op("pop_front", &val));
  EXPECT_EQ(val, 11);
  EXPECT_TRUE(instance->op("pop_back", &val));
  EXPECT_EQ(val, 33);

  EXPECT_TRUE(instance->op("empty"));
  EXPECT_FALSE(instance->op("pop_front", &val));
  EXPECT_FALSE(instance->op("pop_back", &val));

  LOG(INFO) << "DoublyLinkedList::testFrontBackMixedOps() -- end";
}

void DoublyLinkedList::test() {
  LOG(INFO) << "DoublyLinkedList::test() -- start";

  LOG(INFO) << "DoublyLinkedList -- create-instance-before";
  auto instance = builder->createInstance();
  auto instanceTwo = builder->createInstance();
  auto instanceThree = builder->createInstance();
  instance->listAllAvailableFunctions();
  LOG(INFO) << "DoublyLinkedList -- create-instance-after";

  testFrontOps(instance);
  testBackOps(instanceTwo);
  testFrontBackMixedOps(instanceThree);

  LOG(INFO) << "DoublyLinkedList::test() -- end";
}