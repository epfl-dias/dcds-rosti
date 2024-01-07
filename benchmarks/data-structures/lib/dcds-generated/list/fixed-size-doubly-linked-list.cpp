/*
                              Copyright (c) 2024.
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
static auto int64_const_max = std::make_shared<dcds::expressions::Int64Constant>(INT64_MAX);

FixedSizedDoublyLinkedList::FixedSizedDoublyLinkedList(dcds::valueType payload_type) : dcds_generated_ds(ds_name) {
  this->generateLinkedListNode(payload_type);

  auto nodeType = builder->getRegisteredType(ds_node_name);

  builder->addAttributePtr("head", nodeType);
  builder->addAttributePtr("tail", nodeType);

  // this->createFunction_touch();
}

void FixedSizedDoublyLinkedList::generateLinkedListNode(dcds::valueType payload_type) {
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

void FixedSizedDoublyLinkedList::createFunction_init() {
  auto fn = builder->createFunction("init");
  fn->addArgument("size", dcds::valueType::INT64);

  auto stmtBuilder = fn->getStatementBuilder();

  // FOLLOWING SHOULD BE IN LOOP!
  auto nodeType = builder->getRegisteredType(ds_node_name);
  bool has_key_attr = nodeType->hasAttribute("key_");

  // cond is expression
  // inc is also expression
  //

  auto tmp_head = fn->addTempVariable("tmp_head", builder->getAttribute("head")->type);

  auto loop_var = fn->addTempVariable("loop_var_i", dcds::valueType::INT64, UINT64_C(0));
  auto for_cond = new dcds::expressions::LessThanExpression(loop_var, fn->getArgument("size"));
  auto for_inc = new dcds::expressions::AddExpression(loop_var, int64_const_1);

  // auto for_body = stmtBuilder->genForLoop(loopVar, varInit, cond, body, inc); // TODO: later
  //  auto for_body = stmtBuilder->genForLoop(cond, body, inc);

  auto for_body_builder = stmtBuilder->addForLoop(loop_var.get(), for_cond, for_inc);
  // for-body-start

  // FIXME: this will have naming issues!
  auto newNode = for_body_builder->addInsertStatement(nodeType, "tmp_node");

  // FIXME: instead-of-push-value, take a default value here (as expression).
  // stmtBuilder->addMethodCall(nodeType, "tmp_node", "set_payload", "", {"push_value"});
  for_body_builder->addMethodCall(nodeType, newNode, "set_payload", {int64_const_max});

  if (has_key_attr) {
    for_body_builder->addMethodCall(nodeType, newNode, "set_key_", {int64_const_max});
  }

  for_body_builder->addReadStatement(builder->getAttribute("head"), "tmp_head");

  auto isListEmpty = for_body_builder->addConditionalBranch(new dcds::expressions::IsNullExpression{tmp_head});

  {
    // isListEmpty:: IF TRUE
    //    tail = node;
    isListEmpty.ifBlock->addUpdateStatement(builder->getAttribute("tail"), newNode);
  }
  {
    // isListEmpty:: IF FALSE
    //    node->next = head;
    //    head->prev = node;
    isListEmpty.elseBlock->addMethodCall(builder->getRegisteredType(ds_node_name), newNode, "set_next",
                                         std::vector<std::shared_ptr<dcds::expressions::Expression>>{tmp_head});

    isListEmpty.elseBlock->addMethodCall(builder->getRegisteredType(ds_node_name), tmp_head, "set_prev",
                                         std::vector<std::shared_ptr<dcds::expressions::Expression>>{newNode});
  }

  // head = node;
  for_body_builder->addUpdateStatement(builder->getAttribute("head"), newNode);

  // for-body-end

  stmtBuilder->addReturnVoidStatement();
}

//// will-be called from LRU
// void FixedSizedDoublyLinkedList::createFunction_pushFrontWithReturn() {
//   // declare void push_front(uint64_t key, uint64_t value)  // if has key attr
//   // declare void push_front(uint64_t value)
//   auto fn = builder->createFunction("push_front_with_return", dcds::valueType::RECORD_PTR);
//   auto nodeType = builder->getRegisteredType(ds_node_name);
//
//   bool has_key_attr = nodeType->hasAttribute("key_");
//   if (has_key_attr) {
//     LOG(INFO) << "NodeType has key!";
//     fn->addArgument("push_key", nodeType->getAttribute("key_")->type);
//   }
//
//   fn->addArgument("push_value", dcds::valueType::INT64);
//
//   auto stmtBuilder = fn->getStatementBuilder();
//
//   /*
//
//    // the if-condition won't exists given the fixed-sized list will always have something. what about concurrency
//    issue?
//     if(!head){
//       tail = node;
//     } else {
//       node->next = head;
//       head->prev = node;
//     }
//     head = node;
//
//    * */
//
//
//   stmtBuilder->addInsertStatement(nodeType, "tmp_node");
//
//   stmtBuilder->addMethodCall(nodeType, "tmp_node", "set_payload", "", {"push_value"});
//   if (has_key_attr) {
//     stmtBuilder->addMethodCall(nodeType, "tmp_node", "set_key_", "", {"push_key"});
//   }
//
//   auto tmp_head = fn->addTempVariable("tmp_head", builder->getAttribute("head")->type);
//   stmtBuilder->addReadStatement(builder->getAttribute("head"), "tmp_head");
//
//   auto isListEmpty = stmtBuilder->addConditionalBranch(new dcds::expressions::IsNullExpression{tmp_head});
//
//   {
//     // isListEmpty:: IF TRUE
//     //    tail = node;
//     isListEmpty.ifBlock->addUpdateStatement(builder->getAttribute("tail"), "tmp_node");
//   }
//   {
//     // isListEmpty:: IF FALSE
//     //    node->next = head;
//     //    head->prev = node;
//
//     isListEmpty.elseBlock->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_node", "set_next",
//                                          std::vector<std::string>{"tmp_head"});
//
//     isListEmpty.elseBlock->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_head", "set_prev",
//                                          std::vector<std::string>{"tmp_node"});
//   }
//
//   // head = node;
//   stmtBuilder->addUpdateStatement(builder->getAttribute("head"), "tmp_node");
//
//   stmtBuilder->addReturnStatement("tmp_node");
// }

