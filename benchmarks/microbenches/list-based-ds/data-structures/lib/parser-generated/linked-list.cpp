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

#include "parser-generated/linked-list.hpp"

#include <dcds/util/thread-runner.hpp>
#include <dcds/util/timing.hpp>
#include <random>
#include <utility>

using namespace dcds_generated;

// CHECK with an any cast wrapper
static void CHECK_TRUE(std::any v) { CHECK(std::any_cast<bool>(v)); }
static void CHECK_FALSE(std::any v) { CHECK_EQ(std::any_cast<bool>(v), false); }

LinkedList::LinkedList(std::string _ds_name, std::string _ds_node_name)
    : ds_name(std::move(_ds_name)), ds_node_name(std::move(_ds_node_name)) {
  this->builder = std::make_shared<dcds::Builder>(ds_name);
  this->generateLinkedListNode();
  auto nodeType = builder->getRegisteredType(ds_node_name);

  builder->addAttributePtr("head", nodeType);

  this->createFunction_empty();
}

void LinkedList::build(bool inject_cc, bool optimize) {
  if (optimize) {
    LOG(INFO) << "LinkedList::optimize() -- start";
    dcds::BuilderOptPasses buildOptimizer(builder);
    buildOptimizer.runAll();
    LOG(INFO) << "LinkedList::optimize() -- end";
  }

  if (inject_cc) {
    builder->injectCC();
  }

  LOG(INFO) << "LinkedList::build() -- start";
  builder->build();

  //  LOG(INFO) << "LinkedList::build() -- end";
  //  builder->dump();

  LOG(INFO) << "LinkedList::build() -- FN end";
}

void LinkedList::generateLinkedListNode() {
  if (builder->hasRegisteredType(ds_node_name)) {
    return;
  }

  auto nodeBuilder = builder->createType(ds_node_name);
  auto payloadAttribute = nodeBuilder->addAttribute("payload", dcds::valueType::INT64, UINT64_C(0));
  auto nextAttribute = nodeBuilder->addAttribute("next", dcds::valueType::RECORD_PTR, nullptr);
  nodeBuilder->generateGetter(payloadAttribute);  // get_payload
  nodeBuilder->generateSetter(payloadAttribute);  // set_payload

  nodeBuilder->generateGetter(nextAttribute);  // get_next
  nodeBuilder->generateSetter(nextAttribute);  // set_next
}

