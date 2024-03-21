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

#include <dcds/util/logging.hpp>

#include "parser-generated/doubly-linked-list.hpp"
#include "parser-generated/linked-list.hpp"

constexpr size_t total_items = 8_K;

constexpr size_t n_threads = 1;
constexpr size_t n_iter = total_items / n_threads;

static void test_linkedList_stack() {
  auto ll = dcds_generated::Stack();

  ll.build(true, false);
  ll.test();
}

static void test_linkedList_fifo() {
  auto ll = dcds_generated::FIFO();

  ll.build(true, false);
  ll.test();

  ll.testMT(n_threads, n_iter);
}

static void benchmark_l(dcds_generated::LinkedList& list, bool push_only = false) {
  constexpr size_t _num_iter = 5;
  constexpr size_t _ops_per_thr = 100_K;
  const std::vector<size_t> threads{1, 2, 4, 8, 12, 16, 24, 36, 48};

  for (const auto& t : threads) {
    for (size_t i = 0; i < _num_iter; i++) {
      if (push_only) {
        list.benchmark_push(t, _ops_per_thr, true);
      } else {
        list.benchmark(t, _ops_per_thr, true);
        // list.benchmark2(t, _ops_per_thr, true);
      }

      dcds::storage::TableRegistry::getInstance().clear();
    }
  }
}

static void bench_fifo() {
  auto ll_fifo = dcds_generated::FIFO();
  LOG(INFO) << ll_fifo.ds_name;
  ll_fifo.build(true, true);
  benchmark_l(ll_fifo);
}

static void bench_fifo2() {
  auto ll_fifo = dcds_generated::FIFO();
  LOG(INFO) << ll_fifo.ds_name;
  ll_fifo.build(true, true);
  benchmark_l(ll_fifo, true);
}

static void bench_stack_warmup() {
  auto ll_stack = dcds_generated::Stack();
  LOG(INFO) << ll_stack.ds_name;
  ll_stack.build(true, true);
  for (size_t i = 0; i < 4; i++) {
    ll_stack.benchmark(48, 10_K, true);
    dcds::storage::TableRegistry::getInstance().clear();
  }
}

static void bench_stack() {
  auto ll_stack = dcds_generated::Stack();
  LOG(INFO) << ll_stack.ds_name;
  ll_stack.build(true, true);
  benchmark_l(ll_stack);
}

static void bench_stack2() {
  auto ll_stack = dcds_generated::Stack();
  LOG(INFO) << ll_stack.ds_name;
  ll_stack.build(true, true);
  benchmark_l(ll_stack, true);
}

static void benchmark_dll(dcds_generated::DoublyLinkedList& list, bool bench_stack = true) {
  constexpr size_t _num_iter = 5;
  constexpr size_t _ops_per_thr = 100_K;
  const std::vector<size_t> threads{1, 2, 4, 8, 12, 16, 24, 36, 48};

  for (const auto& t : threads) {
    for (size_t i = 0; i < _num_iter; i++) {
      if (bench_stack) {
        // list.benchmark_stack(t, _ops_per_thr, true);
        // list.benchmark_stack3(t, _ops_per_thr, true);
        list.benchmark_stack2(t, _ops_per_thr, true);
      } else {
        list.benchmark_fifo(t, _ops_per_thr, true);
      }

      dcds::storage::TableRegistry::getInstance().clear();
    }
  }
}

static void dll_stack() {
  auto ll_stack = dcds_generated::DoublyLinkedList();
  LOG(INFO) << dcds_generated::DoublyLinkedList::ds_name;
  ll_stack.build(true, true);
  benchmark_dll(ll_stack, true);
}

static void dll_fifo() {
  auto ll_fifo = dcds_generated::DoublyLinkedList();
  LOG(INFO) << dcds_generated::DoublyLinkedList::ds_name;
  ll_fifo.build(true, true);
  benchmark_dll(ll_fifo, false);
}

int main(int argc, char** argv) {
  dcds::InitializeLog(argc, argv);
  LOG(INFO) << "listBasedDS";
  // test_linkedList_stack();
  // test_linkedList_fifo();

  // bench_stack_warmup();

  // dll_fifo();
  // dll_stack();

  // bench_stack();
  // bench_stack2();
  // bench_fifo();
  // bench_fifo2();

  LOG(INFO) << "------- DONE";
  return 0;
}
