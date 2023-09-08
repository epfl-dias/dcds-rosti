//
// Created by Aunn Raza on 18.04.23.
//

#ifndef DCDS_STATEMENT_HPP
#define DCDS_STATEMENT_HPP

#include <dcds/builder/attribute.hpp>

namespace dcds {

enum statementType { READ, UPDATE, YIELD, TEMP_VAR_ADD, CONDITIONAL_STATEMENT };

enum CmpIPredicate { gt, eq, lt, neq };

class StatementBuilder {
 public:
  explicit StatementBuilder(std::string type, std::string actionVar, std::string refVar)
      : stType(type), sourceAttrName(actionVar), refVarName(refVar) {}

  std::string stType;
  std::string sourceAttrName;
  std::string refVarName;
};

class ConditionBuilder {
 public:
  ConditionBuilder(CmpIPredicate predicate, std::string condVar1, std::string condVar2)
      : pred(predicate), conditionVariableAttr1Name(condVar1), conditionVariableAttr2Name(condVar2) {}

  CmpIPredicate pred;
  std::string conditionVariableAttr1Name;
  std::string conditionVariableAttr2Name;
};

class ConditionStatementBuilder {
 public:
  ConditionStatementBuilder(ConditionBuilder cond, std::vector<std::shared_ptr<StatementBuilder>> ifRStatements,
                            std::vector<std::shared_ptr<StatementBuilder>> elseRStatements = {})
      : condition(cond), ifResStatements(ifRStatements), elseResStatements(elseRStatements) {}

  ConditionBuilder condition;
  std::vector<std::shared_ptr<StatementBuilder>> ifResStatements;
  std::vector<std::shared_ptr<StatementBuilder>> elseResStatements;
};
}  // namespace dcds

#endif  // DCDS_STATEMENT_HPP