void LinkedList::createFunction_empty() {
  // declare bool empty()
  auto fn = builder->createFunction("empty", dcds::valueType::BOOL);

  auto stmtBuilder = fn->getStatementBuilder();

  fn->addTempVariable("tmp_head", builder->getAttribute("head")->type);
  stmtBuilder->addReadStatement(builder->getAttribute("head"), "tmp_head");

  auto conditionalBlocks =
      stmtBuilder->addConditionalBranch(new dcds::expressions::IsNullExpression{fn->getTempVariable("tmp_head")});

  conditionalBlocks.ifBlock->addReturnStatement(std::make_shared<dcds::expressions::BoolConstant>(true));
  conditionalBlocks.elseBlock->addReturnStatement(std::make_shared<dcds::expressions::BoolConstant>(false));
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

size_t LinkedList::benchmark(size_t n_threads, size_t num_op_per_thread, bool print_res) {
  assert(builder->hasFunction("push"));
  assert(builder->hasFunction("pop"));
  auto instance = builder->createInstance();
  auto thr = dcds::ThreadRunner(n_threads);
  auto runtime_ms = thr(
      [num_op_per_thread](const uint64_t _tid, dcds::JitContainer* _instance) {
        constexpr size_t seed = 42;
        std::mt19937 gen;
        std::uniform_int_distribution<> distrib(0, 1);
        uint64_t val;

        for (size_t i = 0; i < num_op_per_thread; i++) {
          if (distrib(gen) == 0) {
            // push
            _instance->op("push", i);
          } else {
            // pop
            _instance->op("pop", &val);
          }
        }
      },
      instance);

  dcds::storage::TableRegistry::getInstance().clear();

  if (print_res) printThroughput(runtime_ms, n_threads, num_op_per_thread, " DCDS-LIST::" + this->ds_name);
  return runtime_ms;
}

size_t LinkedList::benchmark2(size_t n_threads, size_t num_op_per_thread, bool print_res) {
  assert(builder->hasFunction("push"));
  assert(builder->hasFunction("pop"));
  auto instance = builder->createInstance();
  auto thr = dcds::ThreadRunner(n_threads);
  auto runtime_ms = thr(
      [num_op_per_thread](const uint64_t _tid, dcds::JitContainer* _instance) {
        constexpr size_t seed = 42;
        std::mt19937 gen;
        std::uniform_int_distribution<> distrib(1, 5);
        uint64_t val;

        for (size_t i = 0; i < num_op_per_thread; i++) {
          if (distrib(gen) > 2) {
            // push
            _instance->op("push", i);
          } else {
            // pop
            _instance->op("pop", &val);
          }
        }
      },
      instance);

  dcds::storage::TableRegistry::getInstance().clear();

  if (print_res) printThroughput(runtime_ms, n_threads, num_op_per_thread, " LL2::" + this->ds_name);
  return runtime_ms;
}

size_t LinkedList::benchmark_push(size_t n_threads, size_t num_op_per_thread, bool print_res) {
  assert(builder->hasFunction("push"));
  assert(builder->hasFunction("pop"));
  auto instance = builder->createInstance();
  auto thr = dcds::ThreadRunner(n_threads);
  auto runtime_ms = thr(
      [num_op_per_thread](const uint64_t _tid, dcds::JitContainer* _instance) {
        for (size_t i = 0; i < num_op_per_thread; i++) {
          _instance->op("push", i);
        }
      },
      instance);

  dcds::storage::TableRegistry::getInstance().clear();

  if (print_res) printThroughput(runtime_ms, n_threads, num_op_per_thread, " bench_push::" + this->ds_name);
  return runtime_ms;
}

Stack::Stack() : LinkedList("LL_Stack") {
  this->createFunction_pop();
  this->createFunction_push();
}
void Stack::createFunction_push() {
  // FIXME: Can argument to a method call be a constant value or constant expression?
  // FIXME: micro-opt: we dont need a txn starting until reading head but that wouldn't be the case.
  // TODO: Add tail-checks, and then write code, and try dead/write-only attribute.
  // TODO: add condition, if tail attribute exists, then update tail if necessary.

  /*
   * void push(uint64_t value){
   *     auto newNode = node(val)
   *     auto tmp = head;
   *     if(tmp != nullptr)
   *        tmp.next = newNode
   *     head = newNode
   *     return;
   * */

  // declare void push(uint64_t value)
  auto fn = builder->createFunction("push");
  auto nodeType = builder->getRegisteredType(ds_node_name);
  fn->addArgument("push_value", dcds::valueType::INT64);

  auto stmtBuilder = fn->getStatementBuilder();

  // gen: auto newNode = node(val)
  stmtBuilder->addInsertStatement(nodeType, "tmp_node");
  stmtBuilder->addMethodCall(nodeType, "tmp_node", "set_payload", "", {"push_value"});

  // gen: auto tmp = head;
  fn->addTempVariable("tmp_head", builder->getAttribute("head")->type);
  stmtBuilder->addReadStatement(builder->getAttribute("head"), "tmp_head");

  // gen: if(tmp != nullptr)
  auto conditionalBlocks =
      stmtBuilder->addConditionalBranch(new dcds::expressions::IsNotNullExpression{fn->getTempVariable("tmp_head")});

  // gen: tmp.next = newNode
  conditionalBlocks.ifBlock->addMethodCall(nodeType, "tmp_node", "set_next", "", {"tmp_head"});

  // gen: head = newNode
  stmtBuilder->addUpdateStatement(builder->getAttribute("head"), "tmp_node");

  // gen: return;
  stmtBuilder->addReturnVoidStatement();
}
void Stack::createFunction_pop() {
  // declare bool pop(uint64_t *val)
  auto fn = builder->createFunction("pop", dcds::valueType::BOOL);
  fn->addArgument("pop_value", dcds::valueType::INT64, true);

  auto stmtBuilder = fn->getStatementBuilder();

  fn->addTempVariable("tmp_head", builder->getAttribute("head")->type);
  stmtBuilder->addReadStatement(builder->getAttribute("head"), "tmp_head");

  // gen: if(tmp != nullptr)
  auto conditionalBlocks =
      stmtBuilder->addConditionalBranch(new dcds::expressions::IsNotNullExpression{fn->getTempVariable("tmp_head")});

  // ifBlock
  {
    // save the payload into the pop value.
    conditionalBlocks.ifBlock->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_head", "get_payload",
                                             "pop_value");

    // get next of the current_head:tmp
    fn->addTempVariable("tmp_next", dcds::valueType::RECORD_PTR);
    conditionalBlocks.ifBlock->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_head", "get_next",
                                             "tmp_next");

    // Update the head.
    conditionalBlocks.ifBlock->addUpdateStatement(builder->getAttribute("head"), "tmp_next");

    // TODO: delete the tmp (dangling record now)
    // stmtBuilder->addDeleteStatement("tmp");

    conditionalBlocks.ifBlock->addReturnStatement(std::make_shared<dcds::expressions::BoolConstant>(true));
  }

  // elseBlock
  {
    // head is null, return false -> no pop.
    conditionalBlocks.elseBlock->addReturnStatement(std::make_shared<dcds::expressions::BoolConstant>(false));
  }
}
void Stack::test() {
  LOG(INFO) << "Stack::test() -- start";

  LOG(INFO) << "generateLinkedList -- create-instance-before";
  auto instance = builder->createInstance();
  LOG(INFO) << "generateLinkedList -- create-instance-after";

  instance->listAllAvailableFunctions();

  CHECK_TRUE(instance->op("empty"));
  instance->op("push", 11);
  CHECK_FALSE(instance->op("empty"));
  instance->op("push", 22);
  instance->op("push", 33);
  instance->op("push", 44);
  CHECK_FALSE(instance->op("empty"));

  // pop
  uint64_t val = 0;
  CHECK_TRUE(instance->op("pop", &val));
  CHECK_EQ(val, 44);
  CHECK_TRUE(instance->op("pop", &val));
  CHECK_EQ(val, 33);
  CHECK_TRUE(instance->op("pop", &val));
  CHECK_EQ(val, 22);
  CHECK_TRUE(instance->op("pop", &val));
  CHECK_EQ(val, 11);
  CHECK_FALSE(instance->op("pop", &val));
  CHECK_TRUE(instance->op("empty"));

  LOG(INFO) << "Stack::test() -- end";
}

