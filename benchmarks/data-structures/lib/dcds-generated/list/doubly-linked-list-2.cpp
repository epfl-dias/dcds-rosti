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

#include "dcds-generated/list/doubly-linked-list.hpp"

using namespace dcds::datastructures;

static auto int64_const_1 = std::make_shared<dcds::expressions::Int64Constant>(1);

DoublyLinkedList2::DoublyLinkedList2(dcds::valueType payload_type) : dcds_generated_ds(ds_name) {
  this->generateLinkedListNode(payload_type);

  auto nodeType = builder->getRegisteredType(ds_node_name);

  builder->addAttributePtr("head", nodeType);
  builder->addAttributePtr("tail", nodeType);
  builder->addAttribute("size", dcds::valueType::INT64, UINT64_C(0));

  this->createFunction_empty();
  // this->createFunction_pushFront();
  //   this->createFunction_popFront();
  //   this->createFunction_pushBack();
  // this->createFunction_popBack();
  //
  this->createFunction_touch();

  //  this->createFunction_getHeadPtr();
  //  this->createFunction_getTailPtr();
}

void DoublyLinkedList2::generateLinkedListNode(dcds::valueType payload_type) {
  if (builder->hasRegisteredType(ds_node_name)) {
    return;
  }

  auto nodeBuilder = builder->createType(ds_node_name);
  auto payloadAttribute = nodeBuilder->addAttribute("payload", payload_type, UINT64_C(0));
  auto nextAttribute = nodeBuilder->addAttribute("next", dcds::valueType::RECORD_PTR, nullptr);
  auto prevAttribute = nodeBuilder->addAttribute("prev", dcds::valueType::RECORD_PTR, nullptr);

  nodeBuilder->generateGetter(payloadAttribute);  // get_payload
  nodeBuilder->generateSetter(payloadAttribute);  // set_payload

  nodeBuilder->generateGetter(nextAttribute);  // get_next
  nodeBuilder->generateSetter(nextAttribute);  // set_next

  nodeBuilder->generateGetter(prevAttribute);  // get_prev
  nodeBuilder->generateSetter(prevAttribute);  // set_prev
}

void DoublyLinkedList2::createFunction_empty() {
  // declare bool empty()
  auto fn = builder->createFunction("empty", dcds::valueType::BOOL);

  auto stmtBuilder = fn->getStatementBuilder();

  auto tmpTmpHead = fn->addTempVariable("tmp_head", builder->getAttribute("head")->type);
  stmtBuilder->addReadStatement(builder->getAttribute("head"), "tmp_head");

  auto conditionalBlocks = stmtBuilder->addConditionalBranch(new dcds::expressions::IsNullExpression{tmpTmpHead});

  conditionalBlocks.ifBlock->addReturnStatement(std::make_shared<dcds::expressions::BoolConstant>(true));

  // conditionalBlocks.elseBlock->addLogStatement("[empty] non-empty: head is %llu\n", {tmpTmpHead});
  conditionalBlocks.elseBlock->addReturnStatement(std::make_shared<dcds::expressions::BoolConstant>(false));
}

