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
#include <dcds/context/DCDSContext.hpp>
#include <dcds/exporter/jit-container.hpp>
#include <dcds/util/logging.hpp>
#include <dcds/util/profiling.hpp>
#include <iostream>
#include <libcuckoo/cuckoohash_map.hh>

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
  // get the first element from the tail (does not pop)

  auto fn = builder->createFunction("front", dcds::valueType::INTEGER);
  fn->addTempVariable("tmp_head", builder->getAttribute("head")->type);
  fn->addReadStatement(builder->getAttribute("head"), "tmp_head");

  // I have the head (RECORD_PTR), of the type NODE (i know if because i coded it, somewhere in the code i can find it
  // during codegen also, thats not the issue) How to call an operation on the sub-type, as the top-level JIT exports
  // (available functions) for top-level DS. we have function in symbol table though.
  auto nodeType = builder->getRegisteredType("LL_NODE");

  fn->addLogStatement("log from inside addFront");
  fn->addTempVariable("tmp_payload_ret", dcds::INTEGER);
  fn->addMethodCall(nodeType, "tmp_head", "get_payload", "tmp_payload_ret");
  // fn->addFunctionCall("tmp_head", "get_payload", "tmp_payload_ret");
  //  (reference_variable of record_ptr, function_name, ...args)
  //  (reference_variable of record_ptr, function_name, returnValueDestinationTemporaryVariable,
  //  ...args{ofTypeTemporaryVariable/FuncARg})

  // INVALID: as we want to return integer, but for compile sake, putting tmp_head:RECORD_PTR
  fn->addReturnStatement("tmp_payload_ret");
}

static void addPushFront(std::shared_ptr<dcds::Builder>& builder) {
  // add a condition of attribute has a tail, then update that.

  /*
   * auto newNode = node(val)
   * auto tmp = head;
   * if(tmp != nullptr)
   *    tmp.next = newNode
   * head = newNode
   * */
  auto fn = builder->createFunction("push_front", dcds::valueType::INTEGER);

  auto nodeType = builder->getRegisteredType("LL_NODE");

  fn->addArgument("push_value", dcds::INTEGER);

  // auto newNode = fn->addInsertStatement(nodeType, "push_value", nullptr); // not possible as we dont have custom
  // constructors
  fn->addInsertStatement(nodeType->getName(), "tmp_node");
  fn->addMethodCall(nodeType, "tmp_node", "set_payload", "", std::string{"push_value"});
  //    instance->op("set_payload", 10);

  // can be a constant value?
  //    // fn->addCallStatement("tmp_node", "set_payload", 10);
  //    fn->addCallStatement("tmp_node", "set_payload", "push_value"); // tmp_node->set_payload(push_value);
  //
  //    fn->addTempVariable("tmp_head", builder.getAttribute("head")->type);
  //    fn->addReadStatement("head", "tmp_head");
  //
  //    fn->addCallStatement("tmp_node", "set_next", "tmp_head"); // tmp_node->set_next(tmp_head);
  //
  fn->addUpdateStatement(builder->getAttribute("head"), "tmp_node");

  // FIXME: we need conditions for checking head==null, but thats later for now.

  //    //dsBuilder.addTempVar("one_1", dcds::valueType::INTEGER, 1, read_fn);
  //    //auto tmpHead = fn->addTempVariable("tmp", nodeType);
  //
  //
  //    //    auto add_statement =
  //    //        dsBuilder.createTempVarAddStatement("counter_value_read_variable", "one_1",
  //    //"counter_value_read_variable");
  //
  //
  //    auto newNode = fn->addInsertStatement(nodeType, "push_value", nullptr);
  //    fn->addReadStatement(builder.getAttribute("head"), tmpHead);
  //
  //    //  if tmpHead is nullptr then directly assign
  //    std::vector<std::shared_ptr<dcds::StatementBuilder>> ifHeadNotNull;
  //    auto cond = dcds::ConditionBuilder(dcds::CmpIPredicate::neq, tmpHead, nullptr);
  //    ifHeadNotNull.emplace_back(nodeType.createCallStatement(tmpHead, "set_next", newNode));
  //
  //    // TODO: add this createCallStatement in list.
  //    fn->createConditionalStatement(cond, ifHeadNotNull);
  //
  //    fn->addUpdateStatement(builder.getAttribute("head"), newNode);
  //
  //
  //
  //    // TODO: add condition, if tail attribute exists, then update tail if necessary.

  fn->addTempVariable("tmp_ret", dcds::valueType::INTEGER, UINT64_C(1));
  fn->addReturnStatement("tmp_ret");
}

static void generateLinkedList() {
  auto builder = std::make_shared<dcds::Builder>("LinkedList");
  auto nodeType = generateLinkedListNode(builder->createType("LL_NODE"));

  // list will have a node type, and two attribute of head/tail.

  // This should be named, and attribute. what about initial value of this attribute?
  // builder.addAttribute("head", nodeType); this is not type nodeType! This is a reference to it!!

  // TODO: how do we ensure the initial value provide is legal for the given type.
  builder->addAttribute("head", dcds::RECORD_PTR, nullptr);  // initially, the list is empty, so pointing null.
  builder->addAttribute("tail", dcds::RECORD_PTR, nullptr);  // initially, the list is empty, so pointing null.

  builder->addAttributePointerType("LL_NODE", "head");
  builder->addAttributePointerType("LL_NODE", "tail");

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

int main(int argc, char** argv) {
  LOG(INFO) << "main() -- start";
  generateLinkedList();
  //     generateNode();

  LOG(INFO) << "main() -- done";
  return 0;
}
