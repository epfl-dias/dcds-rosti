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

#ifndef DCDS_FUNCTION_BUILDER_HPP
#define DCDS_FUNCTION_BUILDER_HPP

#include <concepts>
#include <deque>
#include <iostream>
#include <unordered_map>
#include <utility>
#include <vector>

#include "dcds/builder/builder.hpp"
#include "dcds/common/exceptions/exception.hpp"
#include "dcds/common/types.hpp"

namespace dcds {

class Codegen;
class LLVMCodegen;

class FunctionBuilder {
  friend class Visitor;
  friend class Codegen;
  friend class LLVMCodegen;

 public:
  ///
  /// \param functionName Name of the function
  explicit FunctionBuilder(dcds::Builder *ds_builder, std::string functionName)
      : FunctionBuilder(ds_builder, std::move(functionName), dcds::valueType::VOID) {}

  ///
  /// \param functionName Name of the function
  /// \param returnType Return type of the function
  FunctionBuilder(dcds::Builder *ds_builder, std::string functionName, dcds::valueType returnType)
      : builder(ds_builder), _name(std::move(functionName)), returnValueType(returnType) {
    // TODO: function name should not start with get_/set_
  }

  auto getName() { return _name; }
  auto getReturnValueType() { return returnValueType; }

  // --------------------------------------
  // Function Arguments
  // --------------------------------------
 private:
  [[nodiscard]] auto findArgument(const std::string &arg) const {
    return std::find_if(function_arguments.begin(), function_arguments.end(),
                        [&](const std::pair<std::string, dcds::valueType> &i) { return i.first == arg; });
  }

 public:
  void addArgument(const std::string &name, dcds::valueType varType) {
    isValidVarAddition(name);
    this->function_arguments.emplace_back(name, varType);
  }
  [[nodiscard]] auto getArguments() const { return function_arguments; }
  [[nodiscard]] bool hasArguments() const { return !function_arguments.empty(); }
  [[nodiscard]] bool hasArgument(const std::string &arg) const {
    return (findArgument(arg) != std::end(function_arguments));
  }
  [[nodiscard]] auto getArgumentIndex(const std::string &arg) const {
    assert(hasArgument(arg));
    return std::distance(function_arguments.begin(), findArgument(arg));
  }

  // --------------------------------------

  // --------------------------------------
  // Temporary Variables
  // --------------------------------------

  void addTempVariable(const std::string &name, dcds::valueType varType) { addTempVariable(name, varType, {}); }
  void addTempVariable(const std::string &name, dcds::valueType varType, const std::any &initial_value) {
    isValidVarAddition(name);
    temp_variables.emplace(name, std::make_tuple(varType, initial_value));
  }

  auto getTempVariable(const std::string &name) {
    if (!(this->temp_variables.contains(name))) {
      throw dcds::exceptions::dcds_dynamic_exception("Temporary variable does not exists: " + name);
    }
    return temp_variables[name];
  }
  bool hasTempVariable(const std::string &name) { return temp_variables.contains(name); }
  bool hasSameTypeTempVariable(const std::string &name, dcds::valueType varType) {
    if (!(this->temp_variables.contains(name))) {
      throw dcds::exceptions::dcds_dynamic_exception("Temporary variable does not exists: " + name);
    }
    return temp_variables[name].first == varType;
  }
  // --------------------------------------

  auto addReadStatement(const std::shared_ptr<dcds::SimpleAttribute> &attribute, const std::string &destination) {
    // Ideally this should return a valueType or something operate-able so the user knows what is the return type?

    // NOTE: Source will always be DS-attribute, and destination will be a temporary variable always.
    if (!(this->builder->hasAttribute(attribute))) {
      throw dcds::exceptions::dcds_dynamic_exception("Attribute not registered in the data structure");
    }
    if (!temp_variables.contains(destination)) {
      throw dcds::exceptions::dcds_dynamic_exception("Function does not referenced source variable referenced: " +
                                                     destination);
    }
    auto s = std::make_shared<StatementBuilder>(dcds::statementType::READ, attribute->name, destination);
    statements.push_back(s);
    return s;
  }

  auto addUpdateStatement(const std::shared_ptr<dcds::SimpleAttribute> &attribute, const std::string &source) {
    // NOTE: Destination will always be DS-attribute, and source can be either temporary variable or function argument.

    if (!(this->builder->hasAttribute(attribute))) {
      throw dcds::exceptions::dcds_dynamic_exception("Attribute not registered in the data structure");
    }

    if (!temp_variables.contains(source) && !hasArgument(source)) {
      throw dcds::exceptions::dcds_dynamic_exception("Function does not referenced source variable referenced: " +
                                                     source);
    }

    auto sourceType = temp_variables.contains(source) ? VAR_SOURCE_TYPE::TEMPORARY_VARIABLE : FUNCTION_ARGUMENT;

    auto s = std::make_shared<UpdateStatementBuilder>(dcds::statementType::UPDATE, attribute->name, source, sourceType);
    statements.push_back(s);
    return s;
  }

  auto addReturnStatement(const std::string &name) {
    // TODO: make sure the variable name, and the return type matches!

    // The reference variable should be a temporary variable.
    // what about the case of returning a constant?

    if (!temp_variables.contains(name)) {
      throw dcds::exceptions::dcds_dynamic_exception("Function does not contain variable referenced to be returned: " +
                                                     name);
    }

    auto rs = std::make_shared<StatementBuilder>(dcds::statementType::YIELD, "", name);
    statements.push_back(rs);
    return rs;
  }

  auto addReturnVoidStatement() {
    auto rs = std::make_shared<StatementBuilder>(dcds::statementType::YIELD, "", "");
    statements.push_back(rs);
    return rs;
  }

  //  auto addInsertStatement(){
  //    // what type of insert? insert cannot happen i think, only construct can happen which will call insert.
  //  }

  void debug_print() {
    LOG(INFO) << "Function : " << this->_name;
    LOG(INFO) << "\tfunction_arguments:";
    for (const auto &arg : function_arguments) {
      LOG(INFO) << "\t\t- " << arg.first;  // << "\t" << valueTypeToString(arg.second);
    }
    LOG(INFO) << "\t# of statements: " << statements.size();
  }

  [[nodiscard]] auto isAlwaysInline() const { return _is_always_inline; }

  void setAlwaysInline() { _is_always_inline = true; }

 private:
  void isValidVarAddition(const std::string &name) {
    if (this->hasArgument(name)) {
      throw dcds::exceptions::dcds_dynamic_exception("Argument with the same name already exists: " + name);
    }

    if (this->temp_variables.contains(name)) {
      throw dcds::exceptions::dcds_dynamic_exception("Temporary variable with the name same already exists: " + name);
    }

    if ((this->builder->hasAttribute(name))) {
      throw dcds::exceptions::dcds_dynamic_exception("Data structure attribute with same name already exists: " + name);
    }
  }

 private:
  dcds::Builder *builder;  // convert to shared_ptr
  const std::string _name;

  dcds::valueType returnValueType;
  std::vector<std::pair<std::string, dcds::valueType>> function_arguments;

  std::unordered_map<std::string, std::pair<dcds::valueType, std::any>> temp_variables;

  std::deque<std::shared_ptr<StatementBuilder>> statements;

  bool _is_always_inline = false;
};

}  // namespace dcds

#endif  // DCDS_FUNCTION_BUILDER_HPP
