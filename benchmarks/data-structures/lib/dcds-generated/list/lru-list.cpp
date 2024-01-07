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

#include "dcds-generated/list/lru-list.hpp"

// TODO: move the DoublyLinkedList2::touch function here as it operates on raw-pointer, which should not be exposed to
//  end-user directly but in a composed type only.

using namespace dcds::datastructures;

const auto key_type = dcds::valueType::INT64;
const auto value_type = dcds::valueType::INT64;

LruList::LruList(size_t capacity) : dcds_generated_ds("LRU_LIST"), max_capacity(capacity) {
  // FIXME: -1 is as we are using greater-than expression, it should be greater or equal.
  max_capacity_expr =
      std::make_shared<dcds::expressions::Int64Constant>(max_capacity - 1);  // assuming size var is int64

  this->dl = new DoublyLinkedList2(key_type);
  this->builder->registerType(dl->getBuilder());

  auto dl_node =
      this->builder->getRegisteredType(DoublyLinkedList2::ds_name)->getRegisteredType(DoublyLinkedList2::ds_node_name);

  // this->builder->registerType(dl_node); // FIXME: ideally this should be automatic composed.

  // auto dl_node = dl->getBuilder()->getRegisteredType(DoublyLinkedList2::ds_node_name);

  auto valueAttribute = dl_node->addAttribute("key_", value_type, UINT64_C(0));
  dl_node->generateGetter(valueAttribute);  // get_key_
  dl_node->generateSetter(valueAttribute);  // set_key_

  dl->createFunction_pushFrontWithReturn();
  dl->createFunction_getSize();
  dl->createFunction_popBack();

  //
  //  // dl_node::payload == value
  //  builder->dump();
  //
  //  //builder->getRegisteredType(DoublyLinkedList2::ds_name)->dump();
  //  builder->getRegisteredType(DoublyLinkedList2::ds_name)->getRegisteredType(DoublyLinkedList2::ds_node_name)->dump();

  // FIXME: because its a ptr, it cannot be made at constructor time, make it composite_attribute!
  auto listAttr = builder->addAttributePtr("list", dl->getBuilder());
  // listAttr->is_compile_time_constant = true;
  listAttr->is_runtime_constant = true;
  builder->addAttributeIndexedList("map", dl_node, "key_");

  this->generateInitFunction();
  // this->generateFindFunction();
  this->generateInsertFunction();
}

dcds::JitContainer* LruList::createInstance() {
  auto* ins = builder->createInstance();
  ins->op("init");
  // LOG(INFO) << "INIT DONE";
  return ins;
}

void LruList::generateInitFunction() {
  auto rec_type = dl->getBuilder()->getRegisteredType(DoublyLinkedList2::ds_node_name);
  auto fn = builder->createFunction("init");
  auto sb = fn->getStatementBuilder();

  auto tmpList = fn->addTempVariable("tmp_list", builder->getAttribute("list")->type);
  sb->addReadStatement(builder->getAttribute("list"), tmpList);

  auto conditionalBlocks = sb->addConditionalBranch(new dcds::expressions::IsNullExpression{tmpList});

  conditionalBlocks.ifBlock->addInsertStatement(DoublyLinkedList2::ds_name, "list_init_tmp");
  conditionalBlocks.ifBlock->addUpdateStatement(builder->getAttribute("list"), "list_init_tmp");

  //  conditionalBlocks.ifBlock->addReturnVoidStatement();
  //  conditionalBlocks.elseBlock->addReturnVoidStatement();

  sb->addReturnVoidStatement();
}

/**
 * Find a value by key, and return it by update the pointer-arg.
 * Returns true if the element was found, false
 * otherwise. Updates the eviction list, making the element the
 * most-recently used.
 */
