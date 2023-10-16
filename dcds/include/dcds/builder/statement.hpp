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

#ifndef DCDS_STATEMENT_HPP
#define DCDS_STATEMENT_HPP

#include <dcds/builder/attribute.hpp>
#include <utility>

#include "dcds/builder/expressions/expressions.hpp"
#include "dcds/builder/expressions/unary-expressions.hpp"

namespace dcds {

class Builder;
class StatementBuilder;

/// Types of statements for DCDS functions
enum statementType { READ, UPDATE, CREATE, YIELD, CONDITIONAL_STATEMENT, METHOD_CALL, LOG_STRING };

/// Cass representing a statement in DCDS
class Statement {
 public:
  ///
  /// \param type       Type of the statement to be constructed
  /// \param actionVar  Variable name on which some action will be taken (read/updated).
  ///                   Not a hard specification for all statements
  /// \param refVar     Variable name which will be used as reference for the action on
  ///                   actionVar (will store read/write value). Not a hard specification for all statements
  explicit Statement(dcds::statementType type, std::string actionVar, std::string refVar)
      : stType(type), actionVarName(std::move(actionVar)), refVarName(std::move(refVar)) {}

  /// Type of the statement
  const dcds::statementType stType;
  /// Variable name on which some action (read/update) will be taken. Not a hard specification for all statements.
  const std::string actionVarName;
  /// Variable name which will be used as reference for the action on action variable (will store read/write value).
  /// Not a hard specification for all statements
  const std::string refVarName;
};

class UpdateStatement : public Statement {
 public:
  explicit UpdateStatement(const std::string& destination, const std::string& source, VAR_SOURCE_TYPE sourceType)
      : Statement(statementType::UPDATE, destination, source), source_type(sourceType) {}

  const VAR_SOURCE_TYPE source_type;
};

class InsertStatement : public Statement {
 public:
  explicit InsertStatement(const std::string& type_name, const std::string& var_name)
      : Statement(statementType::CREATE, type_name, var_name) {}
};

class MethodCallStatement : public Statement {
 public:
  explicit MethodCallStatement(
      std::shared_ptr<dcds::Builder> object_type, const std::string& reference_variable, std::string _function_name,
      const std::string& return_destination_variable,
      std::vector<std::shared_ptr<expressions::LocalVariableExpression>> _function_arguments = {})
      : Statement(statementType::METHOD_CALL, reference_variable, return_destination_variable),
        function_name(std::move(_function_name)),
        function_arguments(std::move(_function_arguments)),
        object_type_info(std::move(object_type)) {}

  const std::string function_name;
  const std::vector<std::shared_ptr<expressions::LocalVariableExpression>> function_arguments;
  const std::shared_ptr<dcds::Builder> object_type_info;
};

class LogStringStatement : public Statement {
 public:
  explicit LogStringStatement(const std::string& log_string) : Statement(statementType::LOG_STRING, log_string, "") {}
};

class ConditionalStatement : public Statement {
 public:
  // enum BRANCH_TYPE { IF_BLOCK, ELSE_BLOCK };

  explicit ConditionalStatement(expressions::Expression* _expr, std::shared_ptr<StatementBuilder> if_block,
                                std::shared_ptr<StatementBuilder> else_block)
      : Statement(statementType::CONDITIONAL_STATEMENT, "", ""),
        expr(_expr),
        ifBlock(std::move(if_block)),
        elseBLock(std::move(else_block)) {}

  const expressions::Expression* expr;
  const std::shared_ptr<StatementBuilder> ifBlock;
  const std::shared_ptr<StatementBuilder> elseBLock;
};

}  // namespace dcds

#endif  // DCDS_STATEMENT_HPP
