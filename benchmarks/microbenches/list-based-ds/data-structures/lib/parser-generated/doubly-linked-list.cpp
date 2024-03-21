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

#include "parser-generated/doubly-linked-list.hpp"

#include <dcds/util/thread-runner.hpp>
#include <dcds/util/timing.hpp>
#include <random>
#include <utility>

using namespace dcds_generated;

// CHECK with an any cast wrapper
static void CHECK_TRUE(std::any v) { CHECK(std::any_cast<bool>(v)); }
static void CHECK_FALSE(std::any v) { CHECK_EQ(std::any_cast<bool>(v), false); }

DoublyLinkedList::DoublyLinkedList() {
  this->builder = std::make_shared<dcds::Builder>(ds_name);
  this->generateLinkedListNode();
  auto nodeType = builder->getRegisteredType(ds_node_name);

  builder->addAttributePtr("head", nodeType);
  builder->addAttributePtr("tail", nodeType);

  this->createFunction_empty();
  this->createFunction_pushFront();
  this->createFunction_popFront();
  this->createFunction_pushBack();
  this->createFunction_popBack();
}

void DoublyLinkedList::build(bool inject_cc, bool optimize) {
  if (optimize) {
    LOG(INFO) << "DoublyLinkedList::optimize() -- start";
    dcds::BuilderOptPasses buildOptimizer(builder);
    buildOptimizer.runAll();
    LOG(INFO) << "DoublyLinkedList::optimize() -- end";
  }

  if (inject_cc) {
    builder->injectCC();
  }

  LOG(INFO) << "DoublyLinkedList::build() -- start";
  builder->build();

  LOG(INFO) << "DoublyLinkedList::build() -- end";
  builder->dump();

  LOG(INFO) << "DoublyLinkedList::build() -- FN end";
}

void DoublyLinkedList::generateLinkedListNode() {
  if (builder->hasRegisteredType(ds_node_name)) {
    return;
  }

  auto nodeBuilder = builder->createType(ds_node_name);
  auto payloadAttribute = nodeBuilder->addAttribute("payload", dcds::valueType::INT64, UINT64_C(0));
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
  // stmtBuilder->addLogStatement("[push_back] new node: %llu\n", {fn->getTempVariable("tmp_node")});

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

static void printThroughput(size_t runtime_ms, size_t n_threads, size_t num_op_per_thread,
                            const std::string& prefix = "") {
  auto runtime_s = ((static_cast<double>(runtime_ms)) / 1000);
  auto throughput = ((num_op_per_thread * n_threads) / runtime_s);
  auto avg_per_thread = (throughput / n_threads) / 1000000;
  LOG(INFO) << "Threads: " << n_threads << prefix
            << ": Throughput: " << ((num_op_per_thread * n_threads) / runtime_s) / 1000000 << " MTPS"
            << " | avg-per-thread: " << avg_per_thread << " MTPS"
            << " | total_time: " << runtime_ms << "ms";
}

size_t DoublyLinkedList::benchmark_fifo(size_t n_threads, size_t num_op_per_thread, bool print_res) {
  assert(builder->hasFunction("push_front"));
  assert(builder->hasFunction("pop_back"));
  auto instance = builder->createInstance();
  auto thr = dcds::ThreadRunner(n_threads);
  auto runtime_ms = thr(
      [num_op_per_thread](const uint64_t _tid, dcds::JitContainer* _instance) {
        std::mt19937 gen;
        std::uniform_int_distribution<> distrib(0, 1);
        uint64_t val;

        for (size_t i = 0; i < num_op_per_thread; i++) {
          if (distrib(gen) == 0) {
            // push
            _instance->op("push_front", i);
          } else {
            // pop
            _instance->op("pop_back", &val);
          }
        }
      },
      instance);

  dcds::storage::TableRegistry::getInstance().clear();

  if (print_res)
    printThroughput(runtime_ms, n_threads, num_op_per_thread,
                    " DLL::FIFO::" + std::string{dcds_generated::DoublyLinkedList::ds_name});
  return runtime_ms;
}
size_t DoublyLinkedList::benchmark_stack(size_t n_threads, size_t num_op_per_thread, bool print_res) {
  assert(builder->hasFunction("push_front"));
  assert(builder->hasFunction("pop_front"));
  auto instance = builder->createInstance();
  auto thr = dcds::ThreadRunner(n_threads);
  auto runtime_ms = thr(
      [num_op_per_thread](const uint64_t _tid, dcds::JitContainer* _instance) {
        std::mt19937 gen;
        std::uniform_int_distribution<> distrib(0, 1);
        uint64_t val;

        for (size_t i = 0; i < num_op_per_thread; i++) {
          if (distrib(gen) == 0) {
            // push
            _instance->op("push_front", i);
          } else {
            // pop
            _instance->op("pop_front", &val);
          }
        }
      },
      instance);

  dcds::storage::TableRegistry::getInstance().clear();

  if (print_res)
    printThroughput(runtime_ms, n_threads, num_op_per_thread,
                    " DLL::STACK::" + std::string{dcds_generated::DoublyLinkedList::ds_name});
  return runtime_ms;
}

size_t DoublyLinkedList::benchmark_stack2(size_t n_threads, size_t num_op_per_thread, bool print_res) {
  assert(builder->hasFunction("push_back"));
  assert(builder->hasFunction("pop_back"));
  auto instance = builder->createInstance();
  auto thr = dcds::ThreadRunner(n_threads);
  auto runtime_ms = thr(
      [num_op_per_thread](const uint64_t _tid, dcds::JitContainer* _instance) {
        std::mt19937 gen;
        std::uniform_int_distribution<> distrib(0, 1);
        uint64_t val;

        for (size_t i = 0; i < num_op_per_thread; i++) {
          if (distrib(gen) == 0) {
            // push
            _instance->op("push_back", i);
          } else {
            // pop
            _instance->op("pop_back", &val);
          }
        }
      },
      instance);

  dcds::storage::TableRegistry::getInstance().clear();

  if (print_res)
    printThroughput(runtime_ms, n_threads, num_op_per_thread,
                    " DLL::STACK2::" + std::string{dcds_generated::DoublyLinkedList::ds_name});
  return runtime_ms;
}

size_t DoublyLinkedList::benchmark_stack3(size_t n_threads, size_t num_op_per_thread, bool print_res) {
  assert(builder->hasFunction("push_back"));
  assert(builder->hasFunction("pop_back"));
  auto instance = builder->createInstance();
  auto thr = dcds::ThreadRunner(n_threads);
  auto runtime_ms = thr(
      [num_op_per_thread](const uint64_t _tid, dcds::JitContainer* _instance) {
        std::mt19937 gen;
        std::uniform_int_distribution<> distrib(1, 5);
        uint64_t val;

        for (size_t i = 0; i < num_op_per_thread; i++) {
          if (distrib(gen) > 2) {
            // push
            _instance->op("push_back", i);
          } else {
            // pop
            _instance->op("pop_back", &val);
          }
        }
      },
      instance);

  dcds::storage::TableRegistry::getInstance().clear();

  if (print_res)
    printThroughput(runtime_ms, n_threads, num_op_per_thread,
                    " DLL::STACK3::" + std::string{dcds_generated::DoublyLinkedList::ds_name});
  return runtime_ms;
}