void LruList::generateFindFunction() {
  // declare bool find(key, value)
  auto rec_type = dl->getBuilder()->getRegisteredType(DoublyLinkedList2::ds_node_name);
  auto fn = builder->createFunction("find", dcds::valueType::BOOL);
  auto key_arg = fn->addArgument("key", key_type);
  auto val_arg = fn->addArgument("value", value_type, true);

  auto sb = fn->getStatementBuilder();

  //  // for the given key, get the record
  //  auto rec_attribute = builder->getAttribute("records");
  //  auto rec = fn->addTempVariable("rec", dcds::valueType::RECORD_PTR);

  // lookup in the map, if not found, return false.

  // if found, get the value from the ptr-in-map
  // update the list.

  auto map = builder->getAttribute("map");
  auto rec = fn->addTempVariable("rec", dcds::valueType::RECORD_PTR);

  // attribute, destination, key
  sb->addReadStatement(map, rec, key_arg);

  //  sb->addLogStatement("[find] rec is %llu\n", {rec});
  //  gen: if(rec != nullptr)
  auto conditionalBlocks = sb->addConditionalBranch(new dcds::expressions::IsNotNullExpression{rec});

  {
    // ifBlock :: key exists.
    auto ifBlock = conditionalBlocks.ifBlock;

    // std::vector<std::shared_ptr<dcds::expressions::Expression>> args{val_arg};
    //  "rec" contains the ptr
    //  FIX HERE.
    conditionalBlocks.ifBlock->addMethodCall(rec_type, rec, "get_payload", val_arg);

    // for the list type, do 2 things,
    //  1- first copy the value.
    //  2- call the touch() operation or move it up.

    auto tmpList = fn->addTempVariable("tmp_list", builder->getAttribute("list")->type);
    conditionalBlocks.ifBlock->addReadStatement(builder->getAttribute("list"), "tmp_list");
    // rec::dl_node

    //    void addMethodCall(const std::shared_ptr<Builder> &object_type, const std::string &reference_variable,
    //                       const std::string &function_name, std::vector<std::string> args = {}) {

    conditionalBlocks.ifBlock->addMethodCall(this->builder->getRegisteredType(DoublyLinkedList2::ds_name), tmpList,
                                             "touch", std::vector<std::shared_ptr<dcds::expressions::Expression>>{rec});

    // this->builder->getRegisteredType(DoublyLinkedList2::ds_name)

    conditionalBlocks.ifBlock->addReturnStatement(std::make_shared<dcds::expressions::BoolConstant>(true));
  }

  // elseBlock :: key does not exist.
  conditionalBlocks.elseBlock->addReturnStatement(std::make_shared<dcds::expressions::BoolConstant>(false));
}

/**
 * Insert a value into the list. Both the key and value will be copied.
 * The new element will put into the eviction list as the most-recently
 * used.
 *
 * If there was already an element in the list with the same key, it
 * will not be updated, and false will be returned. Otherwise, true will be
 * returned.
 */
void LruList::generateInsertFunction() {
  // declare bool insert(key, value)
  auto rec_type = dl->getBuilder()->getRegisteredType(DoublyLinkedList2::ds_node_name);
  auto fn = builder->createFunction("insert", dcds::valueType::BOOL);
  auto key_arg = fn->addArgument("key", key_type);
  auto val_arg = fn->addArgument("value", value_type);

  auto sb = fn->getStatementBuilder();

  auto map = builder->getAttribute("map");
  auto rec = fn->addTempVariable("rec", dcds::valueType::RECORD_PTR);

  // attribute, destination, key
  sb->addReadStatement(map, rec, key_arg);

  // gen: if(rec != nullptr)
  auto conditionalBlocks = sb->addConditionalBranch(new dcds::expressions::IsNullExpression{rec});

  {
    // ifBlock :: key does not exist.
    // ==> insert the new key/val
    auto ifBlock = conditionalBlocks.ifBlock;

    auto tmpList = fn->addTempVariable("tmp_list", builder->getAttribute("list")->type);
    conditionalBlocks.ifBlock->addReadStatement(builder->getAttribute("list"), "tmp_list");

    // get-current-size
    // if greater than capacity, pop_back

    auto sizeTmp = fn->addTempVariable(
        "current_list_size", builder->getRegisteredType(DoublyLinkedList2::ds_name)->getAttribute("size")->type);
    conditionalBlocks.ifBlock->addMethodCall(this->builder->getRegisteredType(DoublyLinkedList2::ds_name), tmpList,
                                             "get_size", sizeTmp);

    auto do_pop = conditionalBlocks.ifBlock->addConditionalBranch(
        new dcds::expressions::GreaterThanExpression{sizeTmp, max_capacity_expr});
    // void pop_back()
    // do_pop.ifBlock->addLogStatement("[do_pop] needs popping: curr-size: %llu\n", {sizeTmp});

    auto popped_key = fn->addTempVariable("popped_key", key_type);
    do_pop.ifBlock->addMethodCall(this->builder->getRegisteredType(DoublyLinkedList2::ds_name), tmpList, "pop_back",
                                  std::vector<std::shared_ptr<dcds::expressions::Expression>>{popped_key});
    // do_pop.ifBlock->addLogStatement("[do_pop] popped-key: %llu\n", {popped_key});
    //  FIXME: remove from map also!
    do_pop.ifBlock->addRemoveStatement(map, popped_key);  // --> the key would be from pop_back function!

    //    conditionalBlocks.ifBlock->addLogStatement("[insert] here1\n");

    auto new_record = fn->addTempVariable("new_record", dcds::valueType::RECORD_PTR);

    // push_front_with_return (returns record_ptr, args: key, value)
    // new_record = push_front_with_return(key, value)
    //    conditionalBlocks.ifBlock->addLogStatement("[insert] here2\n");
    conditionalBlocks.ifBlock->addMethodCall(
        this->builder->getRegisteredType(DoublyLinkedList2::ds_name), tmpList, "push_front_with_return", new_record,
        std::vector<std::shared_ptr<dcds::expressions::Expression>>{key_arg, val_arg});

    //    conditionalBlocks.ifBlock->addLogStatement("[insert] here3\n");
    // conditionalBlocks.ifBlock->addLogStatement("[insert] new_record is %llu\n", {new_record});

    // insert into list --> push_front --> we want it to return record_ptr, so that we can insert into map.

    // insert into map
    conditionalBlocks.ifBlock->addInsertStatement(map, key_arg, new_record);
    // conditionalBlocks.ifBlock->addLogStatement("[insert] here4\n");

    conditionalBlocks.ifBlock->addReturnStatement(std::make_shared<dcds::expressions::BoolConstant>(true));
  }

  // elseBlock :: key does not exist.
  conditionalBlocks.elseBlock->addReturnStatement(std::make_shared<dcds::expressions::BoolConstant>(false));
}

