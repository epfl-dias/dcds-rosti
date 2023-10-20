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

#ifndef DCDS_STATEMENT_BUILDER_HPP
#define DCDS_STATEMENT_BUILDER_HPP

#include <iostream>
#include <utility>

#include "dcds/builder/statement.hpp"

namespace dcds {

class FunctionBuilder;

class StatementBuilder {
  friend class Builder;
  friend class FunctionBuilder;
  friend class Codegen;
  friend class LLVMCodegen;
  friend class BuilderOptPasses;

 public:
  struct conditional_blocks {
    std::shared_ptr<StatementBuilder> ifBlock;
    std::shared_ptr<StatementBuilder> elseBlock;
    conditional_blocks(decltype(ifBlock) if_block, decltype(elseBlock) else_block)
        : ifBlock(std::move(if_block)), elseBlock(std::move(else_block)) {}
  };

 public:
  // NOTE: to be created by function builder only, however, as we make a shared_ptr, having a private constructor does
  // not allow shared_ptr's constructAt.
  explicit StatementBuilder(FunctionBuilder &parentFunction, StatementBuilder *parentBlock = nullptr,
                            std::string _name = "")
      : name(std::move(_name)), parent_function(parentFunction), parent_block(parentBlock) {}

 public:
  void addLogStatement(const std::string &log_string);

  void addReadStatement(const std::shared_ptr<dcds::Attribute> &attribute, const std::string &destination);

  void addUpdateStatement(const std::shared_ptr<dcds::Attribute> &attribute, const std::string &source);

  void addReturnStatement(const std::string &temporary_var_name);
  void addReturnStatement(const std::shared_ptr<expressions::Expression> &expr);

  void addReturnVoidStatement();

  std::shared_ptr<expressions::TemporaryVariableExpression> addInsertStatement(const std::string &registered_type_name,
                                                                               const std::string &variable_name);
  std::shared_ptr<expressions::TemporaryVariableExpression> addInsertStatement(
      const std::shared_ptr<Builder> &object_type, const std::string &variable_name);

  void addMethodCall(const std::shared_ptr<Builder> &object_type, const std::string &reference_variable,
                     const std::string &function_name, const std::string &return_destination_variable,
                     std::convertible_to<std::string> auto... args) {
    // NOTE: good background-reading on parameter-packs:
    // https://www.scs.stanford.edu/~dm/blog/param-pack.html#function-parameter-packs
    std::vector<std::string> arg_list;
    // filter out for the empty.
    for (const std::string &a : {args...}) {
      if (!(a.empty())) {
        arg_list.emplace_back(a);
      }
    }
    this->addMethodCall(object_type, reference_variable, function_name, return_destination_variable, arg_list);
  }

  void addMethodCall(const std::shared_ptr<Builder> &object_type, const std::string &reference_variable,
                     const std::string &function_name, const std::string &return_destination_variable);

  void addMethodCall(const std::shared_ptr<Builder> &object_type, const std::string &reference_variable,
                     const std::string &function_name, const std::string &return_destination_variable,
                     const std::vector<std::string> &args);

  conditional_blocks addConditionalBranch(dcds::expressions::Expression *expr);

 public:
  [[nodiscard]] bool haveReturnCall() const { return doesReturn; }
  [[nodiscard]] bool haveMethodCalls() const { return doesHaveMethodCalls; }
  [[nodiscard]] auto numChildBranches() const { return child_blocks; }
  auto hasParentBlock() { return parent_block != nullptr; }
  auto getParentBlock() { return parent_block; }
  auto getFunction() { return &parent_function; }

 private:
  template <class lambda>
  inline void for_each_statement(lambda &&func) {
    for (const auto &s : this->statements) {
      func(s);
      if (s->stType == dcds::statementType::CONDITIONAL_STATEMENT) {
        auto conditional = std::static_pointer_cast<ConditionalStatement>(s);
        conditional->ifBlock->for_each_statement(func);
        conditional->elseBLock->for_each_statement(func);
      }
    }
  }

 private:
  bool doesReturn = false;
  bool doesHaveMethodCalls = false;
  size_t child_blocks = 0;
  //  std::deque<std::shared_ptr<StatementBuilder>> child_branches;
  std::deque<conditional_blocks> child_branches;

  // NOTE: ideally, temporary variables should come here,
  // and then if not in the scope, it should be checked recursively in the call graph above.

 private:
  const std::string name;
  std::deque<std::shared_ptr<Statement>> statements;

  FunctionBuilder &parent_function;
  StatementBuilder *parent_block;
};

}  // namespace dcds

#endif  // DCDS_STATEMENT_BUILDER_HPP
