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
class FunctionBuilder;

enum class statementType {
  READ,
  READ_INDEXED,
  INSERT_INDEXED,
  REMOVE_INDEXED,

  UPDATE,
  CREATE,
  YIELD,
  CONDITIONAL_STATEMENT,
  METHOD_CALL,
  LOG_STRING,
  //  CC_LOCK_SHARED,
  //  CC_LOCK_EXCLUSIVE,
  CC_LOCK,
  TEMP_VAR_ASSIGN,

  FOR_LOOP,
  WHILE_LOOP,
  DO_WHILE_LOOP
};
inline std::ostream& operator<<(std::ostream& os, dcds::statementType ty) {
  os << "statementType::";
  // os << static_cast<int>(ty) << "::";
  switch (ty) {
    case dcds::statementType::READ:
      os << "READ";
      break;
    case dcds::statementType::READ_INDEXED:
      os << "READ_INDEXED";
      break;
    case dcds::statementType::INSERT_INDEXED:
      os << "INSERT_INDEXED";
      break;
    case dcds::statementType::REMOVE_INDEXED:
      os << "REMOVE_INDEXED";
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
    case statementType::FOR_LOOP:
      os << "FOR_LOOP";
      break;
    case statementType::WHILE_LOOP:
      os << "WHILE_LOOP";
      break;
    case statementType::DO_WHILE_LOOP:
      os << "DO_WHILE_LOOP";
      break;
    case statementType::METHOD_CALL:
      os << "METHOD_CALL";
      break;
    case statementType::LOG_STRING:
      os << "LOG_STRING";
      break;
    case statementType::CC_LOCK:
      os << "CC_LOCK";
      break;
      //    case statementType::CC_LOCK_SHARED:
      //      os << "CC_LOCK_SHARED";
      //      break;
      //    case statementType::CC_LOCK_EXCLUSIVE:
      //      os << "CC_LOCK_EXCLUSIVE";
      //      break;
    case statementType::TEMP_VAR_ASSIGN:
      os << "TEMP_VAR_ASSIGN";
      break;
  }
  return os;
}

class Statement {
 protected:
  explicit Statement(dcds::statementType type) : stType(type) {}
  Statement(const Statement&) = default;
  //  Statement &operator=(const Statement &) = default;
  virtual ~Statement() = default;

 public:
  const dcds::statementType stType;

 public:
  [[nodiscard]] virtual Statement* clone() const = 0;
};

// template <class T>
// class StatementCRTP{
//  public:
//   std::shared_ptr<T> downcast(std::shared_ptr<Statement> s){
//     return std::static_pointer_cast<T>(s);
//   }
// };

class TempVarAssignStatement : public Statement {
 public:
  explicit TempVarAssignStatement(std::shared_ptr<expressions::Expression> _source,
                                  std::shared_ptr<expressions::LocalVariableExpression> destination)
      : Statement(statementType::TEMP_VAR_ASSIGN), dest(std::move(destination)), source(std::move(_source)) {}
  TempVarAssignStatement(const TempVarAssignStatement&) = default;

  const std::shared_ptr<expressions::LocalVariableExpression> dest;
  const std::shared_ptr<expressions::Expression> source;

  [[nodiscard]] Statement* clone() const override { return new TempVarAssignStatement(*this); }
  ~TempVarAssignStatement() override = default;
};

class ReturnStatement : public Statement {
 public:
  explicit ReturnStatement(std::shared_ptr<expressions::Expression> _expr)
      : Statement(statementType::YIELD), expr(std::move(_expr)) {}
  ReturnStatement(const ReturnStatement&) = default;

  const std::shared_ptr<expressions::Expression> expr;

  [[nodiscard]] Statement* clone() const override { return new ReturnStatement(*this); }
  ~ReturnStatement() override = default;
};

class ReadStatement : public Statement {
 public:
  explicit ReadStatement(std::string source_attribute,
                         std::shared_ptr<expressions::LocalVariableExpression> destination)
      : Statement(statementType::READ), source_attr(std::move(source_attribute)), dest_expr(std::move(destination)) {}
  ReadStatement(const ReadStatement&) = default;

  const std::string source_attr;
  const std::shared_ptr<expressions::LocalVariableExpression> dest_expr;

  [[nodiscard]] Statement* clone() const override { return new ReadStatement(*this); }
  ~ReadStatement() override = default;
};

class UpdateStatement : public Statement {
 public:
  explicit UpdateStatement(std::string destination_attribute, std::shared_ptr<expressions::Expression> source)
      : Statement(statementType::UPDATE),
        destination_attr(std::move(destination_attribute)),
        source_expr(std::move(source)) {}
  UpdateStatement(const UpdateStatement&) = default;

  const std::string destination_attr;
  const std::shared_ptr<expressions::Expression> source_expr;

