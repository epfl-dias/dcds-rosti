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

#ifndef DCDS_LRU_LIST_HPP
#define DCDS_LRU_LIST_HPP

#include "dcds-generated/base.hpp"
#include "dcds-generated/list/doubly-linked-list.hpp"

namespace dcds::datastructures {

class LruList : public dcds_generated_ds {
 public:
  explicit LruList(size_t _capacity = 10_K);
  dcds::JitContainer* createInstance() override;

 private:
  /**
   * Find a value by key, and return it by update the pointer-arg.
   * Returns true if the element was found, false
   * otherwise. Updates the eviction list, making the element the
   * most-recently used.
   *
   * signature: declare bool find(key, value*)
   */

  void generateFindFunction();

  /**
   * Insert a value into the list. Both the key and value will be copied.
   * The new element will put into the eviction list as the most-recently
   * used.
   *
   * If there was already an element in the list with the same key, it
   * will not be updated, and false will be returned. Otherwise, true will be
   * returned.
   *
   * * signature: declare bool insert(key, value)
   */
  void generateInsertFunction();

  void generateInitFunction();

 protected:
  DoublyLinkedList2* dl;
  size_t max_capacity;
  std::shared_ptr<dcds::expressions::Int64Constant> max_capacity_expr;
};

class LruList2 : public dcds_generated_ds {
 public:
  explicit LruList2(size_t _capacity = 10_K);
  dcds::JitContainer* createInstance() override;

 private:
  /**
   * Insert a value into the list. Both the key and value will be copied.
   * The new element will put into the eviction list as the most-recently
   * used.
   *
   * If there was already an element in the list with the same key, it will
   * not update the value but updates the eviction list, making the element the
   * most-recently used, and false will be returned. Otherwise, true will be
   * returned.
   *
   * * signature: declare bool insert(key, value)
   */
  void generateInsertFunction();

  void generateInitFunction();

 protected:
  FixedSizedDoublyLinkedList* dl;
  size_t max_capacity;
  std::shared_ptr<dcds::expressions::Int64Constant> max_capacity_expr;
};

}  // namespace dcds::datastructures
#endif  // DCDS_LRU_LIST_HPP
