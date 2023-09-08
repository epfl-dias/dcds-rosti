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

//
// Created by prathamesh on 22/8/23.
//

#include "dcds/builder/builder.hpp"
#include "dcds/context/DCDSContext.hpp"

#include "node.hpp"

void DCDS_Node::initialize() {
    dcds::DCDSContext context(false, false);
    dcds::Builder dsBuilder(context, "Node");

    dsBuilder.addAttribute("payload", dcds::INTEGER, 0);
    dsBuilder.addAttribute("next_reference", dcds::RECORD_PTR, nullptr);
    dsBuilder.addAttribute("prev_reference", dcds::RECORD_PTR, nullptr);

    auto read_payload_fn = dsBuilder.createFunction("read_payload_function", dcds::valueType::INTEGER);
    auto read_next_ref_fn = dsBuilder.createFunction("read_next_ref_function", dcds::valueType::RECORD_PTR);
    auto read_prev_ref_fn = dsBuilder.createFunction("read_prev_ref_function", dcds::valueType::RECORD_PTR);

    auto write_payload_fn =
            dsBuilder.createFunction("write_payload_function", dcds::valueType::VOID, dcds::valueType::INTEGER);
    auto write_next_ref_fn =
            dsBuilder.createFunction("write_next_ref_function", dcds::valueType::VOID, dcds::valueType::RECORD_PTR);
    auto write_prev_ref_fn =
            dsBuilder.createFunction("write_prev_ref_function", dcds::valueType::VOID, dcds::valueType::RECORD_PTR);

    dsBuilder.addTempVar("node_read_payload_variable", dcds::valueType::INTEGER, 0, read_payload_fn);
    dsBuilder.addTempVar("node_read_next_ref_variable", dcds::valueType::RECORD_PTR, nullptr, read_next_ref_fn);
    dsBuilder.addTempVar("node_read_prev_ref_variable", dcds::valueType::RECORD_PTR, nullptr, read_prev_ref_fn);

    dsBuilder.addArgVar("node_write_payload_variable", dcds::valueType::INTEGER, write_payload_fn);
    dsBuilder.addArgVar("node_write_next_ref_variable", dcds::valueType::RECORD_PTR, write_next_ref_fn);
    dsBuilder.addArgVar("node_write_prev_ref_variable", dcds::valueType::RECORD_PTR, write_prev_ref_fn);

    auto read_payload_statement =
            dsBuilder.createReadStatement(dsBuilder.getAttribute("payload"), "node_read_payload_variable");
    auto read_next_ref_statement =
            dsBuilder.createReadStatement(dsBuilder.getAttribute("next_reference"), "node_read_next_ref_variable");
    auto read_prev_ref_statement =
            dsBuilder.createReadStatement(dsBuilder.getAttribute("prev_reference"), "node_read_prev_ref_variable");

    auto write_payload_statement =
            dsBuilder.createUpdateStatement(dsBuilder.getAttribute("payload"), "node_write_payload_variable");
    auto write_next_ref_statement =
            dsBuilder.createUpdateStatement(dsBuilder.getAttribute("next_reference"), "node_write_next_ref_variable");
    auto write_prev_ref_statement =
            dsBuilder.createUpdateStatement(dsBuilder.getAttribute("prev_reference"), "node_write_prev_ref_variable");

    auto return_payload_statement = dsBuilder.createReturnStatement("node_read_payload_variable");
    auto return_next_ref_statement = dsBuilder.createReturnStatement("node_read_next_ref_variable");
    auto return_prev_ref_statement = dsBuilder.createReturnStatement("node_read_prev_ref_variable");

    dsBuilder.addStatement(read_payload_statement, read_payload_fn);
    dsBuilder.addStatement(return_payload_statement, read_payload_fn);

    dsBuilder.addStatement(read_next_ref_statement, read_next_ref_fn);
    dsBuilder.addStatement(return_next_ref_statement, read_next_ref_fn);

    dsBuilder.addStatement(read_prev_ref_statement, read_prev_ref_fn);
    dsBuilder.addStatement(return_prev_ref_statement, read_prev_ref_fn);

    dsBuilder.addStatement(write_payload_statement, write_payload_fn);
    dsBuilder.addStatement(write_next_ref_statement, write_next_ref_fn);
    dsBuilder.addStatement(write_prev_ref_statement, write_prev_ref_fn);

    dsBuilder.addFunction(read_payload_fn);
    dsBuilder.addFunction(read_next_ref_fn);
    dsBuilder.addFunction(read_prev_ref_fn);

    dsBuilder.addFunction(write_payload_fn);
    dsBuilder.addFunction(write_next_ref_fn);
    dsBuilder.addFunction(write_prev_ref_fn);

    visitor = dsBuilder.codegen();

    built_read_payload_fn = visitor->getBuiltFunctionInt64ReturnType("read_payload_function");
    built_read_next_ref_fn = visitor->getBuiltFunctionVoidPtrReturnType("read_next_ref_function");
    built_read_prev_ref_fn = visitor->getBuiltFunctionVoidPtrReturnType("read_prev_ref_function");

    built_write_payload_fn = visitor->getBuiltFunctionVoidReturnTypeIntArg1("write_payload_function");
    built_write_next_ref_fn = visitor->getBuiltFunctionVoidReturnTypeVoidArg1("write_next_ref_function");
    built_write_prev_ref_fn = visitor->getBuiltFunctionVoidReturnTypeVoidArg1("write_prev_ref_function");
}

