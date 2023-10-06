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

/// DCDS Frontend class
class Builder {
  friend class Visitor;

 public:
  ///
  /// \param builderContext        DCDS Context for this builder
  /// \param dataStructure_name    Name of the data structure for this builder
  explicit Builder(dcds::DCDSContext& builderContext, std::string dataStructure_name)
      : context(builderContext), dataStructureName(std::move(dataStructure_name)) {}

  ///
  /// \return Name of the data structure for this builder
  auto getName() { return dataStructureName; }

  ///
  /// \return DCDS context for this builder
  auto getContext() { return context; }

  ///
  /// \param functionName  Name of the function to be created
  /// \return              DCDS function representation
  static auto createFunction(const std::string& functionName) {
    return std::make_shared<FunctionBuilder>(functionName);
  }

  ///
  /// \param functionName  Name of the function to be created
  /// \param returnType    Return type of the function to be created
  /// \return              DCDS function representation
  static auto createFunction(const std::string& functionName, dcds::valueType returnType) {
    return std::make_shared<FunctionBuilder>(functionName, returnType);
  }

  ///
  /// \param functionName  Name of the function to be created
  /// \param returnType    Return type of the function to be created
  /// \param argsTypes     Argument types for the function to be created
  /// \return              DCDS function representation
  static auto createFunction(const std::string& functionName, dcds::valueType returnType,
                             std::same_as<dcds::valueType> auto... argsTypes) {
    return std::make_shared<FunctionBuilder>(functionName, returnType, argsTypes...);
  }

  ///
  /// \param functionName  Name of the function to be created
  /// \param argsTypes     Argument types of the function to be created
  /// \return              DCDS function representation
  static auto createFunction(const std::string& functionName, std::same_as<dcds::valueType> auto... args) {
    return std::make_shared<FunctionBuilder>(functionName, args...);
  }

  ///
  /// \param function      DCDS function representation
  void addFunction(const std::shared_ptr<FunctionBuilder>& function) {
    // TODO: Add asserts for checking if the function is in a proper state before the codegen starts.
    assert(functions.contains(function->getName()) == false);
    functions.emplace(function->getName(), function);
  }

  ///
  /// \param functionName  Name of the function to be returned
  /// \return              DCDS function representation
  auto getFunction(std::string functionName) { return *functions[functionName]; }

 private:
  /// Map function names with their representation in DCDS
  std::unordered_map<std::string, std::shared_ptr<FunctionBuilder>> functions;

  // Utilities to include attributes to the data structure
 public:
  ///
  /// \param attribute_name    Name of the attribute to be returned
  /// \return                  DCDS attributes representation
  auto getAttribute(const std::string& attribute_name) { return *attributes[attribute_name]; }

  ///
  /// \param name       Name of the attribute to be added to the data structure
  /// \param attrType   Type of the attribute to be added to the data structure
  /// \param initVal    Initial value of the attribute to be added to the data structure
  void addAttribute(const std::string& name, dcds::valueType attrType, std::variant<int64_t, void*> initVal) {
    attributes.emplace(name, std::make_shared<dcds::Attribute>(name, attrType, initVal));
  }

 private:
  // Modelling attributes as strict int64_t/void* for now.
  /// Map attribute names to their representation in DCDS
  std::map<std::string, std::shared_ptr<Attribute>> attributes;

  // Functionality for adding temporary variables in the function.
 public:
  ///
  /// \param varName    Name of the temporary variable to be added
  /// \param varType    Type of the temporary variable to be added
  /// \param initVal    Initial value of the temporary variable to be added
  /// \param function   DCDS function in which the temporary variable is to be added
  void addTempVar(const std::string& varName, dcds::valueType varType, std::variant<int64_t, void*> initVal,
                  std::shared_ptr<FunctionBuilder> function) {
    tempVarsInfo.emplace(varName,
                         std::tuple<dcds::valueType, std::variant<int64_t, void*>, std::shared_ptr<FunctionBuilder>>{
                             varType, initVal, function});
  }

