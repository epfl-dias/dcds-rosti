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

DoublyLinkedList::DoublyLinkedList(dcds::valueType payload_type) : dcds_generated_ds(ds_name) {
  this->generateLinkedListNode(payload_type);

  auto nodeType = builder->getRegisteredType(ds_node_name);

  builder->addAttributePtr("head", nodeType);
  builder->addAttributePtr("tail", nodeType);

  //this->createFunction_empty();
// this->createFunction_pushFront();
  this->createFunction_popFront();  //-- segf
//  //
  //this->createFunction_pushBack();
//  this->createFunction_popBack();  //--segf
  //
  //  this->createFunction_extract();
//  this->createFunction_touch();

//  this->createFunction_getHeadPtr();
//  this->createFunction_getTailPtr();
}

void DoublyLinkedList::createFunction_getHeadPtr() {
  auto fn = builder->createFunction("head_ptr", dcds::valueType::RECORD_PTR);
  auto stmtBuilder = fn->getStatementBuilder();
  auto tmpTmpHead = fn->addTempVariable("tmp_head", builder->getAttribute("head")->type);
  stmtBuilder->addReadStatement(builder->getAttribute("head"), "tmp_head");
  stmtBuilder->addReturnStatement("tmp_head");
}

void DoublyLinkedList::createFunction_getTailPtr() {
  auto fn = builder->createFunction("tail_ptr", dcds::valueType::RECORD_PTR);
  auto stmtBuilder = fn->getStatementBuilder();
  auto tmpTmpHead = fn->addTempVariable("tmp_tail", builder->getAttribute("tail")->type);
  stmtBuilder->addReadStatement(builder->getAttribute("tail"), "tmp_tail");
  stmtBuilder->addReturnStatement("tmp_tail");
}

void DoublyLinkedList::generateLinkedListNode(dcds::valueType payload_type) {
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

void DoublyLinkedList::createFunction_empty() {
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
  stmtBuilder->addLogStatement("[push_front] new node: %llu\n", {fn->getTempVariable("tmp_node")});

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
  stmtBuilder->addReturnVoidStatement();
}
void DoublyLinkedList::createFunction_popFront() {
  // declare bool pop_front(uint64_t *val)
  auto fn = builder->createFunction("pop_front", dcds::valueType::BOOL);
  fn->addArgument("pop_value", dcds::valueType::INT64, true);

  auto stmtBuilder = fn->getStatementBuilder();

  fn->addTempVariable("tmp_head", builder->getAttribute("head")->type);
  stmtBuilder->addReadStatement(builder->getAttribute("head"), "tmp_head");

  //  stmtBuilder->addLogStatement("[pop_front] Head: %llu\n", {fn->getTempVariable("tmp_head")});

  auto conditionalBlocks =
      stmtBuilder->addConditionalBranch(new dcds::expressions::IsNullExpression{fn->getTempVariable("tmp_head")});
  conditionalBlocks.ifBlock->addReturnStatement(std::make_shared<dcds::expressions::BoolConstant>(false));
  {
    // head not null, pop it

    // get pop value
    conditionalBlocks.elseBlock->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_head", "get_payload",
                                               "pop_value");

    //    conditionalBlocks.elseBlock->addLogStatement("[pop_value] head-val is %llu\n",
    //    {fn->getArgument("pop_value")});

    // pop the ptr
    fn->addTempVariable("tmp_head_next", dcds::valueType::RECORD_PTR);
    conditionalBlocks.elseBlock->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_head", "get_next",
                                               "tmp_head_next");

    //    conditionalBlocks.elseBlock->addLogStatement("[pop_front] head-next  is %llu\n",
    //    {fn->getTempVariable("tmp_head_next")});

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
  stmtBuilder->addLogStatement("[push_back] new node: %llu\n", {fn->getTempVariable("tmp_node")});

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

    //    isListEmpty.elseBlock->addLogStatement("[push_back] Tail: %llu\n", {fn->getTempVariable("tmp_tail")});
    //    isListEmpty.elseBlock->addLogStatement("[push_back] Tail-tmpNode: %llu\n", {fn->getTempVariable("tmp_node")});

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
    //    conditionalBlocks.elseBlock->addLogStatement("[pop_back] Tail: %llu\n", {fn->getTempVariable("tmp_tail")});

    // get pop value
    conditionalBlocks.elseBlock->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_tail", "get_payload",
                                               "pop_value");
    //    conditionalBlocks.elseBlock->addLogStatement("[pop_back] tail-val is %llu\n", {fn->getArgument("pop_value")});

    // pop the ptr
    fn->addTempVariable("tmp_tail_prev", dcds::valueType::RECORD_PTR);
    conditionalBlocks.elseBlock->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_tail", "get_prev",
                                               "tmp_tail_prev");

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

    conditionalBlocks.elseBlock->addReturnStatement(std::make_shared<dcds::expressions::BoolConstant>(true));
  }
}