FIFO::FIFO() : LinkedList("LL_FIFO") {
  builder->addAttributePtr("tail", builder->getRegisteredType(ds_node_name));

  this->createFunction_push();
  this->createFunction_pop();
}

void FIFO::createFunction_push() {
  /*

     if(!head){
      tail=node;
     } else {
      head.next = node;
     }

     head = node;
     return;
   * */

  // declare void push(uint64_t value)
  auto fn = builder->createFunction("push");
  auto nodeType = builder->getRegisteredType(ds_node_name);
  fn->addArgument("push_value", dcds::valueType::INT64);

  auto stmtBuilder = fn->getStatementBuilder();

  // gen: auto newNode = node(val)
  stmtBuilder->addInsertStatement(nodeType, "tmp_node");
  stmtBuilder->addMethodCall(nodeType, "tmp_node", "set_payload", "", {"push_value"});

  // gen: auto tmp = head;
  fn->addTempVariable("tmp_head", builder->getAttribute("head")->type);
  stmtBuilder->addReadStatement(builder->getAttribute("head"), "tmp_head");

  // gen: if(tmp == nullptr)
  auto conditionalBlocks =
      stmtBuilder->addConditionalBranch(new dcds::expressions::IsNullExpression{fn->getTempVariable("tmp_head")});

  conditionalBlocks.ifBlock->addUpdateStatement(builder->getAttribute("tail"), "tmp_node");

  conditionalBlocks.elseBlock->addMethodCall(nodeType, "tmp_head", "set_next", "", {"tmp_node"});

  // gen: head = newNode
  stmtBuilder->addUpdateStatement(builder->getAttribute("head"), "tmp_node");

  // gen: return;
  stmtBuilder->addReturnVoidStatement();
}
void FIFO::createFunction_pop() {
  // TODO: pop from tail
  /*

    if(tail == nullptr){
      return false;
    } else {
      tmp = tail;
      tail = tail.next;
      if(tail.next == nullptr){
        // meaning head was pointing to the same?
        head = nullptr;
      }
      {tmp.payload} => save to referenceArgument
      return true;
    }


   * */

  // declare bool pop(uint64_t *val)
  auto fn = builder->createFunction("pop", dcds::valueType::BOOL);
  fn->addArgument("pop_value", dcds::valueType::INT64, true);

  auto stmtBuilder = fn->getStatementBuilder();

  fn->addTempVariable("tmp_tail", builder->getAttribute("tail")->type);
  stmtBuilder->addReadStatement(builder->getAttribute("tail"), "tmp_tail");

  // gen: if(tail == nullptr)
  auto conditionalBlocks =
      stmtBuilder->addConditionalBranch(new dcds::expressions::IsNullExpression{fn->getTempVariable("tmp_tail")});

  // ifBlock
  { conditionalBlocks.ifBlock->addReturnStatement(std::make_shared<dcds::expressions::BoolConstant>(false)); }

  // elseBlock
  {
    conditionalBlocks.elseBlock->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_tail", "get_payload",
                                               "pop_value");

    auto tmpTailNext = fn->addTempVariable("tmp_tail_next", dcds::valueType::RECORD_PTR);
    // tail = tail.next
    conditionalBlocks.elseBlock->addMethodCall(builder->getRegisteredType(ds_node_name), "tmp_tail", "get_next",
                                               "tmp_tail_next");
    conditionalBlocks.elseBlock->addUpdateStatement(builder->getAttribute("tail"), "tmp_tail_next");

    // if tail.next isNull
    //    head=null
    auto cond2 =
        conditionalBlocks.elseBlock->addConditionalBranch(new dcds::expressions::IsNullExpression{tmpTailNext});
    cond2.ifBlock->addUpdateStatement(builder->getAttribute("head"), tmpTailNext);

    // return true;
    conditionalBlocks.elseBlock->addReturnStatement(std::make_shared<dcds::expressions::BoolConstant>(true));
  }
}

