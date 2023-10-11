//
// Created by Aunn Raza on 18.04.23.
//

#ifndef DCDS_BUILDER_HPP
#define DCDS_BUILDER_HPP

#include <cassert>
#include <dcds/builder/attribute.hpp>
#include <dcds/builder/statement.hpp>
#include <dcds/builder/storage.hpp>
// #include <dcds/codegen/codegen.hpp>

#include <deque>
#include <iostream>
#include <map>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "dcds/common/exceptions/exception.hpp"
#include "dcds/common/types.hpp"
#include "dcds/context/DCDSContext.hpp"

namespace dcds {

// forward declaration
class FunctionBuilder;
class CodegenV2;
class LLVMCodegen;
class JitContainer;

/// DCDS Frontend class
class Builder {
  friend class Visitor;

  friend class CodegenV2;
  friend class LLVMCodegen;

 public:
  ///
  /// \param builderContext        DCDS Context for this builder
  /// \param name    Name of the data structure for this builder
  explicit Builder(std::shared_ptr<dcds::DCDSContext>& builderContext, std::string name)
      : context(builderContext), dataStructureName(std::move(name)) {}

  ///
  /// \param name    Name of the data structure for this builder
  explicit Builder(std::string name) : context({}), dataStructureName(std::move(name)) {}

  ///
  /// \return Name of the data structure for this builder
  auto getName() { return dataStructureName; }

  ///
  /// \return DCDS context for this builder
  auto getContext() { return context; }

  ///
  /// \param functionName  Name of the function to be created
  /// \return              DCDS function representation
  std::shared_ptr<FunctionBuilder> createFunction(const std::string& functionName);

  ///
  /// \param functionName  Name of the function to be created
  /// \param returnType    Return type of the function to be created
  /// \return              DCDS function representation
  std::shared_ptr<FunctionBuilder> createFunction(const std::string& functionName, dcds::valueType returnType);

  //  void createCallStatement(tmpHead, "set_next", newNode)

  ///
  /// \param functionName  Name of the function to be created
  /// \param returnType    Return type of the function to be created
  /// \param argsTypes     Argument types for the function to be created
  /// \return              DCDS function representation
  //  static auto createFunction(const std::string& functionName, dcds::valueType returnType,
  //                             std::same_as<dcds::valueType> auto... argsTypes) {
  //    return std::make_shared<FunctionBuilder>(functionName, returnType, argsTypes...);
  //  }

  ///
  /// \param functionName  Name of the function to be created
  /// \param argsTypes     Argument types of the function to be created
  /// \return              DCDS function representation
  //  static auto createFunction(const std::string& functionName, std::same_as<dcds::valueType> auto... args) {
  //    return std::make_shared<FunctionBuilder>(functionName, args...);
  //  }

  ///
  /// \param function      DCDS function representation
  //  void addFunction(const std::shared_ptr<FunctionBuilder>& function) {
  //    // TODO: Add asserts for checking if the function is in a proper state before the codegen starts.
  //    assert(functions.contains(function->getName()) == false);
  //    functions.emplace(function->getName(), function);
  //  }

  ///
  /// \param functionName  Name of the function to be returned
  /// \return              DCDS function representation
  //  auto getFunction(std::string functionName) { return *functions[functionName]; }

 private:
  /// Map function names with their representation in DCDS
  std::unordered_map<std::string, std::shared_ptr<FunctionBuilder>> functions;

  // Utilities to include attributes to the data structure
 public:
  ///
  /// \param attribute_name    Name of the attribute to be returned
  /// \return                  DCDS attributes representation
  auto getAttribute(const std::string& attribute_name) { return attributes[attribute_name]; }
  auto getAttributeIndex(const std::string& attribute_name) {
    assert(attributes.contains(attribute_name) && "how come index for un-registered attribute");
    return std::distance(std::begin(attributes), attributes.find(attribute_name));
  }

  ///
  /// \param name       Name of the attribute to be added to the data structure
  /// \param attrType   Type of the attribute to be added to the data structure
  auto addAttribute(const std::string& name, dcds::valueType attrType, const std::any& default_value) {
    auto pt = std::make_shared<dcds::SimpleAttribute>(name, attrType, default_value);
    attributes.emplace(name, pt);
    return pt;
  }

  bool hasAttribute(const std::string& name) { return attributes.contains(name); }

  bool hasAttribute(const std::shared_ptr<dcds::SimpleAttribute>& attribute) {
    return this->hasAttribute(attribute->name);
  }

  void generateGetter(const std::string& attribute_name);
  void generateSetter(const std::string& attribute_name);

  void generateGetter(std::shared_ptr<dcds::SimpleAttribute>& attribute);
  void generateSetter(std::shared_ptr<dcds::SimpleAttribute>& attribute);

  // Composability: Types

  std::shared_ptr<Builder> createType(const std::string& name) {
    if (registered_subtypes.contains(name)) {
      throw dcds::exceptions::dcds_dynamic_exception("Type name already exists");
    }
    auto nt = std::make_shared<Builder>(context, name);
    registered_subtypes.emplace(name, nt);
    return nt;
  }

