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
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"

namespace dcds {

class CodegenV2;
class LLVMCodegen;

/// Class representing a function in DCDS
class FunctionBuilder {
  friend class Visitor;
  friend class CodegenV2;
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

  ///
  /// \param name      Name of the argument variable to be added
  /// \param varType      Type of the argument variable to be added
  void addArgument(const std::string &name, dcds::valueType varType) {
    // TODO: duplicate argument does not exists.
    this->function_arguments.emplace_back(name, varType);
  }

  auto getArguments() { return function_arguments; }
  bool getHasArgumentsStatus() { return !function_arguments.empty(); }
  bool hasArgument(std::string arg) {
    return (std::find_if(function_arguments.begin(), function_arguments.end(),
                         [&](std::pair<std::string, dcds::valueType> i) { return i.first == arg; }) !=
            std::end(function_arguments));
  }

  // addTempVariable

  //  template<typename K>
  //  void addTempVariable(const std::string &name, dcds::valueType varType, K initialValue){
  //
  //  }

  void addTempVariable(const std::string &name, dcds::valueType varType) {
    if ((this->builder->hasAttribute(name))) {
      throw dcds::exceptions::dcds_dynamic_exception("Variable name already used by data structure attribute: " + name);
    }

    if (this->temp_variables.contains(name)) {
      throw dcds::exceptions::dcds_dynamic_exception("Variable name already used by another temporary variable: " +
                                                     name);
    }
    temp_variables.emplace(name, std::make_tuple(varType, nullptr));
  }

  // FIXME: have some sort of generic value container then we can put it here.
  //  void addTempVariable(const std::string &name, dcds::valueType varType, std::variant<int64_t, void *> initVal) {
  //    // FIXME: value type, instead of std::variants
  //    //  optional init value
  //
  //    // FIXME: make sure this temp variable name does not conflict with any other attribute name,
  //    //  or temporary variable or function arguments!
  //
  //    if (!(this->builder->hasAttribute(name))) {
  //      throw dcds::exceptions::dcds_dynamic_exception("Variable name already used by data structure attribute");
  //    }
  //
  //    if (this->temp_variables.contains(name)) {
  //      throw dcds::exceptions::dcds_dynamic_exception("Variable name already used by another temporary variable");
  //    }
  //
  //    temp_variables.emplace(name, std::tuple<dcds::valueType, std::variant<int64_t, void *>>{varType, initVal});
  //  }

  auto getName() { return _name; }
  auto getReturnValueType() { return returnValueType; }

  ///
  /// \param theLLVMContext LLVM context from the codegen engine
  /// \param theLLVMModule  LLVM module from the codegen engine
  /// \param hasAttr        Flag to check if the data structure has attributes, if it has them, take a void pointer as
  ///                       first argument
  /// \return               Generated code for the function signature
  auto codegenFunctionSignature(std::unique_ptr<llvm::LLVMContext> &theLLVMContext,
                                std::unique_ptr<llvm::Module> &theLLVMModule, bool hasAttr) {
    std::vector<llvm::Type *> argTypes;
    llvm::Type *returnType = llvm::Type::getVoidTy(*theLLVMContext);

    if (hasAttr) argTypes.push_back(llvm::Type::getInt8PtrTy(*theLLVMContext));

    for (const auto &arg : function_arguments) {
      switch (arg.second) {
        case INTEGER:
        case RECORD_PTR: {
          argTypes.push_back(llvm::Type::getInt64PtrTy(*theLLVMContext));
          break;
        }

        case FLOAT:
        case DATE:
        case RECORD_ID:
        case CHAR:
        case VOID:
        default:
          assert(false && "valueTypeNotSupportedYet");
          break;
      }
    }

    switch (returnValueType) {
      case INTEGER: {
        returnType = llvm::Type::getInt64Ty(*theLLVMContext);
        break;
      }
      case RECORD_PTR: {
        returnType = llvm::Type::getInt8PtrTy(*theLLVMContext);
        break;
      }

      case FLOAT:
      case DATE:
      case RECORD_ID:
      case CHAR:
      case VOID:
      default:
        assert(false && "returnValueTypeNotSupportedYet");
        break;
    }

    auto fn_type = llvm::FunctionType::get(returnType, argTypes, false);
    auto fn =
        llvm::Function::Create(fn_type, llvm::GlobalValue::LinkageTypes::ExternalLinkage, _name, theLLVMModule.get());

    return fn;
  }

  auto addReadStatement(const std::shared_ptr<dcds::SimpleAttribute> &attribute, const std::string &destination) {
    // Ideally this should return a valueType or something operate-able so the user knows what is the return type?

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
    if (!(this->builder->hasAttribute(attribute))) {
      throw dcds::exceptions::dcds_dynamic_exception("Attribute not registered in the data structure");
    }

    if (!temp_variables.contains(source) && !hasArgument(source)) {
      throw dcds::exceptions::dcds_dynamic_exception("Function does not referenced source variable referenced: " +
                                                     source);
    }
    auto s = std::make_shared<StatementBuilder>(dcds::statementType::UPDATE, attribute->name, source);
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

  auto isAlwaysInline() { return _is_always_inline; }

  void setAlwaysInline() { _is_always_inline = true; }

 private:
  dcds::Builder *builder;  // convert to shared_ptr
  const std::string _name;

  dcds::valueType returnValueType;
  std::vector<std::pair<std::string, dcds::valueType>> function_arguments;

  std::unordered_map<std::string, std::tuple<dcds::valueType, void *>> temp_variables;

  std::deque<std::shared_ptr<StatementBuilder>> statements;

  bool _is_always_inline = false;
};

}  // namespace dcds

#endif  // DCDS_FUNCTION_BUILDER_HPP
