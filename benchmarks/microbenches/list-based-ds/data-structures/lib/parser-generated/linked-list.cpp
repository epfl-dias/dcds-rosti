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

#include "parser-generated/linked-list.hpp"

#include <utility>

using namespace dcds_generated;

// CHECKs with an any cast wrapper
static void CHECK_TRUE(std::any v) { CHECK(std::any_cast<bool>(v)); }
static void CHECK_FALSE(std::any v) { CHECK_EQ(std::any_cast<bool>(v), false); }

LinkedList::LinkedList(std::string _ds_name, std::string _ds_node_name)
    : ds_name(std::move(_ds_name)), ds_node_name(std::move(_ds_node_name)) {
  this->builder = std::make_shared<dcds::Builder>(ds_name);
  this->generateLinkedListNode();
  auto nodeType = builder->getRegisteredType(ds_node_name);

  builder->addAttributePtr("head", nodeType);
  builder->addAttributePtr("tail", nodeType);

  this->createFunction_empty();
}

void LinkedList::build() {
  LOG(INFO) << "LinkedList::build() -- start";
  builder->build();
  LOG(INFO) << "LinkedList::build() -- end";
}
void LinkedList::optimize() {
  LOG(INFO) << "LinkedList::optimize() -- start";
  dcds::BuilderOptPasses buildOptimizer(builder);
  buildOptimizer.runAll();
  LOG(INFO) << "LinkedList::optimize() -- end";

  builder->injectCC();
}
void LinkedList::generateLinkedListNode() {
  if (builder->hasRegisteredType(ds_node_name)) {
    return;
  }

  auto nodeBuilder = builder->createType(ds_node_name);
  auto payloadAttribute = nodeBuilder->addAttribute("payload", dcds::valueType::INT64, UINT64_C(0));
  auto nextAttribute = nodeBuilder->addAttribute("next", dcds::valueType::RECORD_PTR, nullptr);
  nodeBuilder->generateGetter(payloadAttribute);  // get_payload
  nodeBuilder->generateSetter(payloadAttribute);  // set_payload

  nodeBuilder->generateGetter(nextAttribute);  // get_next
  nodeBuilder->generateSetter(nextAttribute);  // set_next
}