 public:
  [[nodiscard]] Statement* clone() const override { return new UpdateStatement(*this); }
  ~UpdateStatement() override = default;
};

class InsertStatement : public Statement {
 public:
  explicit InsertStatement(std::string _type_name, std::string var_name)
      : Statement(statementType::CREATE), type_name(std::move(_type_name)), destination_var(std::move(var_name)) {}
  InsertStatement(const InsertStatement&) = default;

  const std::string type_name;
  const std::string destination_var;  // to-be changed to localVarExpression.

 public:
  [[nodiscard]] Statement* clone() const override { return new InsertStatement(*this); }
  ~InsertStatement() override = default;
};

class MethodCallStatement : public Statement {
 public:
  explicit MethodCallStatement(std::shared_ptr<FunctionBuilder> function_call, std::string reference_variable,
                               std::shared_ptr<expressions::LocalVariableExpression> return_destination,
                               std::vector<std::shared_ptr<expressions::Expression>> _function_arguments = {})
      : Statement(statementType::METHOD_CALL),
        function_instance(std::move(function_call)),
        function_arguments(std::move(_function_arguments)),
        referenced_type_variable(std::move(reference_variable)),
        return_dest(std::move(return_destination)),
        has_return_dest(return_dest.operator bool()) {}

  MethodCallStatement(const MethodCallStatement&) = default;

  std::shared_ptr<FunctionBuilder> function_instance;
  const std::vector<std::shared_ptr<expressions::Expression>> function_arguments;
  const std::string referenced_type_variable;
  const std::shared_ptr<expressions::LocalVariableExpression> return_dest;
  const bool has_return_dest;

 public:
  [[nodiscard]] Statement* clone() const override { return new MethodCallStatement(*this); }
  ~MethodCallStatement() override = default;
};

class LogStringStatement : public Statement {
 public:
  explicit LogStringStatement(std::string msg) : Statement(statementType::LOG_STRING), log_string(std::move(msg)) {}
  explicit LogStringStatement(std::string msg, std::vector<std::shared_ptr<expressions::Expression>> _args)
      : Statement(statementType::LOG_STRING), log_string(std::move(msg)), args(std::move(_args)) {}

  LogStringStatement(const LogStringStatement&) = default;

  const std::string log_string;
  const std::vector<std::shared_ptr<expressions::Expression>> args;

 public:
  [[nodiscard]] Statement* clone() const override { return new LogStringStatement(*this); }
  ~LogStringStatement() override = default;
};

class ConditionalStatement : public Statement {
 public:
  explicit ConditionalStatement(expressions::Expression* _expr, std::shared_ptr<StatementBuilder> if_block,
                                std::shared_ptr<StatementBuilder> else_block)
      : Statement(statementType::CONDITIONAL_STATEMENT),
        expr(_expr),
        ifBlock(std::move(if_block)),
        elseBLock(std::move(else_block)) {
    assert(ifBlock);
  }
  ConditionalStatement(const ConditionalStatement&) = default;

  const expressions::Expression* expr;
  const std::shared_ptr<StatementBuilder> ifBlock;
  const std::shared_ptr<StatementBuilder> elseBLock;

 public:
  [[nodiscard]] Statement* clone() const override;
  ~ConditionalStatement() override = default;
};

// Loops
class LoopStatement : public Statement {
 public:
  explicit LoopStatement(dcds::statementType type, std::shared_ptr<StatementBuilder> loop_body)
      : Statement(type), body(std::move(loop_body)) {
    assert(body);
  }

  LoopStatement(const LoopStatement&) = default;
  ~LoopStatement() override = default;

  const std::shared_ptr<StatementBuilder> body;
};

class ForLoopStatement : public LoopStatement {
 public:
  explicit ForLoopStatement(expressions::LocalVariableExpression* loop_var_, expressions::Expression* loop_cond_expr,
                            expressions::Expression* loop_iteration_expr, std::shared_ptr<StatementBuilder> loop_body)
      : LoopStatement(statementType::FOR_LOOP, std::move(loop_body)),
        loop_var(loop_var_),
        cond_expr(loop_cond_expr),
        iteration_expr(loop_iteration_expr) {}
  ForLoopStatement(const ForLoopStatement&) = default;

  const expressions::LocalVariableExpression* loop_var;
  const expressions::Expression* cond_expr;
  const expressions::Expression* iteration_expr;

 public:
  [[nodiscard]] Statement* clone() const override;
  ~ForLoopStatement() override = default;
};

class WhileLoopStatement : public LoopStatement {
 public:
  explicit WhileLoopStatement(expressions::Expression* loop_cond_expr, std::shared_ptr<StatementBuilder> loop_body)
      : LoopStatement(statementType::WHILE_LOOP, std::move(loop_body)), cond_expr(loop_cond_expr) {}
  WhileLoopStatement(const WhileLoopStatement&) = default;

