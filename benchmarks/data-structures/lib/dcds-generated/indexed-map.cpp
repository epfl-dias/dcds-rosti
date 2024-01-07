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

#include "dcds-generated/indexed-map.hpp"

using namespace dcds::datastructures;

const auto key_type = dcds::valueType::INT64;
const auto value_type = dcds::valueType::INT64;
const std::string item_name = "MAPPED_ITEM";

IndexedMap::IndexedMap() : dcds_generated_ds("INDEXED_MAP") {
  auto item = generate_item();
  builder->addAttributeIndexedList("records", item, "key_");

  this->generateLookupFunction();
  //  this->generateInsertFunction();
  //  this->generateUpdateFunction();
  //  generateLookupNFunction(builder);
  //  generateUpdateFunction(builder);
}

std::shared_ptr<dcds::Builder> IndexedMap::generate_item() {
  auto item_builder = builder->createType(item_name);

  auto key_attr = item_builder->addAttribute("key_", key_type, UINT64_C(11));
  auto val_attr = item_builder->addAttribute("value_", value_type, UINT64_C(22));

  {
    // void get_value(value_ref *)
    auto get_val_fn = item_builder->createFunction("get_value", dcds::valueType::VOID);
    auto val_arg = get_val_fn->addArgument("value", value_type, true);
    get_val_fn->setAlwaysInline(true);

    auto sb = get_val_fn->getStatementBuilder();
    sb->addReadStatement(val_attr, val_arg);

    sb->addReturnVoidStatement();
  }

  {
    // void set_value(value )
    auto set_val_fn = item_builder->createFunction("set_value", dcds::valueType::VOID);
    auto val_arg = set_val_fn->addArgument("value", value_type);
    set_val_fn->setAlwaysInline(true);

    auto sb = set_val_fn->getStatementBuilder();
    sb->addUpdateStatement(val_attr, val_arg);

    sb->addReturnVoidStatement();
  }

  {
    // void set(key, value )
    auto set_fn = item_builder->createFunction("set", dcds::valueType::VOID);
    auto key_arg = set_fn->addArgument("key", key_type);
    auto val_arg = set_fn->addArgument("value", value_type);
    set_fn->setAlwaysInline(true);

    auto sb = set_fn->getStatementBuilder();
    sb->addUpdateStatement(key_attr, key_arg);
    sb->addUpdateStatement(val_attr, val_arg);
    sb->addReturnVoidStatement();
  }

  return item_builder;
}

void IndexedMap::generateLookupFunction() {
  // getByKey

  auto fn = builder->createFunction("lookup", dcds::valueType::BOOL);
  auto key_arg = fn->addArgument("key", key_type);
  auto val_arg = fn->addArgument(std::string{"val"} + "_ret", value_type, true);

  auto sb = fn->getStatementBuilder();

  // for the given key, get the record
  auto rec_attribute = builder->getAttribute("records");
  auto rec = fn->addTempVariable("rec", dcds::valueType::RECORD_PTR);

  // TODO: bound checking

  // attribute, destination, key
  sb->addReadStatement(rec_attribute, rec, key_arg);

  // gen: if(rec != nullptr)
  auto conditionalBlocks = sb->addConditionalBranch(new dcds::expressions::IsNotNullExpression{rec});

  // ifBlock

  std::vector<std::shared_ptr<dcds::expressions::Expression>> args{val_arg};
  // "rec" contains the ptr
  conditionalBlocks.ifBlock->addMethodCall(builder->getRegisteredType(item_name), rec, "get_value", args);
  conditionalBlocks.ifBlock->addReturnStatement(std::make_shared<dcds::expressions::BoolConstant>(true));

  // elseBlock
  conditionalBlocks.elseBlock->addReturnStatement(std::make_shared<dcds::expressions::BoolConstant>(false));
}
void IndexedMap::generateInsertFunction() {
  // bool insert(key, value)

  /**
   * Insert a value into the IndexedMap. Both the key and value will be copied.
   * If there was already an element in the map with the same key, it
   * will not be updated, and false will be returned. Otherwise, true will be
   * returned.
   */

  auto fn = builder->createFunction("insert", dcds::valueType::BOOL);
  auto key_arg = fn->addArgument("key", key_type);
  auto val_arg = fn->addArgument("value", value_type);

  auto sb = fn->getStatementBuilder();

  // for the given key, get the record
  auto rec_attribute = builder->getAttribute("records");
  auto rec = fn->addTempVariable("rec", dcds::valueType::RECORD_PTR);

  sb->addReadStatement(rec_attribute, rec, key_arg);

  // gen: if(rec != nullptr)
  auto conditionalBlocks = sb->addConditionalBranch(new dcds::expressions::IsNullExpression{rec});

  // ifBlock ::  key does not exist
  {
    auto ifBlock = conditionalBlocks.ifBlock;

    //    std::vector<std::shared_ptr<dcds::expressions::Expression>> args{val_arg};
    //    // "rec" contains the ptr
    //    conditionalBlocks.ifBlock->addMethodCall(builder->getRegisteredType(item_name), rec, "get_value", args);
    //    conditionalBlocks.ifBlock->addReturnStatement(std::make_shared<dcds::expressions::BoolConstant>(true));

    // create the value-node.
    auto tmpVal = sb->addInsertStatement(builder->getRegisteredType(item_name), "tmp_value");
    std::vector<std::shared_ptr<dcds::expressions::Expression>> args{key_arg, val_arg};
    conditionalBlocks.ifBlock->addMethodCall(builder->getRegisteredType(item_name), tmpVal, "set", args);

    // add it to the index
    // FIXME: do we need to store the key in the value-part?
    sb->addUpdateStatement(rec_attribute, key_arg, tmpVal);

    conditionalBlocks.elseBlock->addReturnStatement(std::make_shared<dcds::expressions::BoolConstant>(true));
  }

  // elseBlock ::  key exist
  conditionalBlocks.elseBlock->addReturnStatement(std::make_shared<dcds::expressions::BoolConstant>(false));

  //??
  assert(false);
}

void upsert() {}

void IndexedMap::generateUpdateFunction() {
  // bool update(key, value)

  assert(false);
}