class DCDS_DL_LinkedList {
 public:
  DCDS_DL_LinkedList() {
    dcds::DCDSContext context(false, false);
    dcds::Builder dsBuilder(context, "DL_LinkedList");

    dsBuilder.addAttribute("size", dcds::valueType::INTEGER, 0);
    dsBuilder.addAttribute("front", dcds::valueType::RECORD_PTR, nullptr);
    dsBuilder.addAttribute("back", dcds::valueType::RECORD_PTR, nullptr);

    storageObject = dsBuilder.initializeStorage();
    storageObjectPtr = reinterpret_cast<void *>(storageObject.get());
  }

  static void initialize() {
    dcds::DCDSContext context(false, false);
    dcds::Builder dsBuilder(context, "DL_LinkedList");

    dsBuilder.addAttribute("size", dcds::INTEGER, 0);
    dsBuilder.addAttribute("front", dcds::valueType::RECORD_PTR, nullptr);
    dsBuilder.addAttribute("back", dcds::valueType::RECORD_PTR, nullptr);

    auto get_front_function = dsBuilder.createFunction("get_front_fn", dcds::valueType::RECORD_PTR);

    dsBuilder.addTempVar("temp_front", dcds::valueType::RECORD_PTR, nullptr, get_front_function);
    auto read_front = dsBuilder.createReadStatement(dsBuilder.getAttribute("front"), "temp_front");
    auto return_front = dsBuilder.createReturnStatement("temp_front");

    dsBuilder.addStatement(read_front, get_front_function);
    dsBuilder.addStatement(return_front, get_front_function);
    dsBuilder.addFunction(get_front_function);

    auto get_back_function = dsBuilder.createFunction("get_back_fn", dcds::valueType::RECORD_PTR);

    dsBuilder.addTempVar("temp_back", dcds::valueType::RECORD_PTR, nullptr, get_back_function);
    auto read_back = dsBuilder.createReadStatement(dsBuilder.getAttribute("back"), "temp_back");
    auto return_back = dsBuilder.createReturnStatement("temp_back");

    dsBuilder.addStatement(read_back, get_back_function);
    dsBuilder.addStatement(return_back, get_back_function);
    dsBuilder.addFunction(get_back_function);

    auto get_size_function = dsBuilder.createFunction("get_size_fn", dcds::valueType::INTEGER);

    dsBuilder.addTempVar("temp_size", dcds::valueType::INTEGER, 0, get_size_function);
    auto read_size = dsBuilder.createReadStatement(dsBuilder.getAttribute("size"), "temp_size");
    auto return_size = dsBuilder.createReturnStatement("temp_size");

    dsBuilder.addStatement(read_size, get_size_function);
    dsBuilder.addStatement(return_size, get_size_function);
    dsBuilder.addFunction(get_size_function);

    auto push_front_function = dsBuilder.createFunction("push_front_fn", dcds::valueType::VOID,
                                                        dcds::valueType::RECORD_PTR, dcds::valueType::RECORD_PTR);

    dsBuilder.addArgVar("new_node_ptr", dcds::valueType::RECORD_PTR, push_front_function);
    dsBuilder.addArgVar("new_node_ptr_storage_ref", dcds::valueType::RECORD_PTR, push_front_function);
    dsBuilder.addTempVar("null_ptr_variable", dcds::valueType::RECORD_PTR, nullptr, push_front_function);
    dsBuilder.addTempVar("temp_front_var", dcds::valueType::RECORD_PTR, nullptr, push_front_function);
    dsBuilder.addTempVar("const_one_var", dcds::valueType::INTEGER, 1, push_front_function);
    dsBuilder.addTempVar("temp_size_var", dcds::valueType::INTEGER, 0, push_front_function);

    auto getFront = dsBuilder.createReadStatement(dsBuilder.getAttribute("front"), "temp_front_var");
    auto changePrevForNewNode = dsBuilder.createCallStatement2VoidPtrArgs("write_prev_ref_function",
                                                                          "new_node_ptr_storage_ref", "temp_front_var");

    auto cond = dcds::ConditionBuilder(dcds::CmpIPredicate::neq, "temp_front_var", "null_ptr_variable");
    std::vector<std::shared_ptr<dcds::StatementBuilder>> ifResStatements, elseResStatements;
    ifResStatements.emplace_back(dsBuilder.createCallStatement2VoidPtrArgs("link_existing_front", "temp_front_var", "new_node_ptr"));
    elseResStatements.emplace_back(dsBuilder.createUpdateStatement(dsBuilder.getAttribute("back"), "new_node_ptr"));
    auto cond_statement = dsBuilder.createConditionStatement(cond, ifResStatements, elseResStatements);

    auto changeFront = dsBuilder.createUpdateStatement(dsBuilder.getAttribute("front"), "new_node_ptr");
    auto getSize = dsBuilder.createReadStatement(dsBuilder.getAttribute("size"), "temp_size_var");
    auto incrementSize = dsBuilder.createTempVarAddStatement("temp_size_var", "const_one_var", "temp_size_var");
    auto updateSize = dsBuilder.createUpdateStatement(dsBuilder.getAttribute("size"), "temp_size_var");

    dsBuilder.addStatement(getFront, push_front_function);
    dsBuilder.addStatement(changePrevForNewNode, push_front_function);
    dsBuilder.addStatement(cond_statement, push_front_function);
    dsBuilder.addStatement(changeFront, push_front_function);
    dsBuilder.addStatement(getSize, push_front_function);
    dsBuilder.addStatement(incrementSize, push_front_function);
    dsBuilder.addStatement(updateSize, push_front_function);
      dsBuilder.addStatement(dsBuilder.createUpdateStatement(dsBuilder.getAttribute("back"), "new_node_ptr"), push_front_function);


    //    auto assign_new_to_back = dsBuilder.createUpdateStatement(dsBuilder.getAttribute("back"), "new_node_ptr");
    //    dsBuilder.addStatement(assign_new_to_back, push_front_function);
    //    dsBuilder.addStatement(changeFront, push_front_function);

    dsBuilder.addFunction(push_front_function);

    visitorDLLL = dsBuilder.codegen();

    built_read_size_fn = visitorDLLL->getBuiltFunctionInt64ReturnType("get_size_fn");
    built_read_front_fn = visitorDLLL->getBuiltFunctionVoidPtrReturnType("get_front_fn");
    built_read_back_fn = visitorDLLL->getBuiltFunctionVoidPtrReturnType("get_back_fn");
    built_push_front_fn = visitorDLLL->getBuiltFunctionVoidReturnTypeVoidArg2("push_front_fn");
  }

