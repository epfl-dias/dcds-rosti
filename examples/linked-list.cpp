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
// Created by prathamesh on 17/8/23.
//

#include "dcds/builder/builder.hpp"
#include "dcds/context/DCDSContext.hpp"

class DCDSNode {
 public:
  DCDSNode() {
    dcds::DCDSContext context(false, false);
    dcds::Builder dsBuilder(context, "LinkedList");

    dsBuilder.addAttribute("payload", dcds::INTEGER, 0);
    dsBuilder.addAttribute("next_reference", dcds::RECORD_PTR, nullptr);

    storageObject = dsBuilder.initializeStorage();
    storageObjectPtr = reinterpret_cast<void *>(storageObject.get());
  }

  static void initialize() {
    dcds::DCDSContext context(false, false);
    dcds::Builder dsBuilder(context, "LinkedList");

    dsBuilder.addAttribute("payload", dcds::INTEGER, 0);
    dsBuilder.addAttribute("next_reference", dcds::RECORD_PTR, nullptr);

    auto read_fn = dsBuilder.createFunction("read_function", dcds::valueType::INTEGER);
    auto read_fn2 = dsBuilder.createFunction("read_function2", dcds::valueType::RECORD_PTR);

    auto write_fn = dsBuilder.createFunction("write_function", dcds::valueType::VOID, dcds::valueType::INTEGER,
                                             dcds::valueType::RECORD_PTR);

    dsBuilder.addTempVar("ll_read_variable1", dcds::valueType::INTEGER, 0, read_fn);
    dsBuilder.addTempVar("ll_read_variable2", dcds::valueType::RECORD_PTR, nullptr, read_fn2);

    dsBuilder.addArgVar("ll_write_variable1", dcds::valueType::INTEGER, write_fn);
    dsBuilder.addArgVar("ll_write_variable2", dcds::valueType::RECORD_PTR, write_fn);

    auto read_statement = dsBuilder.createReadStatement(dsBuilder.getAttribute("payload"), "ll_read_variable1");
    auto read_statement2 = dsBuilder.createReadStatement(dsBuilder.getAttribute("next_reference"), "ll_read_variable2");

    auto write_statement = dsBuilder.createUpdateStatement(dsBuilder.getAttribute("payload"), "ll_write_variable1");
    auto write_statement2 =
        dsBuilder.createUpdateStatement(dsBuilder.getAttribute("next_reference"), "ll_write_variable2");

    auto return_statement = dsBuilder.createReturnStatement("ll_read_variable1");
    auto return_statement2 = dsBuilder.createReturnStatement("ll_read_variable2");

    dsBuilder.addStatement(read_statement, read_fn);
    dsBuilder.addStatement(return_statement, read_fn);

    dsBuilder.addStatement(read_statement2, read_fn2);
    dsBuilder.addStatement(return_statement2, read_fn2);

    dsBuilder.addStatement(write_statement, write_fn);
    dsBuilder.addStatement(write_statement2, write_fn);

    dsBuilder.addFunction(read_fn);
    dsBuilder.addFunction(read_fn2);

    dsBuilder.addFunction(write_fn);

    visitor = dsBuilder.codegen();

    built_read_fn = visitor->getBuiltFunctionInt64ReturnType("read_function");
    built_read_fn2 = visitor->getBuiltFunctionVoidPtrReturnType("read_function2");

    built_write_fn = visitor->getBuiltFunctionVoidReturnTypeIntArg1VoidPtr1("write_function");
  }

  auto read() { return built_read_fn(storageObjectPtr); }
  auto read2() { return built_read_fn2(storageObjectPtr); }

  auto write(int64_t *payload, void *next_reference) {
    built_write_fn(storageObjectPtr, payload, next_reference);
  }

 private:
  static void (*built_write_fn)(void *, int64_t *, void *);
  static void *(*built_read_fn2)(void *);
  static int64_t (*built_read_fn)(void *);
  static std::shared_ptr<dcds::Visitor> visitor;
  std::shared_ptr<dcds::StorageLayer> storageObject;
  void *storageObjectPtr;
};

void (*DCDSNode::built_write_fn)(void *, int64_t *, void *);
void *(*DCDSNode::built_read_fn2)(void *);
int64_t (*DCDSNode::built_read_fn)(void *);
std::shared_ptr<dcds::Visitor> DCDSNode::visitor;

int main() {
  DCDSNode::initialize();

  DCDSNode n1;
  DCDSNode n2;

  int64_t val1 = 7;
  int64_t val2 = 44;

  // LOG(INFO) << n1.read();
  // LOG(INFO) << n1.read2();
  n1.write(&val1, &n2);
  // LOG(INFO) << n1.read();
  // LOG(INFO) << n1.read2();
  // LOG(INFO) << &n2;

  n2.write(&val2, nullptr);

  // LOG(INFO) << n2.read();
  // LOG(INFO) << reinterpret_cast<DCDSNode *>(n1.read2())->read();

  assert(n2.read() == reinterpret_cast<DCDSNode *>(n1.read2())->read());

//  n2.write(&val, &val);
//  // LOG(INFO) << n2.read();
//
//  n1.write(&val, &n2);

//  auto n3 = reinterpret_cast<DCDSNode *>(n1.read2());
  //  // LOG(INFO) << n3->read();
}