// LRU LIST 2

dcds::JitContainer* LruList2::createInstance() {
  auto* ins = builder->createInstance();
  ins->op("init");
  // LOG(INFO) << "INIT DONE";
  return ins;
}

LruList2::LruList2(size_t capacity) : dcds_generated_ds("LRU_LIST2"), max_capacity(capacity) {
  // FIXME: -1 is as we are using greater-than expression, it should be greater or equal.
  max_capacity_expr =
      std::make_shared<dcds::expressions::Int64Constant>(max_capacity - 1);  // assuming size var is int64

  this->dl = new FixedSizedDoublyLinkedList(key_type);
  this->builder->registerType(dl->getBuilder());

  auto dl_node = this->builder->getRegisteredType(FixedSizedDoublyLinkedList::ds_name)
                     ->getRegisteredType(FixedSizedDoublyLinkedList::ds_node_name);

  auto valueAttribute = dl_node->addAttribute("key_", value_type, UINT64_C(0));
  dl_node->generateGetter(valueAttribute);  // get_key_
  dl_node->generateSetter(valueAttribute);  // set_key_

  dl->createFunction_init();
  dl->createFunction_pushFrontWithReturn();
  dl->createFunction_touch();
  dl->createFunction_popBack();

  //
  //  // dl_node::payload == value
  //  builder->dump();
  //
  //  //builder->getRegisteredType(FixedSizedDoublyLinkedList::ds_name)->dump();
  //  builder->getRegisteredType(FixedSizedDoublyLinkedList::ds_name)->getRegisteredType(FixedSizedDoublyLinkedList::ds_node_name)->dump();

  // FIXME: because its a ptr, it cannot be made at constructor time, make it composite_attribute!
  auto listAttr = builder->addAttributePtr("list", dl->getBuilder());
  // listAttr->is_compile_time_constant = true;
  listAttr->is_runtime_constant = true;
  builder->addAttributeIndexedList("map", dl_node, "key_");

  this->generateInitFunction();
  // this->generateFindFunction();
  this->generateInsertFunction();
}