  void push_front(void *node_ref, void *node_ref_storage_ptr) {
    built_push_front_fn(storageObjectPtr, node_ref, node_ref_storage_ptr);
  }

  int64_t get_size() { return built_read_size_fn(storageObjectPtr); }

  void *get_front() { return built_read_front_fn(storageObjectPtr); }

  void *get_back() { return built_read_back_fn(storageObjectPtr); }

 private:
  static int64_t (*built_read_size_fn)(void *);
  static void *(*built_read_front_fn)(void *);
  static void *(*built_read_back_fn)(void *);
  static void (*built_push_front_fn)(void *, void *, void *);
  static std::shared_ptr<dcds::Visitor> visitorDLLL;
  std::shared_ptr<dcds::StorageLayer> storageObject;
  void *storageObjectPtr;
};

int64_t (*DCDS_DL_LinkedList::built_read_size_fn)(void *);
void *(*DCDS_DL_LinkedList::built_read_front_fn)(void *);
void *(*DCDS_DL_LinkedList::built_read_back_fn)(void *);
void (*DCDS_DL_LinkedList::built_push_front_fn)(void *, void *, void *);
std::shared_ptr<dcds::Visitor> DCDS_DL_LinkedList::visitorDLLL;

int main() {
  DCDS_Node::initialize();
  DCDS_DL_LinkedList::initialize();

  DCDS_Node n1(5);

  DCDS_DL_LinkedList dll;

  // LOG(INFO) << n1.read_payload();

  // LOG(INFO) << dll.get_front();
  // LOG(INFO) << dll.get_back();

  dll.push_front(&n1, n1.storageObjectPtr);

  // LOG(INFO) << n1.storageObjectPtr;
  // LOG(INFO) << dll.get_front();
  // LOG(INFO) << dll.get_back();
  // LOG(INFO) << &n1;
  // LOG(INFO) << n1.read_prev_ref();
  // LOG(INFO) << n1.read_next_ref();

  //  // LOG(INFO) << dll.get_front();
  //  // LOG(INFO) << dll.get_back();
    // LOG(INFO) << dll.get_size();

  return 0;
}