// will-be called from LRU
void DoublyLinkedList2::createFunction_pushFrontWithReturn() {
  // declare void push_front(uint64_t key, uint64_t value)
  auto fn = builder->createFunction("push_front_with_return", dcds::valueType::RECORD_PTR);
  auto nodeType = builder->getRegisteredType(ds_node_name);

  bool has_key_attr = nodeType->hasAttribute("key_");
  assert(has_key_attr);
  if (has_key_attr) {
    LOG(INFO) << "NodeType has key!";
    fn->addArgument("push_key", nodeType->getAttribute("key_")->type);
  }

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

  // stmtBuilder->addLogStatement("[push_front_with_return] here1\n");
  stmtBuilder->addInsertStatement(nodeType, "tmp_node");
  // stmtBuilder->addLogStatement("[push_front_with_return] here2: insTmpNode: %llu\n",
  // {fn->getTempVariable("tmp_node")});

  //    isListEmpty.elseBlock->addLogStatement("[push_front] non-empty: head is %llu\n", {tmp_head});

  stmtBuilder->addMethodCall(nodeType, "tmp_node", "set_payload", "", {"push_value"});
  if (has_key_attr) {
    stmtBuilder->addMethodCall(nodeType, "tmp_node", "set_key_", "", {"push_key"});
  }

  // stmtBuilder->addLogStatement("[push_front_with_return] here3\n");

  //  auto tmpTmpHead = fn->addTempVariable("tmp_head", builder->getAttribute("head")->type);
  //  stmtBuilder->addReadStatement(builder->getAttribute("head"), "tmp_head");
  //  auto conditionalBlocks = stmtBuilder->addConditionalBranch(new dcds::expressions::IsNullExpression{tmpTmpHead});

  auto tmp_head = fn->addTempVariable("tmp_head", builder->getAttribute("head")->type);
  stmtBuilder->addReadStatement(builder->getAttribute("head"), "tmp_head");

  // stmtBuilder->addLogStatement("[push_front_with_return] tmp_head: %llu\n", {tmp_head});

  auto isListEmpty = stmtBuilder->addConditionalBranch(new dcds::expressions::IsNullExpression{tmp_head});

  {
    // isListEmpty:: IF TRUE
    //    tail = node;
    //    isListEmpty.ifBlock->addLogStatement("[push_front] EmptyList\n");
    // isListEmpty.ifBlock->addLogStatement("[push_front_with_return] here5\n");
    isListEmpty.ifBlock->addUpdateStatement(builder->getAttribute("tail"), "tmp_node");
  }
  {
    // isListEmpty:: IF FALSE
    //    node->next = head;
    //    head->prev = node;

    // isListEmpty.elseBlock->addLogStatement("[push_front] non-empty: head is %llu\n", {tmp_head});
    //  isListEmpty.elseBlock->addLogStatement("[push_front_with_return] here6\n");
    isListEmpty.elseBlock->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_node", "set_next",
                                         std::vector<std::string>{"tmp_head"});
    // isListEmpty.elseBlock->addLogStatement("[push_front_with_return] here7\n");
    isListEmpty.elseBlock->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_head", "set_prev",
                                         std::vector<std::string>{"tmp_node"});
  }

  // head = node;
  //  stmtBuilder->addLogStatement("[push_front] new head is %llu\n", {fn->getTempVariable("tmp_node")});
  // stmtBuilder->addLogStatement("[push_front_with_return] here8\n");
  stmtBuilder->addUpdateStatement(builder->getAttribute("head"), "tmp_node");
  // stmtBuilder->addLogStatement("[push_front_with_return] here9\n");

  if (builder->hasAttribute("size")) {
    auto tmpSize = fn->addTempVariable("tmp_size", builder->getAttribute("size")->type);
    stmtBuilder->addReadStatement(builder->getAttribute("size"), tmpSize);

    //    auto const_1 = std::make_shared<dcds::expressions::Int64Constant>(1);
    auto addExpr = std::make_shared<dcds::expressions::AddExpression>(tmpSize, int64_const_1);
    LOG(INFO) << "XXXX: " << addExpr->toString();
    stmtBuilder->addUpdateStatement(builder->getAttribute("size"), addExpr);
  }

  stmtBuilder->addReturnStatement("tmp_node");
}

