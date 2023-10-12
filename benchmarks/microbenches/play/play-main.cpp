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

  auto payloadAttribute = builder->addAttribute("payload", dcds::INTEGER, UINT64_C(50));
  auto nextAttribute = builder->addAttribute("next", dcds::RECORD_PTR, nullptr);

  LOG(INFO) << "here-1";
  builder->generateGetter(payloadAttribute);  // get_payload
                                              //  LOG(INFO) << "here-2";
  builder->generateSetter(payloadAttribute);  // set_payload

  //  LOG(INFO) << "here-3";
  //  builder->generateGetter(nextAttribute);  // get_next
  //  LOG(INFO) << "here-4";
  //  builder->generateSetter(nextAttribute);  // set_next

  return true;
}

// static void addPopFront(dcds::Builder &builder){
//
// }
//
// static void addPushFront(dcds::Builder &builder){
//
//   // add a condition of attribute has a tail, then update that.
//
//     /*
//      * auto newNode = node(val)
//      * auto tmp = head;
//      * if(tmp != nullptr)
//      *    tmp.next = newNode
//      * head = newNode
//      * */
//     auto fn = builder.createFunction("push_front", dcds::valueType::INTEGER);
//
////    auto nodeType = builder.getAttribute("LL_NODE");
//    auto nodeType = builder.getType("LL_NODE");
//
//    fn->addArgument("push_value", dcds::INTEGER);
//
//    //dsBuilder.addTempVar("one_1", dcds::valueType::INTEGER, 1, read_fn);
//    //auto tmpHead = fn->addTempVariable("tmp", nodeType);
//
//
//    //    auto add_statement =
//    //        dsBuilder.createTempVarAddStatement("counter_value_read_variable", "one_1",
//    "counter_value_read_variable");
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
//
//
//
//}

static void generateLinkedList() {
  dcds::Builder builder("LinkedList");

  auto nodeType = generateLinkedListNode(builder.createType("LL_NODE"));

  // list will have a node type, and two attribute of head/tail.

  // This should be name, and attribute. what about initial value of this attribute?
  // builder.addAttribute("head", nodeType); this is not type nodeType! This is a reference to it!!

  // TODO: how do we ensure the initial value provide is legal for the given type.
  builder.addAttribute("head", dcds::RECORD_PTR, nullptr);  // initially, the list is empty, so pointing null.

  //builder.codegen();
}

static void generateNode() {
  auto builder = std::make_shared<dcds::Builder>("LL_NODE_TEST");
  generateLinkedListNode(builder);
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
  //  LOG(INFO) << "Aunn2";
  //
  //  std::any x;
  //  std::any y = 2;
  //
  //  LOG(INFO) << x.type().name();
  //  LOG(INFO) << y.type().name();

  //  generateLinkedList();
  generateNode();

  LOG(INFO) << "generateNode -- done";

  //  llvm::LLVMContext c;
  //
  //  libcuckoo::cuckoohash_map<size_t, size_t> indexPartitions;
  return 0;
}
