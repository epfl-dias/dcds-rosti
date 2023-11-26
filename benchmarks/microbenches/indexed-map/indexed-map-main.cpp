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

#include <dcds/dcds.hpp>
#include <random>

constexpr size_t num_columns = 1;
constexpr size_t num_records = 1_M;  // 10;
constexpr size_t num_ops_per_txn = 10;
constexpr size_t num_txn_per_thread = 1_M;
constexpr size_t num_threads = 4;

const std::string item_name = "MAPPED_ITEM";

const auto key_type = dcds::valueType::INT64;
const auto value_type = dcds::valueType::INT64;

// LRU
//

static auto generate_item(const std::shared_ptr<dcds::Builder>& builder) {
  auto item_builder = builder->createType(item_name);

  auto key_attr = item_builder->addAttribute("key_", key_type, UINT64_C(11));
  auto val_attr = item_builder->addAttribute("value_", value_type, UINT64_C(22));

  {
    // void get_value(value_ref *)
    auto get_all_fn = item_builder->createFunction("get_value", dcds::valueType::VOID);
    auto val_arg = get_all_fn->addArgument("value", value_type, true);
    get_all_fn->setAlwaysInline(true);

    auto sb = get_all_fn->getStatementBuilder();
    sb->addReadStatement(val_attr, val_arg);

    sb->addReturnVoidStatement();
  }

  {
    // void set_value(value )
    auto upd_all_fn = item_builder->createFunction("set_value", dcds::valueType::VOID);
    auto val_arg = upd_all_fn->addArgument("value", value_type);
    upd_all_fn->setAlwaysInline(true);

    auto sb = upd_all_fn->getStatementBuilder();
    sb->addUpdateStatement(val_attr, val_arg);

    sb->addReturnVoidStatement();
  }

  return item_builder;
}

static void generateLookupFunction(const std::shared_ptr<dcds::Builder>& builder) {
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

static void generateInsertFunction(const std::shared_ptr<dcds::Builder>& builder) {
  auto fn = builder->createFunction("insert", dcds::valueType::BOOL);
  auto key_arg = fn->addArgument("key", key_type);
  auto val_arg = fn->addArgument("value", value_type);

  auto sb = fn->getStatementBuilder();

  // for the given key, get the record
  auto rec_attribute = builder->getAttribute("records");
  auto rec = fn->addTempVariable("rec", dcds::valueType::RECORD_PTR);

  //??
}

// static void generateUpdateFunction(const std::shared_ptr<dcds::Builder>& builder) {
//   // void update(key, val_1, val_2, ...)
//   auto fn = builder->createFunction("update", dcds::valueType::VOID);
//   auto key_arg = fn->addArgument("key", dcds::valueType::INT64);
//
//   for (size_t i = 0; i < num_columns; i++) {
//     fn->addArgument("val_" + std::to_string(i), item_type);
//   }
//
//   auto sb = fn->getStatementBuilder();
//
//   auto rec_attribute = builder->getAttribute("records");
//   auto rec = fn->addTempVariable("rec", dcds::valueType::RECORD_PTR);
//
//   sb->addReadStatement(rec_attribute, rec, key_arg);
//
//   // update-all-one-go
//   std::vector<std::shared_ptr<dcds::expressions::Expression>> args;
//   for (size_t i = 0; i < num_columns; i++) {
//     args.push_back(fn->getArgument("val_" + std::to_string(i)));
//   }
//
//   sb->addMethodCall(builder->getRegisteredType(item_name), rec, "update_record", args);
//
//   sb->addReturnVoidStatement();
// }
//

//
// static void generateLookupNFunction(const std::shared_ptr<dcds::Builder>& builder) {
//  // getByKey
//
//  auto fn = builder->createFunction("lookup_n", dcds::valueType::VOID);
//  auto key_arg = fn->addArgument("key", dcds::valueType::INT64);
//  auto sb = fn->getStatementBuilder();
//
//  // We need a better way to read the record. maybe a void ptr where all can be read.
//  for (size_t n = 0; n < num_ops_per_txn; n++) {
//    for (size_t i = 0; i < num_columns; i++) {
//      fn->addArgument("column_" + std::to_string(n) + "_" + std::to_string(i) + "_ret", item_type, true);
//    }
//  }
//
//  // for the given key, get the record
//  auto rec_attribute = builder->getAttribute("records");
//  auto rec = fn->addTempVariable("rec", dcds::valueType::RECORD_PTR);
//
//  for (size_t n = 0; n < num_ops_per_txn; n++) {
//    // attribute, destination, key
//    sb->addReadStatement(rec_attribute, rec, key_arg);
//
//    // read-all-one-go
//    std::vector<std::shared_ptr<dcds::expressions::Expression>> args;
//    for (size_t i = 0; i < num_columns; i++) {
//      args.push_back(fn->getArgument("column_" + std::to_string(n) + "_" + std::to_string(i) + "_ret"));
//    }
//
//    // "rec" contains the ptr
//    sb->addMethodCall(builder->getRegisteredType(item_name), rec, "get_record", args);
//  }
//
//  sb->addReturnVoidStatement();
//}

int main(int argc, char** argv) {
  dcds::InitializeLog(argc, argv);
  LOG(INFO) << "INDEXED_MAP";

  dcds::ScopedAffinityManager scopedAffinity(dcds::Core{0});

  auto builder = std::make_shared<dcds::Builder>("INDEXED_MAP");
  //   builder->addHint(dcds::hints::BuilderHints::SINGLE_THREADED);
  auto item = generate_item(builder);

  // Attribute array is integer-indexed.
  // Attribute IndexedList is key-based index.

  builder->addAttributeIndexedList("records", item, "key_");

  // builder->addAttributeArray("records", item, num_records);

  generateLookupFunction(builder);
  //  generateLookupNFunction(builder);
  //  generateUpdateFunction(builder);

  builder->dump();
  //  builder->injectCC();
  LOG(INFO) << "######################";
  builder->dump();
  LOG(INFO) << "######################";

  builder->build();

  auto instance = builder->createInstance();
  instance->listAllAvailableFunctions();

  int64_t val = 0;
  auto x = instance->op("lookup", 1, &val);
  LOG(INFO) << std::any_cast<bool>(x);

  return 0;
}