  const expressions::Expression* cond_expr;

 public:
  [[nodiscard]] Statement* clone() const override;
  ~WhileLoopStatement() override = default;
};

class DoWhileLoopStatement : public LoopStatement {
 public:
  explicit DoWhileLoopStatement(expressions::Expression* loop_cond_expr, std::shared_ptr<StatementBuilder> loop_body)
      : LoopStatement(statementType::DO_WHILE_LOOP, std::move(loop_body)), cond_expr(loop_cond_expr) {
    assert(body);
  }
  DoWhileLoopStatement(const DoWhileLoopStatement&) = default;

  const expressions::Expression* cond_expr;

 public:
  [[nodiscard]] Statement* clone() const override;
  ~DoWhileLoopStatement() override = default;
};

// class LockStatement : public Statement {
//  public:
//   explicit LockStatement(std::string typeName, std::string attribute_name, size_t typeID, bool lock_exclusive = true)
//       : Statement(lock_exclusive ? statementType::CC_LOCK_EXCLUSIVE : statementType::CC_LOCK_SHARED),
//         attribute(std::move(attribute_name)),
//         type_name(std::move(typeName)),
//         type_id(typeID) {}
//   LockStatement(const LockStatement&) = default;
//
//   const std::string attribute;
//   const std::string type_name;
//   const size_t type_id;
//
//  public:
//   [[nodiscard]] Statement* clone() const override { return new LockStatement(*this); }
//   ~LockStatement() override = default;
// };

class LockStatement2 : public Statement {
 public:
  explicit LockStatement2(std::string typeName, std::string attribute_name, size_t typeID, bool lock_exclusive = true)
      : Statement(statementType::CC_LOCK),
        attribute(std::move(attribute_name)),
        type_name(std::move(typeName)),
        type_id(typeID),
        is_exclusive(lock_exclusive) {}
  LockStatement2(const LockStatement2&) = default;

  const std::string attribute;
  const std::string type_name;
  const size_t type_id;
  bool is_exclusive;

 public:
  [[nodiscard]] Statement* clone() const override { return new LockStatement2(*this); }
  ~LockStatement2() override = default;
};

// IndexedList

class ReadIndexedStatement : public Statement {
 public:
  explicit ReadIndexedStatement(std::string source_attribute,
                                std::shared_ptr<expressions::LocalVariableExpression> destination,
                                std::shared_ptr<expressions::Expression> index_key, bool fixed_integer_indexed)
      : Statement(statementType::READ_INDEXED),
        source_attr(std::move(source_attribute)),
        dest_expr(std::move(destination)),
        index_expr(std::move(index_key)),
        integer_indexed(fixed_integer_indexed) {}

  ReadIndexedStatement(const ReadIndexedStatement&) = default;

  const std::string source_attr;
  const std::shared_ptr<expressions::LocalVariableExpression> dest_expr;
  const std::shared_ptr<expressions::Expression> index_expr;
  const bool integer_indexed;

  [[nodiscard]] Statement* clone() const override { return new ReadIndexedStatement(*this); }

  ~ReadIndexedStatement() override = default;
};

class InsertIndexedStatement : public Statement {
 public:
  explicit InsertIndexedStatement(std::string source_attribute, std::shared_ptr<expressions::Expression> index_key,
                                  std::shared_ptr<expressions::LocalVariableExpression> value)
      : Statement(statementType::INSERT_INDEXED),
        source_attr(std::move(source_attribute)),
        value_expr(std::move(value)),
        index_expr(std::move(index_key)) {}

  InsertIndexedStatement(const InsertIndexedStatement&) = default;

  const std::string source_attr;
  const std::shared_ptr<expressions::LocalVariableExpression> value_expr;
  const std::shared_ptr<expressions::Expression> index_expr;

  [[nodiscard]] Statement* clone() const override { return new InsertIndexedStatement(*this); }

  ~InsertIndexedStatement() override = default;
};

class RemoveIndexedStatement : public Statement {
 public:
  explicit RemoveIndexedStatement(std::string source_attribute, std::shared_ptr<expressions::Expression> index_key)
      : Statement(statementType::REMOVE_INDEXED),
        source_attr(std::move(source_attribute)),
        index_expr(std::move(index_key)) {}

  RemoveIndexedStatement(const RemoveIndexedStatement&) = default;

  const std::string source_attr;
  const std::shared_ptr<expressions::Expression> index_expr;

  [[nodiscard]] Statement* clone() const override { return new RemoveIndexedStatement(*this); }

  ~RemoveIndexedStatement() override = default;
};

// class UpdateIndexedStatement: public Statement {
//
// };
//
// class UpsertIndexedStatement : public Statement {
//
// };

}  // namespace dcds

#endif  // DCDS_STATEMENT_HPP
