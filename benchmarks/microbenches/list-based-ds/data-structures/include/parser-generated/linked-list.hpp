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

#ifndef DCDS_LINKED_LIST_HPP
#define DCDS_LINKED_LIST_HPP

#include "dcds/dcds.hpp"

namespace dcds_generated {

class LinkedList {
 public:
  explicit LinkedList(std::string _ds_name = "LinkedList", std::string _ds_node_name = "LL_NODE");
  virtual ~LinkedList() = default;

  void build();
  void optimize();
  virtual void test() = 0;

 private:
  void generateLinkedListNode();

 protected:
  void createFunction_empty();
  virtual void createFunction_push() = 0;
  virtual void createFunction_pop() = 0;

 protected:
  std::shared_ptr<dcds::Builder> builder;

 public:
  const std::string ds_name;
  const std::string ds_node_name;
};

class Stack : public LinkedList {
 public:
  explicit Stack();

  void test() final;

 protected:
  void createFunction_push() final;
  void createFunction_pop() final;
};

class FIFO : public LinkedList {
 public:
  explicit FIFO();
  void test() final;

  void testMT(size_t n_threads, size_t iterations = 100);

 protected:
  void createFunction_push() final;
  void createFunction_pop() final;
};

}  // namespace dcds_generated

#endif  // DCDS_LINKED_LIST_HPP