void LinkedList::createFunction_empty() {
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

Stack::Stack() : LinkedList("LL_Stack") {
  this->createFunction_pop();
  this->createFunction_push();
}
void Stack::createFunction_push() {
  // FIXME: Can argument to a method call be a constant value or constant expression?
  // FIXME: micro-opt: we dont need a txn starting until reading head but that wouldn't be the case.
  // TODO: Add tail-checks, and then write code, and try dead/write-only attribute.
  // TODO: add condition, if tail attribute exists, then update tail if necessary.

  /*
   * void push(uint64_t value){
   *     auto newNode = node(val)
   *     auto tmp = head;
   *     if(tmp != nullptr)
   *        tmp.next = newNode
   *     head = newNode
   *     return;
   * */

  // declare void push(uint64_t value)
  auto fn = builder->createFunction("push");
  auto nodeType = builder->getRegisteredType(ds_node_name);
  fn->addArgument("push_value", dcds::valueType::INT64);

  auto stmtBuilder = fn->getStatementBuilder();

  // gen: auto newNode = node(val)
  stmtBuilder->addInsertStatement(nodeType, "tmp_node");
  stmtBuilder->addMethodCall(nodeType, "tmp_node", "set_payload", "", {"push_value"});

  // gen: auto tmp = head;
  fn->addTempVariable("tmp_head", builder->getAttribute("head")->type);
  stmtBuilder->addReadStatement(builder->getAttribute("head"), "tmp_head");

  // gen: if(tmp != nullptr)
  auto conditionalBlocks =
      stmtBuilder->addConditionalBranch(new dcds::expressions::IsNotNullExpression{fn->getTempVariable("tmp_head")});

  // gen: tmp.next = newNode
  conditionalBlocks.ifBlock->addMethodCall(nodeType, "tmp_node", "set_next", "", {"tmp_head"});

  // gen: head = newNode
  stmtBuilder->addUpdateStatement(builder->getAttribute("head"), "tmp_node");

  // gen: return;
  stmtBuilder->addReturnVoidStatement();
}
void Stack::createFunction_pop() {
  // declare bool pop(uint64_t *val)
  auto fn = builder->createFunction("pop", dcds::valueType::BOOL);
  fn->addArgument("pop_value", dcds::valueType::INT64, true);

  auto stmtBuilder = fn->getStatementBuilder();

  fn->addTempVariable("tmp_head", builder->getAttribute("head")->type);
  stmtBuilder->addReadStatement(builder->getAttribute("head"), "tmp_head");

  // gen: if(tmp != nullptr)
  auto conditionalBlocks =
      stmtBuilder->addConditionalBranch(new dcds::expressions::IsNotNullExpression{fn->getTempVariable("tmp_head")});

  // ifBlock
  {
    // save the payload into the pop value.
    conditionalBlocks.ifBlock->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_head", "get_payload",
                                             "pop_value");

    // get next of the current_head:tmp
    fn->addTempVariable("tmp_next", dcds::valueType::RECORD_PTR);
    conditionalBlocks.ifBlock->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_head", "get_next",
                                             "tmp_next");

    // Update the head.
    conditionalBlocks.ifBlock->addUpdateStatement(builder->getAttribute("head"), "tmp_next");

    // TODO: delete the tmp (dangling record now)
    // stmtBuilder->addDeleteStatement("tmp");

    conditionalBlocks.ifBlock->addReturnStatement(std::make_shared<dcds::expressions::BoolConstant>(true));
  }

  // elseBlock
  {
    // head is null, return false -> no pop.
    conditionalBlocks.elseBlock->addReturnStatement(std::make_shared<dcds::expressions::BoolConstant>(false));
  }
}
void Stack::test() {
  LOG(INFO) << "Stack::test() -- start";

  LOG(INFO) << "generateLinkedList -- create-instance-before";
  auto instance = builder->createInstance();
  LOG(INFO) << "generateLinkedList -- create-instance-after";

  instance->listAllAvailableFunctions();

  CHECK_TRUE(instance->op("empty"));
  instance->op("push", 11);
  CHECK_FALSE(instance->op("empty"));
  instance->op("push", 22);
  instance->op("push", 33);
  instance->op("push", 44);
  CHECK_FALSE(instance->op("empty"));

  // pop
  uint64_t val = 0;
  CHECK_TRUE(instance->op("pop", &val));
  CHECK_EQ(val, 44);
  CHECK_TRUE(instance->op("pop", &val));
  CHECK_EQ(val, 33);
  CHECK_TRUE(instance->op("pop", &val));
  CHECK_EQ(val, 22);
  CHECK_TRUE(instance->op("pop", &val));
  CHECK_EQ(val, 11);
  CHECK_FALSE(instance->op("pop", &val));
  CHECK_TRUE(instance->op("empty"));

  LOG(INFO) << "Stack::test() -- end";
}