void DoublyLinkedList2::createFunction_pushFront() {
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
  // stmtBuilder->addLogStatement("[push_front] new node: %llu\n", {fn->getTempVariable("tmp_node")});

  stmtBuilder->addMethodCall(nodeType, "tmp_node", "set_payload", "", {"push_value"});

  auto tmp_head = fn->addTempVariable("tmp_head", builder->getAttribute("head")->type);
  stmtBuilder->addReadStatement(builder->getAttribute("head"), "tmp_head");

  auto isListEmpty = stmtBuilder->addConditionalBranch(new dcds::expressions::IsNullExpression{tmp_head});

  {
    // isListEmpty:: IF TRUE
    //    tail = node;
    //    isListEmpty.ifBlock->addLogStatement("[push_front] EmptyList\n");
    isListEmpty.ifBlock->addUpdateStatement(builder->getAttribute("tail"), "tmp_node");
  }
  {
    // isListEmpty:: IF FALSE
    //    node->next = head;
    //    head->prev = node;

    //    isListEmpty.elseBlock->addLogStatement("[push_front] non-empty: head is %llu\n", {tmp_head});
    isListEmpty.elseBlock->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_node", "set_next",
                                         std::vector<std::string>{"tmp_head"});
    isListEmpty.elseBlock->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_head", "set_prev",
                                         std::vector<std::string>{"tmp_node"});
  }

  // head = node;
  //  stmtBuilder->addLogStatement("[push_front] new head is %llu\n", {fn->getTempVariable("tmp_node")});
  stmtBuilder->addUpdateStatement(builder->getAttribute("head"), "tmp_node");

  if (builder->hasAttribute("size")) {
    auto tmpSize = fn->addTempVariable("tmp_size", builder->getAttribute("size")->type);
    stmtBuilder->addReadStatement(builder->getAttribute("size"), tmpSize);

    // auto const_1 = std::make_shared<dcds::expressions::Int64Constant>(1);
    auto addExpr = std::make_shared<dcds::expressions::AddExpression>(tmpSize, int64_const_1);
    stmtBuilder->addUpdateStatement(builder->getAttribute("size"), addExpr);
  }

  stmtBuilder->addReturnVoidStatement();
}

void DoublyLinkedList2::createFunction_popBack() {
  // FIXME: this is the different from the original doublyLinkedList
  // declare void pop_back()
  auto fn = builder->createFunction("pop_back");
  auto stmtBuilder = fn->getStatementBuilder();

  auto nodeType = builder->getRegisteredType(ds_node_name);
  bool has_key_attr = nodeType->hasAttribute("key_");
  if (has_key_attr) {
    fn->addArgument("popped_key", nodeType->getAttribute("key_")->type, true);
  }

  fn->addTempVariable("tmp_tail", builder->getAttribute("tail")->type);
  stmtBuilder->addReadStatement(builder->getAttribute("tail"), "tmp_tail");

  auto conditionalBlocks =
      stmtBuilder->addConditionalBranch(new dcds::expressions::IsNullExpression{fn->getTempVariable("tmp_tail")});
  {
    // empty list
    conditionalBlocks.ifBlock->addReturnVoidStatement();
    // conditionalBlocks.ifBlock->addReturnStatement(std::make_shared<dcds::expressions::BoolConstant>(false));
  }

  {
    // tail not null, pop it
    // FIXME: to delete the popped node!

    // pop the ptr
    fn->addTempVariable("tmp_tail_prev", dcds::valueType::RECORD_PTR);
    conditionalBlocks.elseBlock->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_tail", "get_prev",
                                               "tmp_tail_prev");
    if (has_key_attr) {
      conditionalBlocks.elseBlock->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_tail", "get_key_",
                                                 "popped_key");
    }

    //    conditionalBlocks.elseBlock->addLogStatement("[pop_back] tail is %llu\n", {fn->getTempVariable("tmp_tail")});
    //    conditionalBlocks.elseBlock->addLogStatement("[pop_back] tail-prev is %llu\n",
    //                                                 {fn->getTempVariable("tmp_tail_prev")});

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
      fn->addTempVariable("nullptr_tmp", dcds::valueType::RECORD_PTR, 0);
      //      cond2.ifBlock->addLogStatement("[pop_back] nulL-val is:  %llu\n",
      //                                                   {fn->getTempVariable("nullptr_tmp")});
      cond2.ifBlock->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_tail_prev", "set_next",
                                   std::vector<std::string>{"nullptr_tmp"});
    }

    {
      cond2.elseBlock->addUpdateStatement(builder->getAttribute("head"),
                                          std::make_shared<dcds::expressions::NullPtrConstant>());
    }

    if (builder->hasAttribute("size")) {
      auto tmpSize = fn->addTempVariable("tmp_size", builder->getAttribute("size")->type);
      conditionalBlocks.elseBlock->addReadStatement(builder->getAttribute("size"), tmpSize);

      auto addExpr = std::make_shared<dcds::expressions::SubtractExpression>(tmpSize, int64_const_1);
      conditionalBlocks.elseBlock->addUpdateStatement(builder->getAttribute("size"), addExpr);
    }

    //    conditionalBlocks.elseBlock->addReturnStatement(std::make_shared<dcds::expressions::BoolConstant>(true));
    // conditionalBlocks.elseBlock->addLogStatement("[pop_back] done\n");
    conditionalBlocks.elseBlock->addReturnVoidStatement();
  }
}

