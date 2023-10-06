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
// Created by prathamesh on 21/8/23.
//

#include "dcds/builder/builder.hpp"
#include "dcds/context/DCDSContext.hpp"

class DCDS_Node {
 public:
  DCDS_Node(int64_t payload = 0) {
    dcds::DCDSContext context(false, false);
    dcds::Builder dsBuilder(context, "Node");

    dsBuilder.addAttribute("payload", dcds::INTEGER, payload);
    dsBuilder.addAttribute("next_reference", dcds::RECORD_PTR, nullptr);
    dsBuilder.addAttribute("prev_reference", dcds::RECORD_PTR, nullptr);

    storageObject = dsBuilder.initializeStorage();
    storageObjectPtr = reinterpret_cast<void *>(storageObject.get());
  }

  static void initialize() {
    // context(enable_multithreading_flag, allow_dangling_functions_flag)
    dcds::DCDSContext context(false, false);
    // builder(DCDSContext, data_structure_name)
    dcds::Builder dsBuilder(context, "Node");

    // addAttribute(attribute_name, attribute_type, initial_value)
    dsBuilder.addAttribute("payload", dcds::INTEGER, 0);
    dsBuilder.addAttribute("next_reference", dcds::RECORD_PTR, nullptr);
    dsBuilder.addAttribute("prev_reference", dcds::RECORD_PTR, nullptr);

    // createFunction(function_name, function_return_type, <optional>function_arguments_types)
    auto read_payload_fn = dsBuilder.createFunction("read_payload_function", dcds::valueType::INTEGER);
    auto read_next_ref_fn = dsBuilder.createFunction("read_next_ref_function", dcds::valueType::RECORD_PTR);
    auto read_prev_ref_fn = dsBuilder.createFunction("read_prev_ref_function", dcds::valueType::RECORD_PTR);

    auto write_fn = dsBuilder.createFunction("write_function", dcds::valueType::VOID, dcds::valueType::INTEGER,
                                             dcds::valueType::RECORD_PTR, dcds::valueType::RECORD_PTR);

    // addTempVar(temp_var_name, temp_var_type, temp_var_initial_value, function_to_which_it_is_attached)
    dsBuilder.addTempVar("node_read_payload_variable", dcds::valueType::INTEGER, 0, read_payload_fn);
    dsBuilder.addTempVar("node_read_next_ref_variable", dcds::valueType::RECORD_PTR, nullptr, read_next_ref_fn);
    dsBuilder.addTempVar("node_read_prev_ref_variable", dcds::valueType::RECORD_PTR, nullptr, read_prev_ref_fn);

    // addArgVar(arg_var_name, arg_var_type, function_to_which_it_is_attached)
    dsBuilder.addArgVar("node_write_payload_variable", dcds::valueType::INTEGER, write_fn);
    dsBuilder.addArgVar("node_write_next_ref_variable", dcds::valueType::RECORD_PTR, write_fn);
    dsBuilder.addArgVar("node_write_prev_ref_variable", dcds::valueType::RECORD_PTR, write_fn);

    // createReadStatement(attribute_to_be_read_from, temp_var_name_which_will_have_read_value)
    auto read_payload_statement =
        dsBuilder.createReadStatement(dsBuilder.getAttribute("payload"), "node_read_payload_variable");
    auto read_next_ref_statement =
        dsBuilder.createReadStatement(dsBuilder.getAttribute("next_reference"), "node_read_next_ref_variable");
    auto read_prev_ref_statement =
        dsBuilder.createReadStatement(dsBuilder.getAttribute("prev_reference"), "node_read_prev_ref_variable");

    // createUpdateStatement(attribute_to_be_written_to, temp_var_which_will_have_value_to_write)
    auto write_payload_statement =
        dsBuilder.createUpdateStatement(dsBuilder.getAttribute("payload"), "node_write_payload_variable");
    auto write_next_ref_statement =
        dsBuilder.createUpdateStatement(dsBuilder.getAttribute("next_reference"), "node_write_next_ref_variable");
    auto write_prev_ref_statement =
        dsBuilder.createUpdateStatement(dsBuilder.getAttribute("prev_reference"), "node_write_prev_ref_variable");

    // createReturnStatement(temp_var_name_which_is_to_be_returned)
    auto return_payload_statement = dsBuilder.createReturnStatement("node_read_payload_variable");
    auto return_next_ref_statement = dsBuilder.createReturnStatement("node_read_next_ref_variable");
    auto return_prev_ref_statement = dsBuilder.createReturnStatement("node_read_prev_ref_variable");

    // addStatement(statement, function_to_which_it_should_be_added)
    dsBuilder.addStatement(read_payload_statement, read_payload_fn);
    dsBuilder.addStatement(return_payload_statement, read_payload_fn);

    dsBuilder.addStatement(read_next_ref_statement, read_next_ref_fn);
    dsBuilder.addStatement(return_next_ref_statement, read_next_ref_fn);

    dsBuilder.addStatement(read_prev_ref_statement, read_prev_ref_fn);
    dsBuilder.addStatement(return_prev_ref_statement, read_prev_ref_fn);

    dsBuilder.addStatement(write_payload_statement, write_fn);
    dsBuilder.addStatement(write_next_ref_statement, write_fn);
    dsBuilder.addStatement(write_prev_ref_statement, write_fn);

    // addFunction(function_to_be_added)
    dsBuilder.addFunction(read_payload_fn);
    dsBuilder.addFunction(read_next_ref_fn);
    dsBuilder.addFunction(read_prev_ref_fn);

    dsBuilder.addFunction(write_fn);

    // Generate code
    visitor = dsBuilder.codegen();

    built_read_payload_fn = visitor->getBuiltFunctionInt64ReturnType("read_payload_function");
    built_read_next_ref_fn = visitor->getBuiltFunctionVoidPtrReturnType("read_next_ref_function");
    built_read_prev_ref_fn = visitor->getBuiltFunctionVoidPtrReturnType("read_prev_ref_function");

    built_write_fn = visitor->getBuiltFunctionVoidReturnTypeIntArg1VoidPtr2("write_function");
  }