FIFO::FIFO() : LinkedList("LL_FIFO") {
  this->createFunction_push();
  this->createFunction_pop();
  builder->injectCC();
}
void FIFO::createFunction_push() {
  /*

     if(!head){
      tail=node;
     } else {
      head.next = node;
     }

     head = node;
     return;
   * */

  // declare void push(uint64_t value)
  auto fn = builder->createFunction("push");
  auto nodeType = builder->getRegisteredType(ds_node_name);
  fn->addArgument("push_value", dcds::valueType::INT64);

  auto stmtBuilder = fn->getStatementBuilder();

  // gen: auto newNode = node(val)
  stmtBuilder->addInsertStatement(nodeType, "tmp_node");
  stmtBuilder->addMethodCall(nodeType, "tmp_node", "set_payload", "", {"push_value"});

  // gen: auto tmp = head;
  fn->addTempVariable("tmp_head", builder->getAttribute("head")->type);
  stmtBuilder->addReadStatement(builder->getAttribute("head"), "tmp_head");

  // gen: if(tmp == nullptr)
  auto conditionalBlocks =
      stmtBuilder->addConditionalBranch(new dcds::expressions::IsNullExpression{fn->getTempVariable("tmp_head")});

  conditionalBlocks.ifBlock->addUpdateStatement(builder->getAttribute("tail"), "tmp_node");

  conditionalBlocks.elseBlock->addMethodCall(nodeType, "tmp_head", "set_next", "", {"tmp_node"});

  // gen: head = newNode
  stmtBuilder->addUpdateStatement(builder->getAttribute("head"), "tmp_node");

  // gen: return;
  stmtBuilder->addReturnVoidStatement();
}
void FIFO::createFunction_pop() {
  // TODO: pop from tail
  /*

    if(tail == nullptr){
      return false;
    } else {
      tmp = tail;
      tail = tail.next;
      if(tail.next == nullptr){
        // meaning head was pointing to the same?
        head = nullptr;
      }
      {tmp.payload} => save to referenceArgument
      return true;
    }


   * */

  // declare bool pop(uint64_t *val)
  auto fn = builder->createFunction("pop", dcds::valueType::BOOL);
  fn->addArgument("pop_value", dcds::valueType::INT64, true);

  auto stmtBuilder = fn->getStatementBuilder();

  fn->addTempVariable("tmp_tail", builder->getAttribute("tail")->type);
  stmtBuilder->addReadStatement(builder->getAttribute("tail"), "tmp_tail");

  // gen: if(tail == nullptr)
  auto conditionalBlocks =
      stmtBuilder->addConditionalBranch(new dcds::expressions::IsNullExpression{fn->getTempVariable("tmp_tail")});

  // ifBlock
  { conditionalBlocks.ifBlock->addReturnStatement(std::make_shared<dcds::expressions::BoolConstant>(false)); }

  // elseBlock
  {
    conditionalBlocks.elseBlock->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_tail", "get_payload",
                                               "pop_value");

    auto tmpTailNext = fn->addTempVariable("tmp_tail_next", dcds::valueType::RECORD_PTR);
    // tail = tail.next
    conditionalBlocks.elseBlock->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_tail", "get_next",
                                               "tmp_tail_next");
    conditionalBlocks.elseBlock->addUpdateStatement(builder->getAttribute("tail"), "tmp_tail_next");

    // if tail.next isNull
    //    head=null
    auto cond2 =
        conditionalBlocks.elseBlock->addConditionalBranch(new dcds::expressions::IsNullExpression{tmpTailNext});
    cond2.ifBlock->addUpdateStatement(builder->getAttribute("head"), tmpTailNext);

    // return true;
    conditionalBlocks.elseBlock->addReturnStatement(std::make_shared<dcds::expressions::BoolConstant>(true));
  }
}

void FIFO::test() {
  LOG(INFO) << "FIFO::test() -- start";

  LOG(INFO) << "generateLinkedList -- create-instance-before";
  auto instance = builder->createInstance();
  LOG(INFO) << "generateLinkedList -- create-instance-after";

  instance->listAllAvailableFunctions();

  // push
  CHECK_TRUE(instance->op("empty"));
  instance->op("push", 11);
  CHECK_FALSE(instance->op("empty"));
  instance->op("push", 22);
  instance->op("push", 33);
  instance->op("push", 44);
  CHECK_FALSE(instance->op("empty"));

  // pop
  uint64_t val = 0;
  CHECK_TRUE(instance->op("pop", &val));
  CHECK_EQ(val, 11);
  CHECK_TRUE(instance->op("pop", &val));
  CHECK_EQ(val, 22);
  CHECK_TRUE(instance->op("pop", &val));
  CHECK_EQ(val, 33);
  CHECK_TRUE(instance->op("pop", &val));
  CHECK_EQ(val, 44);
  CHECK_FALSE(instance->op("pop", &val));
  CHECK_TRUE(instance->op("empty"));

  LOG(INFO) << "FIFO::test() -- end";
}