void DoublyLinkedList::createFunction_extract() {
  // extract-op
  // node->prev->next = node->next
  // node->next->prev = node->prev

  // not so simple, what if it was head or tail.
}
void DoublyLinkedList::createFunction_touch() {
  // void touch(node*)
  auto fn = builder->createFunction("touch");
  auto nodeArg = fn->addArgument("node_ptr", dcds::valueType::RECORD_PTR);
  auto nodeType = builder->getRegisteredType(ds_node_name);

  auto stmtBuilder = fn->getStatementBuilder();
  stmtBuilder->addLogStatement("[touch] node-arg: %llu\n", {nodeArg});

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
  stmtBuilder->addLogStatement("[touch] tmp-head: %llu\n", {tmpHead});

  auto nodeIsHead = stmtBuilder->addConditionalBranch(new dcds::expressions::EqualExpression{tmpHead, nodeArg});

  {
    // node == head
    nodeIsHead.ifBlock->addLogStatement("[touch] node is head: %llu\n", {fn->getTempVariable("tmp_head")});
    nodeIsHead.ifBlock->addReturnVoidStatement();
  }

  auto tmpTail = fn->addTempVariable("tmp_tail", builder->getAttribute("tail")->type);
  nodeIsHead.elseBlock->addReadStatement((*builder)["tail"], tmpTail);
  nodeIsHead.elseBlock->addLogStatement("[touch] tmp-tail: %llu\n", {tmpTail});

  auto nodeIsTail =
      nodeIsHead.elseBlock->addConditionalBranch(new dcds::expressions::EqualExpression{tmpTail, nodeArg});

  //  nodeIsTail.ifBlock->addLogStatement("BLAH-1");
  //  nodeIsTail.ifBlock->addReturnVoidStatement();
  {
    // node == tail
    auto elseIfSb = nodeIsTail.ifBlock;

    elseIfSb->addLogStatement("[touch] node is tail: %llu\n", {fn->getTempVariable("tmp_tail")});

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
    elseSb->addLogStatement("[touch] node is neither-head-nor-tail: %llu\n", {nodeArg});

    //    node->prev->next = node->next;
    //    node->next->prev = node->prev;
    //    head->prev = node;
    //    node->next = head;
    //    node->prev = nullptr;
    //    head = node;

    auto node_prev = fn->addTempVariable("node_prev", dcds::valueType::RECORD_PTR);
    auto node_next = fn->addTempVariable("node_next", dcds::valueType::RECORD_PTR);

    elseSb->addMethodCall(nodeType, "node_ptr", "get_prev", "node_prev");
    elseSb->addMethodCall(nodeType, "node_ptr", "get_next", "node_next");

    //    node->prev->next = node->next;
    elseSb->addMethodCall(nodeType, "node_prev", "set_next", std::vector<std::string>{"node_next"});

    //    node->next->prev = node->prev;
    elseSb->addMethodCall(nodeType, "node_next", "set_prev", std::vector<std::string>{"node_prev"});

    //    head->prev = node;
    elseSb->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_head", "set_prev",
                          std::vector<std::string>{"node_ptr"});

    //    node->next = head;
    elseSb->addMethodCall(builder->getRegisteredType(ds_node_name), "node_ptr", "set_next",
                          std::vector<std::string>{"tmp_head"});

    //    node->prev = nullptr;
    elseSb->addMethodCall(builder->getRegisteredType(ds_node_name), "node_ptr", "set_prev",
                          std::vector<std::string>{"nullptr_tmp"});

    //    head = node;
    elseSb->addUpdateStatement(builder->getAttribute("head"), nodeArg);

    elseSb->addReturnVoidStatement();
  }

  // stmtBuilder->addReturnVoidStatement();
}
