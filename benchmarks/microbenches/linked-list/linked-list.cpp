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

#include <benchmark/benchmark.h>

//#include <dcds/builder/builder.hpp>
#include <dcds/util/logging.hpp>
#include <dcds/util/profiling.hpp>
#include <iostream>

/*

 Singly Linked List
  Attributes for each record:
    payload<T>
    next

  Global attributes:
    Head
    Tail

  push()
      BEGIN

      new_node := INSERT into node(payload, null)
      UPDATE head set next = new_node
      UPDATE list-table SET head = new_node



    // DEFINE ATTRIBUTES/ Structures

    list_node_struct = builder.createSchema("list_node")
    list_node_struct.add_numeric_attribute("payload", types::INTEGER)
    list_node_struct.add_ptr_attribute("next", types::PTR, list_node_struct) // takes referencing schema
    list_node_struct.finalize() // marking node as sealed. would it help, no idea.

    struct_LinkedList = builder.createSchema("linked_list");
    struct_LinkedList.add_attribute("list_head", types::PTR, list_node_struct) // catch is that this is ptr to a
 different table! struct_LinkedList.add_attribute("list_tail", types::PTR, list_node_struct),


    All structures are basically tables, and each attribute will be columns in it.
    All structures will provide DML statements like: insert/update/delete



    // DEFINE operations

    // createFunction
    //    std:string  name
    //    Arguments (std::vector<std::string>)
    //      ? Args... or std::tuple<Args...?>
    //        Actually a map would be better, then we will have names also.
    //    returnType? or deduce from last statement/op?


    push_func = builder.createFunction("pushFront", {"payload"})


    headRec = INSERT INTO list_node_struct
      values(payload, select list_head from struct_LinkedList)
      RETURNING *
    UPDATE struct_LinkedList set list_head = headRec.rowID;

    if(headRec is null)
      UPDATE tail also.



    struct_LinkedList.getRecord();

    nodeRef = list_node.insert(payload, null);
    struct_LinkedList.update()




 // hamish


 addStatement([IRBuilder b](){

    node = b.getType("node").create(payload, null); // {payload, next}


    auto head = b.getAttribute("head");
    if(head.isNotNUllPtr)  // specialized additional function for PtrType
      b.updateAttribute("head", node);


 })


*/

static void createPushFrontOperation(dcds::Builder &dsBuilder) {
  // pushFunction:: functionBuilder(name, returnType)
  auto pushFunction = dsBuilder.createFunction("push_front", null);

  pushFunction.addArgument("payload", dcds::INTEGER);

  // structName, argumentName (Arg...)
  auto insertedNode = pushFunction.addInsertStatement("list_node", "payload");

  // will fetch the node reference by attribute head
  auto currentHead = pushFunction.addGetStatement("list_node", dsBuilder.getAttribute("head"));

  auto headIsNotNull = pushFunction.createStatemetBlock();

  auto headIsNotNull_updateNodeNext =
      headIsNotNull.addUpdateStatement("list_node", insertedNode, {dsBuilder.getAttribute("next"), currentHead});
  auto headIsNotNull_updateTail = headIsNotNull.updateAttribute("tail", insertedNode);

  // void addConditionalStatement(struct, attributeInStruct, predicate, statementWhenTrue, statementWhenFalse)
  pushFunction.addConditionalStatment(headNode, "next", predicate::EQUALS, headIsNotNull, null);

  auto updateHead = pushFunction.updateAttribute("head", insertedNode);

  pushFunction.addVoidReturn();
}

static void createPopBackOperation(dcds::Builder &dsBuilder) {
  // createFunction(name, returnType, arguments)
  auto popFunc =
      dsBuilder.createFunction("pop_back", types::BOOL, referenceType(listNodeStruct.getAttribute("payload").type));

  // listNodeStruct.getAttribute("payload").type);

  auto currentTail = popFunc.addGetStatement("list_node", dsBuilder.getAttribute("tail"));

  auto currentTailNull = popFunc.createStatemetBlock();
  // when tail null, return false.
  currentTailNull.addReturnStatement(type::BOOL, false);

  // Actually this cant happen, as we do not have the back reference in single-list.
  auto currentTailIsNotNull = popFunc.createStatemetBlock();
  // when tail not null, save to reference, update tail, return true.

  // currentTailNotNull
  popFunc.addConditionalStatement(currentTail, predicte::NOT_NULL, currentTailIsNotNull, currentTailNull);
}

static void func() {
  dcds::Builder dsBuilder("linkedList");

  auto listNodeStruct = builder::createStruct("list_node");
  listNodeStruct.addAttribute("payload", dcds::INTEGER, 0);            // defaultValue
  listNodeStruct.addAttribute("next", dcds::RECORD_PTR, "list_node");  // defaultValue should be nullPtr here

  dsBuilder.addStruct(listNodeStruct);

  //    auto listStruct = builder::createStruct("list");
  //    listNodeStruct.addAttribute("head", dcds::RECORD_PTR, "list_node");
  //    listNodeStruct.addAttribute("tail", dcds::RECORD_PTR, "list_node");
  //    dsBuilder.addStruct(listStruct);

  dsBuilder.addAttribute("head", dcds::RECORD_PTR, "list_node");
  dsBuilder.addAttribute("tail", dcds::RECORD_PTR, "list_node");
}

int main(int argc, char **argv) { return 0; }