  auto read_payload() { return built_read_payload_fn(storageObjectPtr); }
  auto read_next_ref() { return built_read_next_ref_fn(storageObjectPtr); }
  auto read_prev_ref() { return built_read_prev_ref_fn(storageObjectPtr); }

  auto write(int64_t *payload, void *next_reference, void *prev_reference) {
    built_write_fn(storageObjectPtr, payload, next_reference, prev_reference);
  }

 private:
  static void (*built_write_fn)(void *, int64_t *, void *, void *);
  static void *(*built_read_prev_ref_fn)(void *);
  static void *(*built_read_next_ref_fn)(void *);
  static int64_t (*built_read_payload_fn)(void *);

 public:
  static std::shared_ptr<dcds::Visitor> visitor;
  std::shared_ptr<dcds::StorageLayer> storageObject;
  void *storageObjectPtr;
};

void (*DCDS_Node::built_write_fn)(void *, int64_t *, void *, void *);
void *(*DCDS_Node::built_read_prev_ref_fn)(void *);
void *(*DCDS_Node::built_read_next_ref_fn)(void *);
int64_t (*DCDS_Node::built_read_payload_fn)(void *);
std::shared_ptr<dcds::Visitor> DCDS_Node::visitor;

class DCDS_DL_LinkedList {
 public:
  void push_front(int64_t payload) {
    auto new_node_ptr = new DCDS_Node;
    new_node_ptr->write(&payload, nullptr, front);

    if (front != nullptr) {
      auto front_val = front->read_payload();

      front->write(&front_val, new_node_ptr, front->read_prev_ref());
    } else {
      back = new_node_ptr;
    }
    front = new_node_ptr;
    ++size;
  }

  auto pop_back() {
    if (back == nullptr) return static_cast<int64_t>(-1);

    int64_t return_val = back->read_payload();
    if (front != back) {
      back = reinterpret_cast<DCDS_Node *>(back->read_next_ref());
    } else {
      back = nullptr;
      front = nullptr;
    }

    --size;
    return return_val;
  }

  auto find(int64_t payload_key) {
    DCDS_Node *ref_ptr = back;

    for (int64_t i = 0; i < size; ++i) {
      if (ref_ptr->read_payload() == payload_key) return 1;
      ref_ptr = reinterpret_cast<DCDS_Node *>(ref_ptr->read_next_ref());
    }

    return 0;
  }

  void print() {
    DCDS_Node *ref_ptr = front;

    for (int64_t i = 0; i < size; ++i) {
      LOG(INFO) << ref_ptr->read_payload();
      ref_ptr = reinterpret_cast<DCDS_Node *>(ref_ptr->read_prev_ref());
    }
  }

  void delete_elem(int64_t payload_key) {
    DCDS_Node *ref_ptr = back;

    for (int64_t i = 0; i < size; ++i) {
      if (ref_ptr->read_payload() == payload_key) {
        if (ref_ptr != front && ref_ptr != back) {
          auto ref_ptr_prev_ref_val = reinterpret_cast<DCDS_Node *>(ref_ptr->read_prev_ref())->read_payload();

          reinterpret_cast<DCDS_Node *>(ref_ptr->read_prev_ref())
              ->write(&ref_ptr_prev_ref_val, ref_ptr->read_next_ref(),
                      reinterpret_cast<DCDS_Node *>(ref_ptr->read_prev_ref())->read_prev_ref());
        } else if (ref_ptr == back && ref_ptr != front) {
          back = reinterpret_cast<DCDS_Node *>(ref_ptr->read_next_ref());
        } else if (ref_ptr == front && ref_ptr != back) {
          front = reinterpret_cast<DCDS_Node *>(ref_ptr->read_prev_ref());
        } else if (ref_ptr == front && ref_ptr == back) {
          front = nullptr;
          back = nullptr;
        }

        delete ref_ptr;
        --size;
        return;
      }
      ref_ptr = reinterpret_cast<DCDS_Node *>(ref_ptr->read_next_ref());
    }
  }

  auto getFront() { return front; }

  auto getBack() { return back; }

  auto getSize() { return size; }

 private:
  DCDS_Node *front = nullptr;
  DCDS_Node *back = nullptr;
  intptr_t size = 0;
};

class DCDS_LRU_List {
 public:
  DCDS_LRU_List(int max_size_) : max_size(max_size_) {}

  void refer(int key) {
    if (hash_map.find(key) == hash_map.end()) {
      if (dll.getSize() == max_size) {
        hash_map.erase(dll.pop_back());
      }
    } else {
      dll.delete_elem(hash_map[key]->read_payload());
      delete hash_map[key];
    }

    dll.push_front(key);
    hash_map[key] = new DCDS_Node(key);
  }

  void display() { dll.print(); }

 private:
  DCDS_DL_LinkedList dll;
  std::unordered_map<int, DCDS_Node *> hash_map;
  int max_size;
};

static void check1(DCDS_LRU_List *dli) {
  dli->refer(1);
  dli->refer(2);
  dli->refer(3);
  dli->refer(1);
  dli->refer(4);
  dli->refer(5);
}

int main() {
  DCDS_Node::initialize();
  DCDS_LRU_List dli1(4);

  check1(&dli1);
  dli1.display();

  return 0;
}