void DoublyLinkedList2::createFunction_touch() {
  // void touch(node*)
  auto fn = builder->createFunction("touch");
  auto nodeArg = fn->addArgument("node_ptr", dcds::valueType::RECORD_PTR);
  auto nodeType = builder->getRegisteredType(ds_node_name);

  auto stmtBuilder = fn->getStatementBuilder();
  // stmtBuilder->addLogStatement("[touch] node-arg: %llu\n", {nodeArg});

  fn->addTempVariable("nullptr_tmp", dcds::valueType::RECORD_PTR);

  //  if (node == head) {
  //    return;
  //  } else if (node == tail) {
  //    head->prev = tail;
  //    tail->next = head;
  //    tail = tail->prev;
  //    head = head->prev;
  //    tail->next = nullptr;
  //    head->prev = nullptr;
  //  } else {
  //    node->prev->next = node->next;
  //    node->next->prev = node->prev;
  //    head->prev = node;
  //    node->next = head;
  //    node->prev = nullptr;
  //    head = node;
  //  }

  auto tmpHead = fn->addTempVariable("tmp_head", builder->getAttribute("head")->type);
  stmtBuilder->addReadStatement((*builder)["head"], tmpHead);
  // stmtBuilder->addLogStatement("[touch] tmp-head: %llu\n", {tmpHead});

  auto nodeIsHead = stmtBuilder->addConditionalBranch(new dcds::expressions::EqualExpression{tmpHead, nodeArg});

  {
    // node == head
    // nodeIsHead.ifBlock->addLogStatement("[touch] node is head: %llu\n", {fn->getTempVariable("tmp_head")});
    nodeIsHead.ifBlock->addReturnVoidStatement();
  }

  auto tmpTail = fn->addTempVariable("tmp_tail", builder->getAttribute("tail")->type);
  nodeIsHead.elseBlock->addReadStatement((*builder)["tail"], tmpTail);
  // nodeIsHead.elseBlock->addLogStatement("[touch] tmp-tail: %llu\n", {tmpTail});

  auto nodeIsTail =
      nodeIsHead.elseBlock->addConditionalBranch(new dcds::expressions::EqualExpression{tmpTail, nodeArg});

  //  nodeIsTail.ifBlock->addLogStatement("BLAH-1");
  //  nodeIsTail.ifBlock->addReturnVoidStatement();
  {
    // node == tail
    auto elseIfSb = nodeIsTail.ifBlock;

    // elseIfSb->addLogStatement("[touch] node is tail: %llu\n", {fn->getTempVariable("tmp_tail")});

    //    head->prev = tail;
    //    tail->next = head;
    //    tail = tail->prev;
    //    head = head->prev;  --> head = tail
    //    tail->next = nullptr;
    //    head->prev = nullptr;

    // equivalent to:

    /*
     * tmp_head = head
     * tmp_tail = tail
     *
     * tmp_head->prev = tmp_tail
     * tmp_tail->next = tmp_head
     *
     * tail = tmp_tail->prev
     * tail->next = nullptr
     *
     * head = tmp_tail
     * tmp_tail->prev = nullptr
     *
     * */

    //   tmp_head->prev = tmp_tail
    elseIfSb->addMethodCall(nodeType, "tmp_head", "set_prev", std::vector<std::string>{"tmp_tail"});

    //    tmp_tail->next = tmp_head
    elseIfSb->addMethodCall(nodeType, "tmp_tail", "set_next", std::vector<std::string>{"tmp_head"});

    //    tail = tmp_tail->prev
    auto tmp_tail_prev = fn->addTempVariable("tmp_tail_prev", builder->getAttribute("tail")->type);

    elseIfSb->addMethodCall(nodeType, "tmp_tail", "get_prev", "tmp_tail_prev");
    elseIfSb->addUpdateStatement(builder->getAttribute("tail"), tmp_tail_prev);

    //    tail->next = nullptr // tmp_tail_prev->next = nullptr;
    elseIfSb->addMethodCall(nodeType, "tmp_tail_prev", "set_next", std::vector<std::string>{"nullptr_tmp"});

    //   head = tmp_tail
    elseIfSb->addUpdateStatement(builder->getAttribute("head"), "tmp_tail");

    //    tmp_tail->prev = nullptr
    elseIfSb->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_tail", "set_prev",
                            std::vector<std::string>{"nullptr_tmp"});
    elseIfSb->addReturnVoidStatement();
  }

  {
    // else
    auto elseSb = nodeIsTail.elseBlock;
    // elseSb->addLogStatement("[touch] node is neither-head-nor-tail: %llu\n", {nodeArg});

    //    node->prev->next = node->next;
    //    node->next->prev = node->prev;
    //    head->prev = node;
    //    node->next = head;
    //    node->prev = nullptr;
    //    head = node;

    auto node_prev = fn->addTempVariable("node_prev", dcds::valueType::RECORD_PTR);
    auto node_next = fn->addTempVariable("node_next", dcds::valueType::RECORD_PTR);

    elseSb->addMethodCall(nodeType, "node_ptr", "get_prev", "node_prev");
    // elseSb->addLogStatement("[touch] here-1\n");
    elseSb->addMethodCall(nodeType, "node_ptr", "get_next", "node_next");
    // elseSb->addLogStatement("[touch] here-2\n");

    //    node->prev->next = node->next;
    elseSb->addMethodCall(nodeType, "node_prev", "set_next", std::vector<std::string>{"node_next"});
    // elseSb->addLogStatement("[touch] here-3\n");

    //    node->next->prev = node->prev;
    elseSb->addMethodCall(nodeType, "node_next", "set_prev", std::vector<std::string>{"node_prev"});
    // elseSb->addLogStatement("[touch] here-4\n");

    //    head->prev = node;
    elseSb->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_head", "set_prev",
                          std::vector<std::string>{"node_ptr"});
    // elseSb->addLogStatement("[touch] here-5\n");

    //    node->next = head;
    elseSb->addMethodCall(builder->getRegisteredType(ds_node_name), "node_ptr", "set_next",
                          std::vector<std::string>{"tmp_head"});
    // elseSb->addLogStatement("[touch] here-6\n");

    //    node->prev = nullptr;
    elseSb->addMethodCall(builder->getRegisteredType(ds_node_name), "node_ptr", "set_prev",
                          std::vector<std::string>{"nullptr_tmp"});
    // elseSb->addLogStatement("[touch] here-7\n");

    //    head = node;
    elseSb->addUpdateStatement(builder->getAttribute("head"), nodeArg);
    // elseSb->addLogStatement("[touch] here-8\n");

    elseSb->addReturnVoidStatement();
  }

  // stmtBuilder->addReturnVoidStatement();
}

void DoublyLinkedList2::createFunction_getSize() {
  // declare bool size()
  if (!(builder->hasAttribute("size"))) {
    LOG(WARNING) << "Doubly linked list does not have the size-attribute";
    return;
  }

  auto fn = builder->createFunction("get_size", builder->getAttribute("size")->type);

  auto stmtBuilder = fn->getStatementBuilder();

  auto tmpSize = fn->addTempVariable("tmp_size", builder->getAttribute("size")->type);

  stmtBuilder->addReadStatement(builder->getAttribute("size"), tmpSize);
  stmtBuilder->addReturnStatement(tmpSize);
}
