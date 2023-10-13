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

namespace dcds {

class Builder;

/// Types of statements for DCDS functions
enum statementType { READ, UPDATE, CREATE, YIELD, TEMP_VAR_ADD, CONDITIONAL_STATEMENT, METHOD_CALL, LOG_STRING };

/// Types of comparisons for DCDS comparison statements
enum CmpIPredicate { gt, eq, lt, neq };

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
  dcds::statementType stType;
  /// Variable name on which some action (read/update) will be taken. Not a hard specification for all statements.
  std::string actionVarName;
  /// Variable name which will be used as reference for the action on action variable (will store read/write value).
  /// Not a hard specification for all statements
  std::string refVarName;
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
  explicit MethodCallStatement(std::shared_ptr<dcds::Builder> object_type, const std::string& reference_variable,
                               std::string _function_name, const std::string& return_destination_variable,
                               std::vector<std::pair<std::string, dcds::VAR_SOURCE_TYPE>> _function_arguments = {})
      : Statement(statementType::METHOD_CALL, reference_variable, return_destination_variable),
        function_name(std::move(_function_name)),
        function_arguments(std::move(_function_arguments)),
        object_type_info(std::move(object_type)) {}

  const std::string function_name;
  const std::vector<std::pair<std::string, dcds::VAR_SOURCE_TYPE>> function_arguments;
  const std::shared_ptr<dcds::Builder> object_type_info;
};

class LogStringStatement : public Statement {
 public:
  explicit LogStringStatement(const std::string& log_string) : Statement(statementType::LOG_STRING, log_string, "") {}
};

/// Class to represent conditional statements in DCDS
class ConditionBuilder {
 public:
  ///
  /// \param predicate Type of comparison
  /// \param condVar1 First variable name for comparison
  /// \param condVar2 Second variable name for comparison
  ConditionBuilder(CmpIPredicate predicate, std::string condVar1, std::string condVar2)
      : pred(predicate), conditionVariableAttr1Name(condVar1), conditionVariableAttr2Name(condVar2) {}

  /// Type of Comparison
  CmpIPredicate pred;
  /// First variable name for comparison
  std::string conditionVariableAttr1Name;
  /// Second variable name for comparison
  std::string conditionVariableAttr2Name;
};

/// Class to represent conditional blocks in DCDS
class ConditionStatementBuilder {
 public:
  ///
  /// \param cond            Condition for the block
  /// \param ifRStatements   Statements residing in if block
  /// \param elseRStatements Statements residing in else block
  ConditionStatementBuilder(ConditionBuilder cond, std::vector<std::shared_ptr<Statement>> ifRStatements,
                            std::vector<std::shared_ptr<Statement>> elseRStatements = {})
      : condition(cond), ifResStatements(ifRStatements), elseResStatements(elseRStatements) {}

  /// Condition for the block
  ConditionBuilder condition;
  /// Statements residing in if block
  std::vector<std::shared_ptr<Statement>> ifResStatements;
  /// Statement residing in else block
  std::vector<std::shared_ptr<Statement>> elseResStatements;
};
}  // namespace dcds

#endif  // DCDS_STATEMENT_HPP