  ///
  /// \param varName      Name of the argument variable to be added
  /// \param varType      Type of the argument variable to be added
  /// \param function     DCDS function in which the argument variable is to be added
  void addArgVar(const std::string& varName, dcds::valueType varType, std::shared_ptr<FunctionBuilder> function) {
    funcArgsInfo[function].emplace_back(std::pair<std::string, dcds::valueType>{varName, varType});
  }

 private:
  /// Map temporary variable names to their info
  std::unordered_map<std::string,
                     std::tuple<dcds::valueType, std::variant<int64_t, void*>, std::shared_ptr<FunctionBuilder>>>
      tempVarsInfo;
  /// Map DCDS functions to their arguments
  std::unordered_map<std::shared_ptr<FunctionBuilder>, std::vector<std::pair<std::string, dcds::valueType>>>
      funcArgsInfo;

  // Utilize statements in the data structure
 public:
  ///
  /// \param sourceAttr    Source attribute which is to be read
  /// \param varReadName   Reference variable which will store the read value
  /// \return              DCDS statement representation
  auto createReadStatement(dcds::Attribute sourceAttr, std::string varReadName) {
    // TODO: Add assertion for argument variable validity as well (in all statement creators).
    assert(tempVarsInfo.find(varReadName) != tempVarsInfo.end() ||
           dcds::llvmutil::findInMapOfVectors(funcArgsInfo, varReadName) != -1);
    return std::make_shared<StatementBuilder>(dcds::statementType::READ, sourceAttr.name, varReadName);
  }

  ///
  /// \param sourceAttr      Source attribute which is to be written
  /// \param varWriteName    Reference variable which will store the value to be written
  /// \return                DCDS statement representation
  auto createUpdateStatement(dcds::Attribute sourceAttr, std::string varWriteName) {
    assert(tempVarsInfo.find(varWriteName) != tempVarsInfo.end() ||
           dcds::llvmutil::findInMapOfVectors(funcArgsInfo, varWriteName) != -1);
    return std::make_shared<StatementBuilder>(dcds::statementType::UPDATE, sourceAttr.name, varWriteName);
  }

  ///
  /// \param var1Name        Name of the first variable to be added
  /// \param var2Name        Name of the second variable to be added
  /// \param resVarName      Name of the variable which will store the result of this addition
  /// \return                DCDS statement representation
  auto createTempVarAddStatement(std::string var1Name, std::string var2Name, std::string resVarName) {
    assert(tempVarsInfo.find(var1Name) != tempVarsInfo.end() ||
           dcds::llvmutil::findInMapOfVectors(funcArgsInfo, var1Name) != -1);
    assert(tempVarsInfo.find(var2Name) != tempVarsInfo.end() ||
           dcds::llvmutil::findInMapOfVectors(funcArgsInfo, var2Name) != -1);
    assert(tempVarsInfo.find(resVarName) != tempVarsInfo.end() ||
           dcds::llvmutil::findInMapOfVectors(funcArgsInfo, resVarName) != -1);

    auto statement = std::make_shared<StatementBuilder>(dcds::statementType::TEMP_VAR_ADD, var1Name, var2Name);
    tempVarsOpResName.emplace(statement, resVarName);
    return statement;
  }

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
  /// \param returnVarName    Name of the variable to be returned
  /// \return                 DCDS statement representation
  auto createReturnStatement(std::string returnVarName) {
    assert(tempVarsInfo.find(returnVarName) != tempVarsInfo.end());
    return std::make_shared<StatementBuilder>(dcds::statementType::YIELD, "", returnVarName);
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
  void addStatement(const std::shared_ptr<StatementBuilder> statement, std::shared_ptr<FunctionBuilder> function) {
    orderedStatements.emplace(
        std::pair<std::shared_ptr<FunctionBuilder>, uint32_t>{function, ++statementOrder[function]}, statement);
  }

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
  const DCDSContext context;
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