void FixedSizedDoublyLinkedList::createFunction_touch() {
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

// void FixedSizedDoublyLinkedList::createFunction_popBack() {
//   // declare void pop_back()
//   // declare void pop_back(key *popped_key)
//   auto fn = builder->createFunction("pop_back");
//   auto stmtBuilder = fn->getStatementBuilder();
//
//   auto nodeType = builder->getRegisteredType(ds_node_name);
//   bool has_key_attr = nodeType->hasAttribute("key_");
//   if (has_key_attr) {
//     fn->addArgument("popped_key", nodeType->getAttribute("key_")->type, true);
//   }
//
//   fn->addTempVariable("tmp_tail", builder->getAttribute("tail")->type);
//   stmtBuilder->addReadStatement(builder->getAttribute("tail"), "tmp_tail");
//
//   auto conditionalBlocks =
//       stmtBuilder->addConditionalBranch(new dcds::expressions::IsNullExpression{fn->getTempVariable("tmp_tail")});
//   {
//     // empty list
//     conditionalBlocks.ifBlock->addReturnVoidStatement();
//     // conditionalBlocks.ifBlock->addReturnStatement(std::make_shared<dcds::expressions::BoolConstant>(false));
//   }
//
//   {
//     // tail not null, pop it
//     // FIXME: to delete the popped node!
//
//     // pop the ptr
//     fn->addTempVariable("tmp_tail_prev", dcds::valueType::RECORD_PTR);
//     conditionalBlocks.elseBlock->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_tail", "get_prev",
//                                                "tmp_tail_prev");
//     if (has_key_attr) {
//       conditionalBlocks.elseBlock->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_tail", "get_key_",
//                                                  "popped_key");
//     }
//
//     //    conditionalBlocks.elseBlock->addLogStatement("[pop_back] tail is %llu\n",
//     {fn->getTempVariable("tmp_tail")});
//     //    conditionalBlocks.elseBlock->addLogStatement("[pop_back] tail-prev is %llu\n",
//     //                                                 {fn->getTempVariable("tmp_tail_prev")});
//
//     // tail = tmp_tail_prev
//     // if (tmp_tail_prev){
//     //    tmp_tail_prev->next = nullptr
//     // } else {
//     //    head = nullptr;
//     // }
//
//
//     conditionalBlocks.elseBlock->addUpdateStatement(builder->getAttribute("tail"), "tmp_tail_prev");
//     fn->addTempVariable("nullptr_tmp", dcds::valueType::RECORD_PTR, 0);
//     conditionalBlocks.elseBlock->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_tail_prev", "set_next",
//                                  std::vector<std::string>{"nullptr_tmp"});
//
//     // single-value wont exists so remove the head condition!.
////    auto cond2 = conditionalBlocks.elseBlock->addConditionalBranch(
////        new dcds::expressions::IsNotNullExpression{fn->getTempVariable("tmp_tail_prev")});
////
////    {
////      // hacked: to be passed constant expression!
////      fn->addTempVariable("nullptr_tmp", dcds::valueType::RECORD_PTR, 0);
////      //      cond2.ifBlock->addLogStatement("[pop_back] nulL-val is:  %llu\n",
////      //                                                   {fn->getTempVariable("nullptr_tmp")});
////      cond2.ifBlock->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_tail_prev", "set_next",
////                                   std::vector<std::string>{"nullptr_tmp"});
////    }
////
////    {
////      cond2.elseBlock->addUpdateStatement(builder->getAttribute("head"),
////                                          std::make_shared<dcds::expressions::NullPtrConstant>());
////    }
//
//    conditionalBlocks.elseBlock->addReturnVoidStatement();
//  }
//}

void FixedSizedDoublyLinkedList::createFunction_popBack() {
  // declare void pop_back()
  // declare void pop_back(key *popped_key)
  auto fn = builder->createFunction("pop_back");
  auto stmtBuilder = fn->getStatementBuilder();

  auto nodeType = builder->getRegisteredType(ds_node_name);
  bool has_key_attr = nodeType->hasAttribute("key_");
  if (has_key_attr) {
    fn->addArgument("popped_key", nodeType->getAttribute("key_")->type, true);
  }

  fn->addTempVariable("tmp_tail", builder->getAttribute("tail")->type);
  fn->addTempVariable("tmp_tail_prev", dcds::valueType::RECORD_PTR);
  fn->addTempVariable("nullptr_tmp", dcds::valueType::RECORD_PTR, 0);

  stmtBuilder->addReadStatement(builder->getAttribute("tail"), "tmp_tail");

  // pop the ptr
  stmtBuilder->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_tail", "get_prev", "tmp_tail_prev");
  if (has_key_attr) {
    stmtBuilder->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_tail", "get_key_", "popped_key");
  }

  stmtBuilder->addUpdateStatement(builder->getAttribute("tail"), "tmp_tail_prev");

  stmtBuilder->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_tail_prev", "set_next",
                             std::vector<std::string>{"nullptr_tmp"});

  stmtBuilder->addReturnVoidStatement();
}

