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

class FunctionBuilder : remove_copy {
  friend class Codegen;
  friend class LLVMCodegen;
  friend class Builder;

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
    auto s = std::make_shared<Statement>(dcds::statementType::READ, attribute->name, destination);
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

    auto s = std::make_shared<UpdateStatement>(attribute->name, source, sourceType);
    statements.push_back(s);
    return s;
  }

  auto addReturnStatement(const std::string &name) {
    // The reference variable should be a temporary variable.
    // what about the case of returning a constant?

    if (!temp_variables.contains(name)) {
      throw dcds::exceptions::dcds_dynamic_exception("Function does not contain variable referenced to be returned: " +
                                                     name);
    }

    if (temp_variables[name].first != returnValueType) {
      throw dcds::exceptions::dcds_invalid_type_exception(
          "Return type mismatch between return variable and declared return type");
    }

    auto rs = std::make_shared<Statement>(dcds::statementType::YIELD, "", name);
    statements.push_back(rs);
    return rs;
  }

  auto addReturnVoidStatement() {
    auto rs = std::make_shared<Statement>(dcds::statementType::YIELD, "", "");
    statements.push_back(rs);
    return rs;
  }

  auto addInsertStatement(const std::string &registered_type_name, const std::string &variable_name) {
    assert(builder->hasRegisteredType(registered_type_name));

    this->addTempVariable(variable_name, dcds::valueType::RECORD_PTR);
    // this would call the constructor of the registered_type,
    // and then assigns the returned record_reference to the variable name,

    auto rs = std::make_shared<InsertStatement>(registered_type_name, variable_name);
    statements.push_back(rs);
    return rs;
  }

  // (reference_variable of record_ptr, function_name, ...args)
  // (reference_variable of record_ptr, function_name, returnValueDestinationTemporaryVariable,
  // ...args{ofTypeTemporaryVariable/FuncARg})
  auto addMethodCall(const std::shared_ptr<Builder> &object_type, const std::string &reference_variable,
                     const std::string &function_name, const std::string &return_destination_variable,
                     std::same_as<std::string> auto... args) {
    // FIXME: a lot of type-check, validity checks, etc.

    valueType referenceVarType;
    if (hasTempVariable(reference_variable)) {
      auto var = getTempVariable(reference_variable);
      referenceVarType = var.first;
    } else if (hasArgument(reference_variable)) {
      referenceVarType = findArgument(reference_variable)->second;
    } else {
      throw dcds::exceptions::dcds_dynamic_exception("Reference variable does not exists in the scope: " +
                                                     reference_variable);
    }

    if (referenceVarType != RECORD_PTR) {
      throw dcds::exceptions::dcds_invalid_type_exception("Reference variable is not of type RECORD_PTR ");
    }
    // FIXME: how to check if the record_ptr is of the correct type! it can be done for temporary variables, but what
    //  about function arguments.
    // FIXME: Additionally, we need to know so that we can check if the appropriate function exists in that type!

    std::vector<std::pair<std::string, dcds::VAR_SOURCE_TYPE>> arg_variables;
    for (const std::string &arg_name : {args...}) {
      if (!arg_name.empty()) {
        if (temp_variables.contains(arg_name)) {
          arg_variables.emplace_back(arg_name, VAR_SOURCE_TYPE::TEMPORARY_VARIABLE);
        } else if (hasArgument(arg_name)) {
          arg_variables.emplace_back(arg_name, VAR_SOURCE_TYPE::FUNCTION_ARGUMENT);
        } else {
          throw dcds::exceptions::dcds_dynamic_exception(
              "Method call argument is neither a temporary variable, nor a function argument: " + arg_name);
        }
      }
    }

    auto rs = std::make_shared<MethodCallStatement>(object_type, reference_variable, function_name,
                                                    return_destination_variable, std::move(arg_variables));
    statements.push_back(rs);
    return rs;
  }

  auto addMethodCall(const std::shared_ptr<Builder> &object_type, const std::string &reference_variable,
                     const std::string &function_name, const std::string &return_destination_variable) {
    return addMethodCall(object_type, reference_variable, function_name, return_destination_variable, std::string{""});
  }

  auto addLogStatement(const std::string &log_string) {
    auto rs = std::make_shared<LogStringStatement>(log_string);
    statements.push_back(rs);
    return rs;
  }

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
  std::shared_ptr<FunctionBuilder> cloneShared(dcds::Builder *ds_builder) {
    auto f = std::make_shared<FunctionBuilder>(ds_builder, this->_name, this->returnValueType);
    f->function_arguments = this->function_arguments;
    f->temp_variables = this->temp_variables;
    for (auto &s : this->statements) {
      f->statements.emplace_back(std::make_shared<Statement>(*s));
    }
    return f;
  }

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
  const dcds::valueType returnValueType;

  std::vector<std::pair<std::string, dcds::valueType>> function_arguments;
  std::unordered_map<std::string, std::pair<dcds::valueType, std::any>> temp_variables;
  std::deque<std::shared_ptr<Statement>> statements;

  bool _is_always_inline = false;
};

}  // namespace dcds

#endif  // DCDS_FUNCTION_BUILDER_HPP