void LruList2::generateInitFunction() {
  auto rec_type = dl->getBuilder()->getRegisteredType(FixedSizedDoublyLinkedList::ds_node_name);
  auto fn = builder->createFunction("init");
  fn->setIsSingleton(true);

  auto sb = fn->getStatementBuilder();

  auto tmpList = fn->addTempVariable("tmp_list", builder->getAttribute("list")->type);
  sb->addReadStatement(builder->getAttribute("list"), tmpList);

  auto conditionalBlocks = sb->addConditionalBranch(new dcds::expressions::IsNullExpression{tmpList});

  auto list_init_tmp =
      conditionalBlocks.ifBlock->addInsertStatement(FixedSizedDoublyLinkedList::ds_name, "list_init_tmp");
  conditionalBlocks.ifBlock->addUpdateStatement(builder->getAttribute("list"), "list_init_tmp");

  // Call list::init to initialize fixed-size list.
  conditionalBlocks.ifBlock->addMethodCall(
      dl->getBuilder(), list_init_tmp, "init",
      std::vector<std::shared_ptr<dcds::expressions::Expression>>{
          std::make_shared<dcds::expressions::Int64Constant>(static_cast<int64_t>(this->max_capacity))});

  sb->addReturnVoidStatement();
}

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
void LruList2::generateInsertFunction() {
  // declare bool insert(key, value)
  // auto rec_type = dl->getBuilder()->getRegisteredType(FixedSizedDoublyLinkedList::ds_node_name);
  auto fn = builder->createFunction("insert", dcds::valueType::BOOL);
  auto key_arg = fn->addArgument("key", key_type);
  auto val_arg = fn->addArgument("value", value_type);

  auto sb = fn->getStatementBuilder();

  auto map = builder->getAttribute("map");
  auto rec = fn->addTempVariable("rec", dcds::valueType::RECORD_PTR);

  auto tmpList = fn->addTempVariable("tmp_list", builder->getAttribute("list")->type);
  sb->addReadStatement(builder->getAttribute("list"), "tmp_list");

  // attribute, destination, key
  sb->addReadStatement(map, rec, key_arg);

  // gen: if(rec != nullptr)
  auto conditionalBlocks = sb->addConditionalBranch(new dcds::expressions::IsNullExpression{rec});

  {
    // ifBlock :: key does not exist.
    // ==> insert the new key/val

    // void pop_back()
    // do_pop.ifBlock->addLogStatement("[do_pop] needs popping: curr-size: %llu\n", {sizeTmp});

    auto popped_key = fn->addTempVariable("popped_key", key_type);
    conditionalBlocks.ifBlock->addMethodCall(this->builder->getRegisteredType(FixedSizedDoublyLinkedList::ds_name),
                                             tmpList, "pop_back",
                                             std::vector<std::shared_ptr<dcds::expressions::Expression>>{popped_key});
    // conditionalBlocks.ifBlock->addLogStatement("[do_pop] popped-key: %llu\n", {popped_key});
    conditionalBlocks.ifBlock->addRemoveStatement(map, popped_key);  // --> the key would be from pop_back function!

    auto new_record = fn->addTempVariable("new_record", dcds::valueType::RECORD_PTR);

    // push_front_with_return (returns record_ptr, args: key, value)
    // new_record = push_front_with_return(key, value)
    conditionalBlocks.ifBlock->addMethodCall(
        this->builder->getRegisteredType(FixedSizedDoublyLinkedList::ds_name), tmpList, "push_front_with_return",
        new_record, std::vector<std::shared_ptr<dcds::expressions::Expression>>{key_arg, val_arg});

    //    conditionalBlocks.ifBlock->addLogStatement("[insert] here3\n");
    // conditionalBlocks.ifBlock->addLogStatement("[insert] new_record is %llu\n", {new_record});

    // insert into list --> push_front --> we want it to return record_ptr, so that we can insert into map.

    // insert into map
    conditionalBlocks.ifBlock->addInsertStatement(map, key_arg, new_record);
    // conditionalBlocks.ifBlock->addLogStatement("[insert] here4\n");

    conditionalBlocks.ifBlock->addReturnStatement(std::make_shared<dcds::expressions::BoolConstant>(true));
  }

  {
    // elseBlock :: key does not exist.
    conditionalBlocks.elseBlock->addMethodCall(this->builder->getRegisteredType(FixedSizedDoublyLinkedList::ds_name),
                                               tmpList, "touch",
                                               std::vector<std::shared_ptr<dcds::expressions::Expression>>{rec});
    conditionalBlocks.elseBlock->addReturnStatement(std::make_shared<dcds::expressions::BoolConstant>(false));
  }
}