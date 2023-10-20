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

enum class statementType { READ, UPDATE, CREATE, YIELD, CONDITIONAL_STATEMENT, METHOD_CALL, LOG_STRING };
inline std::ostream& operator<<(std::ostream& os, dcds::statementType ty) {
  os << "dcds::statementType::";
  switch (ty) {
    case dcds::statementType::READ:
      os << "READ";
      break;
    case statementType::UPDATE:
      os << "UPDATE";
      break;
    case statementType::CREATE:
      os << "CREATE";
      break;
    case statementType::YIELD:
      os << "YIELD";
      break;
    case statementType::CONDITIONAL_STATEMENT:
      os << "CONDITIONAL_STATEMENT";
      break;
    case statementType::METHOD_CALL:
      os << "METHOD_CALL";
      break;
    case statementType::LOG_STRING:
      os << "LOG_STRING";
      break;
  }
  return os;
}

class Statement {
 protected:
  explicit Statement(dcds::statementType type) : stType(type) {}

 public:
  const dcds::statementType stType;
};

// template <class T>
// class StatementCRTP{
//  public:
//   std::shared_ptr<T> downcast(std::shared_ptr<Statement> s){
//     return std::static_pointer_cast<T>(s);
//   }
// };

class ReturnStatement : public Statement {
 public:
  explicit ReturnStatement(std::shared_ptr<expressions::Expression> _expr)
      : Statement(statementType::YIELD), expr(std::move(_expr)) {}

  const std::shared_ptr<expressions::Expression> expr;
};

class ReadStatement : public Statement {
 public:
  explicit ReadStatement(std::string source_attribute, std::shared_ptr<expressions::Expression> destination)
      : Statement(statementType::READ), source_attr(std::move(source_attribute)), dest_expr(std::move(destination)) {}

  const std::string source_attr;
  const std::shared_ptr<expressions::Expression> dest_expr;
};

class UpdateStatement : public Statement {
 public:
  explicit UpdateStatement(std::string destination_attribute, std::shared_ptr<expressions::Expression> source)
      : Statement(statementType::UPDATE),
        destination_attr(std::move(destination_attribute)),
        source_expr(std::move(source)) {}

  const std::string destination_attr;
  const std::shared_ptr<expressions::Expression> source_expr;
};

class InsertStatement : public Statement {
 public:
  explicit InsertStatement(std::string _type_name, std::string var_name)
      : Statement(statementType::CREATE), type_name(std::move(_type_name)), destination_var(std::move(var_name)) {}

  const std::string type_name;
  const std::string destination_var;  // to-be changed to localVarExpression.
};

class MethodCallStatement : public Statement {
 public:
  explicit MethodCallStatement(
      std::shared_ptr<dcds::Builder> object_type, std::string reference_variable, std::string _function_name,
      std::string return_destination_variable,
      std::vector<std::shared_ptr<expressions::LocalVariableExpression>> _function_arguments = {})
      : Statement(statementType::METHOD_CALL),
        function_name(std::move(_function_name)),
        function_arguments(std::move(_function_arguments)),
        object_type_info(std::move(object_type)),
        referenced_type_variable(std::move(reference_variable)),
        return_dest_variable(std::move(return_destination_variable)) {}

  const std::string function_name;
  const std::vector<std::shared_ptr<expressions::LocalVariableExpression>> function_arguments;
  const std::shared_ptr<dcds::Builder> object_type_info;
  const std::string referenced_type_variable;
  const std::string return_dest_variable;
};

class LogStringStatement : public Statement {
 public:
  explicit LogStringStatement(std::string msg) : Statement(statementType::LOG_STRING), log_string(std::move(msg)) {}

  const std::string log_string;
};

class ConditionalStatement : public Statement {
 public:
  explicit ConditionalStatement(expressions::Expression* _expr, std::shared_ptr<StatementBuilder> if_block,
                                std::shared_ptr<StatementBuilder> else_block)
      : Statement(statementType::CONDITIONAL_STATEMENT),
        expr(_expr),
        ifBlock(std::move(if_block)),
        elseBLock(std::move(else_block)) {}

  const expressions::Expression* expr;
  const std::shared_ptr<StatementBuilder> ifBlock;
  const std::shared_ptr<StatementBuilder> elseBLock;
};

}  // namespace dcds

#endif  // DCDS_STATEMENT_HPP
