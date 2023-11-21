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
  friend class CCInjector;
  friend class LLVMCodegenStatement;

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
  void print(std::ostream &out, size_t indent_level = 0);

 public:
  void addLogStatement(const std::string &log_string);
  void addLogStatement(const std::string &log_string, const std::vector<std::shared_ptr<expressions::Expression>> &);

  void addReadStatement(const std::shared_ptr<dcds::Attribute> &attribute, const std::string &destination);
  void addReadStatement(const std::shared_ptr<dcds::Attribute> &attribute,
                        const std::shared_ptr<expressions::LocalVariableExpression> &destination);

  // For array/list
  void addReadStatement(const std::shared_ptr<dcds::Attribute> &attribute,
                        const std::shared_ptr<expressions::LocalVariableExpression> &destination,
                        const std::shared_ptr<dcds::expressions::Expression> &key);

  void addUpdateStatement(const std::shared_ptr<dcds::Attribute> &attribute, const std::string &source);
  void addUpdateStatement(const std::shared_ptr<dcds::Attribute> &attribute,
                          const std::shared_ptr<expressions::Expression> &source);

  void addReturnStatement(const std::string &temporary_var_name);
  void addReturnStatement(const std::shared_ptr<expressions::Expression> &expr);

  void addReturnVoidStatement();

  std::shared_ptr<expressions::TemporaryVariableExpression> addInsertStatement(const std::string &registered_type_name,
                                                                               const std::string &variable_name);
  std::shared_ptr<expressions::TemporaryVariableExpression> addInsertStatement(
      const std::shared_ptr<Builder> &object_type, const std::string &variable_name);

  // without-return + with/without args
  void addMethodCall(const std::shared_ptr<Builder> &object_type, const std::string &reference_variable,
                     const std::string &function_name, std::vector<std::string> args = {}) {
    return this->addMethodCall(object_type, reference_variable, function_name, "", std::move(args));
  }

  void addMethodCall(const std::shared_ptr<Builder> &object_type,
                     const std::shared_ptr<dcds::expressions::LocalVariableExpression> &reference_variable,
                     const std::string &function_name,
                     const std::vector<std::shared_ptr<dcds::expressions::Expression>> &args = {}) {
    return this->addMethodCall(object_type, reference_variable, function_name, nullptr, args);
  }

  // with-return + with/without args
  void addMethodCall(const std::shared_ptr<Builder> &object_type, const std::string &reference_variable,
                     const std::string &function_name, const std::string &return_destination_variable,
                     std::vector<std::string> args = {});

  void addMethodCall(const std::shared_ptr<Builder> &object_type,
                     const std::shared_ptr<dcds::expressions::LocalVariableExpression> &reference_variable,
                     const std::string &function_name,
                     const std::shared_ptr<expressions::LocalVariableExpression> &return_destination,
                     const std::vector<std::shared_ptr<dcds::expressions::Expression>> &args = {});

  conditional_blocks addConditionalBranch(dcds::expressions::Expression *expr);

 public:
  [[nodiscard]] bool haveReturnCall() const { return doesReturn; }
  [[nodiscard]] bool haveMethodCalls() const { return doesHaveMethodCalls; }
  [[nodiscard]] auto numChildBranches() const { return child_blocks; }
  auto hasParentBlock() { return parent_block != nullptr; }
  auto getParentBlock() { return parent_block; }
  auto getFunction() { return &parent_function; }

  void extractReadWriteSet_recursive(rw_set_t &read_set, rw_set_t &write_set);

 private:
  template <class lambda>
  inline void for_each_statement(lambda &&func) {
    if (this->statements.empty()) return;
    for (const auto &s : this->statements) {
      func(s);
      if (s->stType == dcds::statementType::CONDITIONAL_STATEMENT) {
        auto conditional = reinterpret_cast<const ConditionalStatement *>(s);
        conditional->ifBlock->for_each_statement(func);
        conditional->elseBLock->for_each_statement(func);
      }
    }
  }
  //  template <class lambda>
  //  inline void for_each_statement(lambda &&func) const {
  //    for (const auto &s : this->statements) {
  //      if(this->statements.empty())
  //        return;
  //      func(s);
  //      if (s->stType == dcds::statementType::CONDITIONAL_STATEMENT) {
  //        auto conditional = reinterpret_cast<const ConditionalStatement*>(s);
  //        conditional->ifBlock->for_each_statement(func);
  //        conditional->elseBLock->for_each_statement(func);
  //      }
  //    }
  //  }

 private:
  std::shared_ptr<expressions::LocalVariableExpression> MethodCall_getReturnDestination(
      const std::string &return_destination_variable);

 private:
  std::shared_ptr<StatementBuilder> clone_deep();

 private:
  bool doesReturn = false;
  bool doesHaveMethodCalls = false;
  size_t child_blocks = 0;
  std::deque<Statement *> statements;

  // NOTE: ideally, temporary variables should come here,
  // and then if not in the scope, it should be checked recursively in the call graph above.

 private:
  const std::string name;

  FunctionBuilder &parent_function;
  StatementBuilder *parent_block;

 private:
  static std::atomic<size_t> variable_name_index;
  std::shared_ptr<expressions::TemporaryVariableExpression> add_temp_var(const std::string &name_prefix,
                                                                         valueType type);
};

// std::ostream &operator<<(std::ostream &out, const StatementBuilder &sb);

}  // namespace dcds

#endif  // DCDS_STATEMENT_BUILDER_HPP
