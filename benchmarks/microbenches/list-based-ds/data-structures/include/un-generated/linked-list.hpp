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

#ifndef DCDS_UNGENERATED_LINKED_LIST_HPP
#define DCDS_UNGENERATED_LINKED_LIST_HPP

#include <iostream>
#include <queue>

namespace dcds_hardcoded {

class LinkedList {
 public:
  virtual void push(const int64_t& value) = 0;
  virtual bool pop(int64_t& value) = 0;
  virtual ~LinkedList() = default;

  void testMT(size_t n_threads, size_t iterations = 100);
};

class ConcurrentQueue : public LinkedList {
  const std::string name = "ConcurrentQueue";

 public:
  void push(const int64_t& value) override {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(value);
  }

  bool pop(int64_t& value) override {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) {
      return false;
    }

    value = queue_.front();
    queue_.pop();
    return true;
  }

 private:
  std::queue<int64_t> queue_;
  std::mutex mutex_;
};

class Node {
 public:
  int64_t data;
  Node* next;

  explicit Node(const int64_t& value) : data(value), next(nullptr) {}
};

class ConcurrentLinkedList : public LinkedList {
 public:
  const std::string name = "ConcurrentLinkedList";
  ConcurrentLinkedList() : head(nullptr), tail(nullptr) {}

  void push(const int64_t& value) override {
    std::lock_guard<std::mutex> lock(mutex_);
    Node* newNode = new Node(value);

    if (!head) {
      head = tail = newNode;
    } else {
      tail->next = newNode;
      tail = newNode;
    }
  }

  bool pop(int64_t& value) override {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!head) {
      return false;
    }

    value = head->data;
    Node* oldHead = head;
    head = head->next;
    delete oldHead;

    if (!head) {
      tail = nullptr;
    }

    return true;
  }

 private:
  Node* head;
  Node* tail;
  std::mutex mutex_;
};

}  // namespace dcds_hardcoded

#endif  // DCDS_UNGENERATED_LINKED_LIST_HPP