  std::shared_ptr<Builder> registerType(std::shared_ptr<Builder> other) {
    if (registered_subtypes.contains(other->getName())) {
      throw dcds::exceptions::dcds_dynamic_exception("Type name already exists");
    }

    auto nt = std::make_shared<Builder>(*other);
    nt->context = this->context;
    registered_subtypes.emplace(nt->getName(), nt);
    return other;
  }

  auto getType(const std::string& name) {
    if (!(registered_subtypes.contains(name))) {
      throw dcds::exceptions::dcds_dynamic_exception("Unknown/Unregistered type: " + name);
    }
    return registered_subtypes[name];
  }

 private:
  // Modelling attributes as strict int64_t/void* for now.
  /// Map attribute names to their representation in DCDS
  std::map<std::string, std::shared_ptr<SimpleAttribute>> attributes;

  std::map<std::string, std::shared_ptr<Builder>> registered_subtypes;

 public:
  ///
  /// \param condition            DCDS condition for comparison
  /// \param ifResStatements      Statements in if block
  /// \param elseResStatements    Statements in else block
  /// \return                     DCDS statement representation
  auto createConditionStatement(dcds::ConditionBuilder& condition,
                                std::vector<std::shared_ptr<StatementBuilder>>& ifResStatements,
                                std::vector<std::shared_ptr<StatementBuilder>>& elseResStatements) {
    std::shared_ptr<StatementBuilder> conditionStatementIndicator =
        std::make_shared<StatementBuilder>(dcds::statementType::CONDITIONAL_STATEMENT, "", "");
    statementToConditionMap.emplace(conditionStatementIndicator, std::make_shared<ConditionStatementBuilder>(
                                                                     condition, ifResStatements, elseResStatements));

    return conditionStatementIndicator;
  }

  ///
  /// \param functionName    Name of the external function to whom the call must be created
  /// \param arg1            First argument of the external function
  /// \param arg2            Second argument of the external function
  /// \return                DCDS statement representation
  auto createCallStatement2VoidPtrArgs(std::string functionName, std::string arg1, std::string arg2) {
    externalCallInfo.emplace(functionName, std::vector<std::string>{arg1, arg2});
    return std::make_shared<StatementBuilder>(dcds::statementType::CALL, functionName, "");
  }

  ///
  /// \param statement       DCDS statement which is to be added
  /// \param function        DCDS function to whom the statement must be added
  //  void addStatement(const std::shared_ptr<StatementBuilder> statement, std::shared_ptr<FunctionBuilder> function) {
  //    orderedStatements.emplace(
  //        std::pair<std::shared_ptr<FunctionBuilder>, uint32_t>{function, ++statementOrder[function]}, statement);
  //  }

 private:
  /// Map functions to their corresponding statements
  std::map<std::pair<std::shared_ptr<FunctionBuilder>, uint32_t>, std::shared_ptr<StatementBuilder>> orderedStatements;
  /// Map aliased condition statement to actual condition statement and assosciated blocks
  std::unordered_map<std::shared_ptr<StatementBuilder>, std::shared_ptr<ConditionStatementBuilder>>
      statementToConditionMap;
  /// Map statements to assosciated temporary variables which are holding results
  std::unordered_map<std::shared_ptr<StatementBuilder>, std::string> tempVarsOpResName;

  /// Map external function name to temporary variable names which are to be supplied as arguments
  std::unordered_map<std::string, std::vector<std::string>> externalCallInfo;

  /// Container used for maintaining statement order inside functions.
  std::unordered_map<std::shared_ptr<FunctionBuilder>, int64_t> statementOrder;

 private:  /// DCDS context for the builder
  std::shared_ptr<DCDSContext> context;
  /// Name of the data structure for this builder
  const std::string dataStructureName;

 public:
  ///
  /// \return Return storage object's pointer specific to the current instance of the data structure
  auto initializeStorage() {
    std::shared_ptr<dcds::StorageLayer> storageObject =
        std::make_shared<dcds::StorageLayer>(dataStructureName + "_ns", this->attributes);

    // Initialize data structure attribute with default value. Can this be done from inside the storage layer
    // constructor?
    uint64_t indexVar = 0;
    for (auto attribute : this->attributes) {
      auto txnPtr = storageObject->beginTxn();
      if (std::holds_alternative<int64_t>(attribute.second->initVal))
        storageObject->write(&std::get<int64_t>(attribute.second->initVal), indexVar, txnPtr);
      else if (std::holds_alternative<void*>(attribute.second->initVal))
        storageObject->write(&std::get<void*>(attribute.second->initVal), indexVar, txnPtr);
      storageObject->commitTxn(txnPtr);

      ++indexVar;
    }

    return storageObject;
  }

  ///
  /// \return Visitor object used during the codegen process
  auto codegen() {
    //    std::shared_ptr<dcds::Visitor> visitor = std::make_shared<dcds::Visitor>(
    //        dataStructureName, functions, attributes, tempVarsInfo, funcArgsInfo, orderedStatements,
    //        statementToConditionMap, tempVarsOpResName, externalCallInfo);
    //    visitor->visit();
    //    visitor->runPasses();
    //    visitor->build();

    return 0;
  }

  // CODEGEN
 public:
  void build();
  JitContainer* createInstance();

 private:
  std::shared_ptr<CodegenV2> codegen_engine;

 private:
  bool is_jit_generated = false;
};

}  // namespace dcds

#endif  // DCDS_BUILDER_HPP
