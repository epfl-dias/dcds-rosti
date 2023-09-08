//
// Created by Aunn Raza on 18.04.23.
//

#ifndef DCDS_BUILDER_HPP
#define DCDS_BUILDER_HPP

#include <cassert>
#include <dcds/builder/attribute.hpp>
#include <dcds/builder/function-builder.hpp>
#include <dcds/builder/statement.hpp>
#include <dcds/builder/storage.hpp>
#include <dcds/codegen/codegen.hpp>
#include <dcds/common/types.hpp>
#include <dcds/context/DCDSContext.hpp>
#include <deque>
#include <iostream>
#include <map>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace dcds {

class Builder {
  friend class Visitor;

 public:
  explicit Builder(dcds::DCDSContext builderContext, std::string dataStructure_name)
      : context(builderContext), dataStructureName(std::move(dataStructure_name)) {}

  auto getName() { return dataStructureName; }

  auto getContext() { return context; }

  // FunctionBuilder ------- BEGIN
  static auto createFunction(const std::string& functionName) {
    return std::make_shared<FunctionBuilder>(functionName);
  }

  static auto createFunction(const std::string& functionName, dcds::valueType returnType) {
    return std::make_shared<FunctionBuilder>(functionName, returnType);
  }

  static auto createFunction(const std::string& functionName, dcds::valueType returnType,
                             std::same_as<dcds::valueType> auto... args) {
    return std::make_shared<FunctionBuilder>(functionName, returnType, args...);
  }

  static auto createFunction(const std::string& functionName, std::same_as<dcds::valueType> auto... args) {
    return std::make_shared<FunctionBuilder>(functionName, args...);
  }

  void addFunction(const std::shared_ptr<FunctionBuilder>& function) {
    // TODO: Add asserts for checking if the function is in a proper state before the codegen starts.
    assert(functions.contains(function->getName()) == false);
    functions.emplace(function->getName(), function);
  }
  auto getFunction(std::string functionName) { return *functions[functionName]; }

 private:
  std::unordered_map<std::string, std::shared_ptr<FunctionBuilder>> functions;

  // Utilities to include attributes to the data structure
 public:
  auto getAttribute(const std::string& attribute_name) { return *attributes[attribute_name]; }

  void addAttribute(const std::string& name, dcds::valueType attrType, std::variant<int64_t, void*> initVal) {
    attributes.emplace(name, std::make_shared<dcds::Attribute>(name, attrType, initVal));
  }

 private:
  // Modelling attributes as strict int64_t/void* for now.
  std::map<std::string, std::shared_ptr<Attribute>> attributes;

  // Functionality for adding temporary variables in the function.
 public:
  void addTempVar(const std::string& varName, dcds::valueType varType, std::variant<int64_t, void*> initVal,
                  std::shared_ptr<FunctionBuilder> function) {
    tempVarsInfo.emplace(varName,
                         std::tuple<dcds::valueType, std::variant<int64_t, void*>, std::shared_ptr<FunctionBuilder>>{
                             varType, initVal, function});
  }

  void addArgVar(const std::string& varName, dcds::valueType varType, std::shared_ptr<FunctionBuilder> function) {
    funcArgsInfo[function].emplace_back(std::pair<std::string, dcds::valueType>{varName, varType});
  }

 private:
  std::unordered_map<std::string,
                     std::tuple<dcds::valueType, std::variant<int64_t, void*>, std::shared_ptr<FunctionBuilder>>>
      tempVarsInfo;
  std::unordered_map<std::shared_ptr<FunctionBuilder>, std::vector<std::pair<std::string, dcds::valueType>>>
      funcArgsInfo;

  // Utilize statements in the data structure
 public:
  auto createReadStatement(dcds::Attribute sourceAttr, std::string varReadName) {
    // TODO: Add assertion for argument variable validity as well (in all statement creators).
    assert(tempVarsInfo.find(varReadName) != tempVarsInfo.end() ||
           dcds::llvmutil::findInMapOfVectors(funcArgsInfo, varReadName) != -1);
    return std::make_shared<StatementBuilder>("readStatement", sourceAttr.name, varReadName);
  }

