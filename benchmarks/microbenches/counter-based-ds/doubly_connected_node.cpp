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
    DCDS_Node() {
        dcds::DCDSContext context(false, false);
        dcds::Builder dsBuilder(context, "Node");

        dsBuilder.addAttribute("payload", dcds::INTEGER, 0);
        dsBuilder.addAttribute("next_reference", dcds::RECORD_PTR, nullptr);
        dsBuilder.addAttribute("prev_reference", dcds::RECORD_PTR, nullptr);

        storageObject = dsBuilder.initializeStorage();
        storageObjectPtr = reinterpret_cast<void *>(storageObject.get());
    }

    static void initialize() {
        dcds::DCDSContext context(false, false);
        dcds::Builder dsBuilder(context, "Node");

        dsBuilder.addAttribute("payload", dcds::INTEGER, 0);
        dsBuilder.addAttribute("next_reference", dcds::RECORD_PTR, nullptr);
        dsBuilder.addAttribute("prev_reference", dcds::RECORD_PTR, nullptr);

        auto read_payload_fn = dsBuilder.createFunction("read_payload_function", dcds::valueType::INTEGER);
        auto read_next_ref_fn = dsBuilder.createFunction("read_next_ref_function", dcds::valueType::RECORD_PTR);
        auto read_prev_ref_fn = dsBuilder.createFunction("read_prev_ref_function", dcds::valueType::RECORD_PTR);

        auto write_fn = dsBuilder.createFunction("write_function", dcds::valueType::VOID, dcds::valueType::INTEGER,
                                                 dcds::valueType::RECORD_PTR, dcds::valueType::RECORD_PTR);

        dsBuilder.addTempVar("node_read_payload_variable", dcds::valueType::INTEGER, 0, read_payload_fn);
        dsBuilder.addTempVar("node_read_next_ref_variable", dcds::valueType::RECORD_PTR, nullptr, read_next_ref_fn);
        dsBuilder.addTempVar("node_read_prev_ref_variable", dcds::valueType::RECORD_PTR, nullptr, read_prev_ref_fn);

        dsBuilder.addArgVar("node_write_payload_variable", dcds::valueType::INTEGER, write_fn);
        dsBuilder.addArgVar("node_write_next_ref_variable", dcds::valueType::RECORD_PTR, write_fn);
        dsBuilder.addArgVar("node_write_prev_ref_variable", dcds::valueType::RECORD_PTR, write_fn);

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

        dsBuilder.addStatement(write_payload_statement, write_fn);
        dsBuilder.addStatement(write_next_ref_statement, write_fn);
        dsBuilder.addStatement(write_prev_ref_statement, write_fn);

        dsBuilder.addFunction(read_payload_fn);
        dsBuilder.addFunction(read_next_ref_fn);
        dsBuilder.addFunction(read_prev_ref_fn);

        dsBuilder.addFunction(write_fn);

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
    void push_front(DCDS_Node &node) {
        int64_t currNodeVal = node.read_payload();
        node.write(&currNodeVal, nullptr, front.get());
        front = std::make_shared<DCDS_Node>(node);
    }

    DCDS_Node pop_back() {
        auto back_ref = back;
        back = std::shared_ptr<DCDS_Node>(reinterpret_cast<DCDS_Node*>(back->read_prev_ref()));
        return *back_ref;
    }

    std::shared_ptr<DCDS_Node> getFront() {
        return front;
    }

    std::shared_ptr<DCDS_Node> getBack() {
        return back;
    }
private:
    std::shared_ptr<DCDS_Node> front;
    std::shared_ptr<DCDS_Node> back;
};

int main() {
    DCDS_Node::initialize();

    DCDS_Node n1;
    DCDS_Node n2;
    DCDS_Node n3;

    int64_t val1 = 7;
    int64_t val2 = 3333;

    // LOG(INFO) << n1.read_payload();
    // LOG(INFO) << n1.read_next_ref();
    // LOG(INFO) << n1.read_prev_ref();

    //       payload, next_ref, prev_ref
    n1.write(&val1, &n3, &n2);

    // LOG(INFO) << n1.read_payload();
    // LOG(INFO) << n1.read_next_ref();
    // LOG(INFO) << n1.read_prev_ref();

    n2.write(&val2, &n1, &n3);  // Circular doubly linked list

    assert(n2.read_payload() == reinterpret_cast<DCDS_Node *>(n1.read_prev_ref())->read_payload());
    assert(&n2 == n1.read_prev_ref());

    // LOG(INFO) << &n2;
    // LOG(INFO) << n1.read_prev_ref();

    // LOG(INFO) << &n3;
    // LOG(INFO) << n1.read_next_ref();

    n2.write(&val2, nullptr, nullptr);
}
