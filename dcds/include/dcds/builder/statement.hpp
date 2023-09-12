//
// Created by Aunn Raza on 18.04.23.
//

#ifndef DCDS_STATEMENT_HPP
#define DCDS_STATEMENT_HPP

#include <dcds/builder/attribute.hpp>

namespace dcds {

/// Types of statements for DCDS functions
enum statementType { READ, UPDATE, YIELD, TEMP_VAR_ADD, CONDITIONAL_STATEMENT, CALL };

/// Types of comparisons for DCDS comparison statements
enum CmpIPredicate { gt, eq, lt, neq };

/// Cass representing a statement in DCDS
class StatementBuilder {
 public:
  ///
  /// \param type       Type of the statement to be constructed
  /// \param actionVar  Variable name on which some action will be taken (read/updated).
  ///                   Not a hard specification for all statements
  /// \param refVar     Variable name which will be used as reference for the action on
  ///                   actionVar (will store read/write value). Not a hard specification for all statements
  explicit StatementBuilder(dcds::statementType type, std::string actionVar, std::string refVar)
      : stType(type), actionVarName(actionVar), refVarName(refVar) {}

  /// Type of the statement
  dcds::statementType stType;
  /// Variable name on which some action (read/update) will be taken. Not a hard specification for all statements.
  std::string actionVarName;
  /// Variable name which will be used as reference for the action on action variable (will store read/write value).
  /// Not a hard specification for all statements
  std::string refVarName;
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
  ConditionStatementBuilder(ConditionBuilder cond, std::vector<std::shared_ptr<StatementBuilder>> ifRStatements,
                            std::vector<std::shared_ptr<StatementBuilder>> elseRStatements = {})
      : condition(cond), ifResStatements(ifRStatements), elseResStatements(elseRStatements) {}

  /// Condition for the block
  ConditionBuilder condition;
  /// Statements residing in if block
  std::vector<std::shared_ptr<StatementBuilder>> ifResStatements;
  /// Statement residing in else block
  std::vector<std::shared_ptr<StatementBuilder>> elseResStatements;
};
}  // namespace dcds

#endif  // DCDS_STATEMENT_HPP