  auto createUpdateStatement(dcds::Attribute sourceAttr, std::string varWriteName) {
    assert(tempVarsInfo.find(varWriteName) != tempVarsInfo.end() ||
           dcds::llvmutil::findInMapOfVectors(funcArgsInfo, varWriteName) != -1);
    return std::make_shared<StatementBuilder>("updateStatement", sourceAttr.name, varWriteName);
  }

  auto createTempVarAddStatement(std::string var1Name, std::string var2Name, std::string resVarName) {
    assert(tempVarsInfo.find(var1Name) != tempVarsInfo.end() ||
           dcds::llvmutil::findInMapOfVectors(funcArgsInfo, var1Name) != -1);
    assert(tempVarsInfo.find(var2Name) != tempVarsInfo.end() ||
           dcds::llvmutil::findInMapOfVectors(funcArgsInfo, var2Name) != -1);
    assert(tempVarsInfo.find(resVarName) != tempVarsInfo.end() ||
           dcds::llvmutil::findInMapOfVectors(funcArgsInfo, resVarName) != -1);

    auto statement = std::make_shared<StatementBuilder>("tempVarAddStatement", var1Name, var2Name);
    tempVarsOpResName.emplace(statement, resVarName);
    return statement;
  }

  auto createConditionStatement(dcds::ConditionBuilder& condition,
                                std::vector<std::shared_ptr<StatementBuilder>>& ifResStatements,
                                std::vector<std::shared_ptr<StatementBuilder>>& elseResStatements) {
    std::shared_ptr<StatementBuilder> conditionStatementIndicator =
        std::make_shared<StatementBuilder>("conditionStatementIndicator", "", "");
    statementToConditionMap.emplace(conditionStatementIndicator, std::make_shared<ConditionStatementBuilder>(
                                                                     condition, ifResStatements, elseResStatements));

    return conditionStatementIndicator;
  }

  auto createReturnStatement(std::string returnVarName) {
    assert(tempVarsInfo.find(returnVarName) != tempVarsInfo.end());
    return std::make_shared<StatementBuilder>("returnStatement", "", returnVarName);
  }

  auto createCallStatement2VoidPtrArgs(std::string functionName, std::string arg1, std::string arg2) {
    externalCallInfo.emplace(functionName, std::vector<std::string>{arg1, arg2});
    return std::make_shared<StatementBuilder>("callStatement", functionName, "");
  }

  void addStatement(const std::shared_ptr<StatementBuilder> statement, std::shared_ptr<FunctionBuilder> function) {
    orderedStatements.emplace(
        std::pair<std::shared_ptr<FunctionBuilder>, uint32_t>{function, ++statementOrder[function]}, statement);
  }

 private:
  std::map<std::pair<std::shared_ptr<FunctionBuilder>, uint32_t>, std::shared_ptr<StatementBuilder>> orderedStatements;
  std::unordered_map<std::shared_ptr<StatementBuilder>, std::shared_ptr<ConditionStatementBuilder>>
      statementToConditionMap;
  std::unordered_map<std::shared_ptr<StatementBuilder>, std::string> tempVarsOpResName;

  std::unordered_map<std::string, std::vector<std::string>> externalCallInfo;

  // Variable used for maintaining statement order inside functions.
  std::unordered_map<std::shared_ptr<FunctionBuilder>, int64_t> statementOrder;

 private:
  const DCDSContext context;
  const std::string dataStructureName;

 public:
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

  auto codegen() {
    std::shared_ptr<dcds::Visitor> visitor = std::make_shared<dcds::Visitor>(
        dataStructureName, functions, attributes, tempVarsInfo, funcArgsInfo, orderedStatements,
        statementToConditionMap, tempVarsOpResName, externalCallInfo);
    visitor->visit();
    visitor->runPasses();
    visitor->build();

    return visitor;
  }

  // TODO: Devise a generic method to store and provide JITed Data Structure functions from this layer itself. Ideally,
  // the user should not be dealing with dcds::Visitor directly.
};

}  // namespace dcds

#endif  // DCDS_BUILDER_HPP
