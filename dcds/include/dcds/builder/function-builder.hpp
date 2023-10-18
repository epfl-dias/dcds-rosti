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

#include <algorithm>
#include <concepts>
#include <deque>
#include <iostream>
#include <unordered_map>
#include <utility>
#include <vector>

#include "dcds/builder/builder.hpp"
#include "dcds/builder/expressions/unary-expressions.hpp"
#include "dcds/common/exceptions/exception.hpp"
#include "dcds/common/types.hpp"

namespace dcds {

class Codegen;
class LLVMCodegen;

class StatementBuilder;

class FunctionBuilder : remove_copy {
  friend class Codegen;
  friend class LLVMCodegen;
  friend class Builder;
  friend class StatementBuilder;

 public:
  explicit FunctionBuilder(dcds::Builder *ds_builder, std::string functionName);
  FunctionBuilder(dcds::Builder *ds_builder, std::string functionName, dcds::valueType returnType);

  std::shared_ptr<StatementBuilder> getStatementBuilder() { return entryPoint; }

  auto getName() { return _name; }
  auto getReturnValueType() { return returnValueType; }

  [[nodiscard]] auto isAlwaysInline() const { return _is_always_inline; }
  void setAlwaysInline(bool val) { _is_always_inline = val; }

  // --------------------------------------
  // Function Arguments
  // --------------------------------------
 private:
  [[nodiscard]] auto findArgument(const std::string &arg) const {
    return std::find_if(
        function_args.begin(), function_args.end(),
        [&](const std::shared_ptr<expressions::FunctionArgumentExpression> &i) { return i->getName() == arg; });
  }

 public:
  auto addArgument(const std::string &name, dcds::valueType varType) {
    isValidVarAddition(name);
    auto arg = std::make_shared<expressions::FunctionArgumentExpression>(name, varType);
    this->function_args.push_back(arg);
    return arg;
  }
  [[nodiscard]] auto getArguments() const { return function_args; }
  [[nodiscard]] bool hasArguments() const { return !function_args.empty(); }
  [[nodiscard]] bool hasArgument(const std::string &arg) const {
    return (findArgument(arg) != std::end(function_args));
  }
  [[nodiscard]] auto getArgumentIndex(const std::string &arg) const {
    assert(hasArgument(arg));
    return std::distance(function_args.begin(), findArgument(arg));
  }
  std::shared_ptr<expressions::FunctionArgumentExpression> getArgument(const std::string &arg) {
    return findArgument(arg).operator*();
  }

  // --------------------------------------

  // --------------------------------------
  // Temporary Variables
  // --------------------------------------

  auto addTempVariable(const std::string &name, dcds::valueType varType, const std::any &initial_value) {
    isValidVarAddition(name);
    auto var = std::make_shared<expressions::TemporaryVariableExpression>(name, varType, initial_value);
    temp_variables.emplace(name, var);
    return var;
  }
  auto addTempVariable(const std::string &name, dcds::valueType varType) { return addTempVariable(name, varType, {}); }

 private:
  auto addTempVariable(const std::string &name, std::shared_ptr<Builder> objectType) {
    isValidVarAddition(name);
    assert(hasRegisteredType(objectType->getName()));

    auto var = std::make_shared<expressions::TemporaryVariableExpression>(name, objectType);
    temp_variables.emplace(name, var);
    return var;
  }

 public:
  //  void addTempVariable(const std::string &name, std::shared_ptr<SimpleAttribute> &attributeTypeRef) {
  //    addTempVariable(name, varType, {});
  //  }

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
    return temp_variables[name]->getType() == varType;
  }
  // --------------------------------------

  void dump() {
    LOG(INFO) << "Function : " << this->_name;
    LOG(INFO) << "\tfunction_arguments:";
    for (const auto &arg : function_args) {
      LOG(INFO) << "\t\t- " << arg->getName();  // << "\t" << valueTypeToString(arg.second);
    }
    // todo: number of statements.
    //    LOG(INFO) << "\t# of statements: " << statements.size();
  }

 private:
  std::shared_ptr<FunctionBuilder> cloneShared(dcds::Builder *ds_builder) {
    auto f = std::make_shared<FunctionBuilder>(ds_builder, this->_name, this->returnValueType);

    for (const auto &fa : this->function_args) {
      f->function_args.emplace_back(std::make_shared<expressions::FunctionArgumentExpression>(*(fa)));
    }
    for (const auto &tv : this->temp_variables) {
      f->temp_variables.emplace(tv.first, std::make_shared<expressions::TemporaryVariableExpression>(*(tv.second)));
    }
    assert(false && " todo clone statements");
    //    for (auto &s : this->statements) {
    //      f->statements.emplace_back(std::make_shared<Statement>(*s));
    //    }
    return f;
  }

 private:
  inline void isValidVarAddition(const std::string &name) {
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
  inline bool hasAttribute(const std::shared_ptr<dcds::SimpleAttribute> &attribute) {
    return this->builder->hasAttribute(attribute);
  }
  inline bool hasRegisteredType(const std::string &registered_type_name) {
    return builder->hasRegisteredType(registered_type_name);
  }
  inline auto getRegisteredType(const std::string &registered_type_name) {
    return builder->getRegisteredType(registered_type_name);
  }

 private:
  dcds::Builder *builder;  // convert to shared_ptr
  const std::string _name;
  const dcds::valueType returnValueType;

  std::vector<std::shared_ptr<expressions::FunctionArgumentExpression>> function_args;
  std::map<std::string, std::shared_ptr<expressions::TemporaryVariableExpression>> temp_variables;
  std::shared_ptr<StatementBuilder> entryPoint;
  //  std::deque<std::shared_ptr<Statement>> statements;
  bool _is_always_inline = false;
};

}  // namespace dcds

#endif  // DCDS_FUNCTION_BUILDER_HPP