void FIFO::test() {
  LOG(INFO) << "FIFO::test() -- start";

  LOG(INFO) << "generateLinkedList -- create-instance-before";
  auto instance = builder->createInstance();
  LOG(INFO) << "generateLinkedList -- create-instance-after";

  instance->listAllAvailableFunctions();

  // push
  CHECK_TRUE(instance->op("empty"));
  instance->op("push", 11);
  CHECK_FALSE(instance->op("empty"));
  instance->op("push", 22);
  instance->op("push", 33);
  instance->op("push", 44);
  CHECK_FALSE(instance->op("empty"));

  // pop
  uint64_t val = 0;
  CHECK_TRUE(instance->op("pop", &val));
  CHECK_EQ(val, 11);
  CHECK_TRUE(instance->op("pop", &val));
  CHECK_EQ(val, 22);
  CHECK_TRUE(instance->op("pop", &val));
  CHECK_EQ(val, 33);
  CHECK_TRUE(instance->op("pop", &val));
  CHECK_EQ(val, 44);
  CHECK_FALSE(instance->op("pop", &val));
  CHECK_TRUE(instance->op("empty"));

  LOG(INFO) << "FIFO::test() -- end";
}

void FIFO::testMT(size_t n_threads, size_t iterations) {
  LOG(INFO) << "FIFO::testMT() -- start";
  auto instance = builder->createInstance();
  instance->listAllAvailableFunctions();
  instance->op("empty");  // warmup
  instance->op("push", 1);

  std::vector<std::thread> runners;
  std::barrier<void (*)()> sync_point(n_threads, []() {});

  std::atomic<size_t> overall_sum = 0;
  size_t expected_sum = n_threads * iterations;

  std::vector<std::chrono::milliseconds> times;

  for (size_t x = 0; x < n_threads; x++) {
    runners.emplace_back([&, x]() {
      size_t local_sum = 0;
      uint64_t val = 0;

      sync_point.arrive_and_wait();
      // we need to time the following block
      {
        // time_block t("Trad_clust_p1:");
        time_block t{[&](auto tms) {
          //          LOG(INFO) << '\t' << std::this_thread::get_id() << ": "<< tms.count() << "ms";
          times.emplace_back(tms);
        }};

        for (size_t i = 0; i < iterations; i++) {
          instance->op("push", 1);
        }

        for (size_t i = 0; i < iterations; i++) {
          instance->op("pop", &val);
          local_sum += val;
        }
      }

      sync_point.arrive_and_wait();  // not required i guess.

      overall_sum.fetch_add(local_sum);
    });
  }

  for (auto& th : runners) {
    th.join();
  }
  size_t total = 0;
  for (auto& t : times) {
    total += t.count();
  }
  LOG(INFO) << "Total time: " << total << "ms";

  LOG(INFO) << "Expected sum: " << expected_sum << " | calc_sum: " << overall_sum;
}