// will-be called from LRU
void FixedSizedDoublyLinkedList::createFunction_pushFrontWithReturn() {
  // declare void push_front(uint64_t key, uint64_t value)  // if has key attr
  // declare void push_front(uint64_t value)
  auto fn = builder->createFunction("push_front_with_return", dcds::valueType::RECORD_PTR);
  auto nodeType = builder->getRegisteredType(ds_node_name);

  bool has_key_attr = nodeType->hasAttribute("key_");
  if (has_key_attr) {
    LOG(INFO) << "NodeType has key!";
    fn->addArgument("push_key", nodeType->getAttribute("key_")->type);
  }

  fn->addArgument("push_value", dcds::valueType::INT64);

  auto stmtBuilder = fn->getStatementBuilder();

  /*

   // the if-condition won't exists given the fixed-sized list will always have something. what about concurrency issue?
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
  if (has_key_attr) {
    stmtBuilder->addMethodCall(nodeType, "tmp_node", "set_key_", "", {"push_key"});
  }

  auto tmp_head = fn->addTempVariable("tmp_head", builder->getAttribute("head")->type);
  stmtBuilder->addReadStatement(builder->getAttribute("head"), "tmp_head");

  // isListEmpty:: IF FALSE
  //    node->next = head;
  //    head->prev = node;

  stmtBuilder->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_node", "set_next",
                             std::vector<std::string>{"tmp_head"});

  stmtBuilder->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_head", "set_prev",
                             std::vector<std::string>{"tmp_node"});

  // head = node;
  stmtBuilder->addUpdateStatement(builder->getAttribute("head"), "tmp_node");

  stmtBuilder->addReturnStatement("tmp_node");
}
