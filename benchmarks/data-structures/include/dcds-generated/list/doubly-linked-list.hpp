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

#ifndef DCDS_DOUBLY_LINKED_LIST_HPP
#define DCDS_DOUBLY_LINKED_LIST_HPP

#include "dcds-generated/base.hpp"

namespace dcds::datastructures {

class DoublyLinkedList : public dcds_generated_ds {
 public:
  constexpr static auto ds_name = "DoublyLinkedList";
  constexpr static auto ds_node_name = "DLL_NODE";

 public:
  explicit DoublyLinkedList(dcds::valueType payload_type = dcds::valueType::INT64);

 public:
  void generateLinkedListNode(dcds::valueType payload_type);

  void createFunction_empty();
  void createFunction_pushFront();
  void createFunction_popFront();

  void createFunction_pushBack();
  void createFunction_popBack();

  void createFunction_extract();
  void createFunction_touch();

  void createFunction_getHeadPtr();
  void createFunction_getTailPtr();

  void createFunction_pushFrontWithReturn();  // returns the newly created record also.
};

// somewhat special for LRU, having size variable.
class DoublyLinkedList2 : public dcds_generated_ds {
 public:
  constexpr static auto ds_name = "DoublyLinkedList";
  constexpr static auto ds_node_name = "DLL_NODE";

 public:
  explicit DoublyLinkedList2(dcds::valueType payload_type = dcds::valueType::INT64);

 public:
  void generateLinkedListNode(dcds::valueType payload_type);

  void createFunction_getSize();

  void createFunction_empty();
  void createFunction_pushFront();
  // void createFunction_popFront();

  // void createFunction_pushBack();
  void createFunction_popBack();
  void createFunction_popBackWithKeyReturn();

  void createFunction_touch();

  //  void createFunction_getHeadPtr();
  //  void createFunction_getTailPtr();

  void createFunction_pushFrontWithReturn();  // returns the newly created record also.
};

// somewhat special for LRU, having size variable.
class FixedSizedDoublyLinkedList : public dcds_generated_ds {
 public:
  constexpr static auto ds_name = "DoublyLinkedList";
  constexpr static auto ds_node_name = "DLL_NODE";

 public:
  explicit FixedSizedDoublyLinkedList(dcds::valueType payload_type = dcds::valueType::INT64);

 public:
  void generateLinkedListNode(dcds::valueType payload_type);

  void createFunction_init();
  void createFunction_popBack();
  void createFunction_touch();
  void createFunction_pushFrontWithReturn();  // returns the newly created record also.
};
}  // namespace dcds::datastructures

#endif  // DCDS_DOUBLY_LINKED_LIST_HPP
