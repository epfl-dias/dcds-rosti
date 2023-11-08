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

#include "dcds/builder/statement-builder.hpp"

#include "dcds/builder/expressions/constant-expressions.hpp"
#include "dcds/builder/function-builder.hpp"

using namespace dcds;

void StatementBuilder::addReadStatement(const std::shared_ptr<dcds::Attribute> &attribute,
                                        const std::string &destination) {
  // Ideally this should return a valueType or something operate-able so the user knows what is the return type?
  // NOTE: Source will always be DS-attribute, and destination will be a temporary variable always.

  if (!(this->parent_function.hasAttribute(attribute))) {
    throw dcds::exceptions::dcds_dynamic_exception("Attribute not registered in the data structure");
  }

  if (!(this->parent_function.hasTempVariable(destination))) {
    throw dcds::exceptions::dcds_dynamic_exception("Function does not referenced destination variable referenced: " +
                                                   destination);
  }

  auto s = new ReadStatement(attribute->name, this->parent_function.getTempVariable(destination));
  statements.push_back(s);
}

void StatementBuilder::addReadStatement(const std::shared_ptr<dcds::Attribute> &attribute,
                                        const std::shared_ptr<expressions::LocalVariableExpression> &destination) {
  // How do we know if expr is valid?
  // checks?
  if (destination->getResultType() != attribute->type) {
    throw dcds::exceptions::dcds_invalid_type_exception("Type mismatch between attribute and destination variable");
  }

  auto rs = new ReadStatement(attribute->name, destination);
  statements.push_back(rs);
}

void StatementBuilder::addUpdateStatement(const std::shared_ptr<dcds::Attribute> &attribute,
                                          const std::string &source) {
  // NOTE: Destination will always be DS-attribute, and source can be either temporary variable or function argument.

  if (!(this->parent_function.hasAttribute(attribute))) {
    throw dcds::exceptions::dcds_dynamic_exception("Attribute not registered in the data structure");
  }

  if (!(this->parent_function.hasTempVariable(source)) && !(this->parent_function.hasArgument(source))) {
    throw dcds::exceptions::dcds_dynamic_exception("Function does not referenced source variable referenced: " +
                                                   source);
  }

  auto sourceType = this->parent_function.hasTempVariable(source) ? VAR_SOURCE_TYPE::TEMPORARY_VARIABLE
                                                                  : VAR_SOURCE_TYPE::FUNCTION_ARGUMENT;

  std::shared_ptr<expressions::Expression> source_expr;
  if (sourceType == VAR_SOURCE_TYPE::FUNCTION_ARGUMENT) {
    source_expr = this->parent_function.getArgument(source);
  } else {
    source_expr = this->parent_function.getTempVariable(source);
  }

  auto s = new UpdateStatement(attribute->name, source_expr);
  statements.push_back(s);
}

void StatementBuilder::addUpdateStatement(const std::shared_ptr<dcds::Attribute> &attribute,
                                          const std::shared_ptr<expressions::Expression> &source) {
  // How do we know if expr is valid?
  // checks?
  if (source->getResultType() != attribute->type) {
    throw dcds::exceptions::dcds_invalid_type_exception("Type mismatch between attribute and source expression");
  }

  auto s = new UpdateStatement(attribute->name, source);
  statements.push_back(s);
}

void StatementBuilder::addReturnStatement(const std::shared_ptr<expressions::Expression> &expr) {
  // How do we know if expr is valid?
  // checks?
  if (expr->getResultType() != this->parent_function.returnValueType) {
    throw dcds::exceptions::dcds_invalid_type_exception(
        "Return type mismatch between return variable and declared return type");
  }

  auto rs = new ReturnStatement(expr);
  statements.push_back(rs);
  this->doesReturn = true;
}

void StatementBuilder::addReturnStatement(const std::string &temporary_var_name) {
  // The reference variable should be a temporary variable.
  // what about the case of returning a constant?

  if (!(this->parent_function.hasTempVariable(temporary_var_name))) {
    throw dcds::exceptions::dcds_dynamic_exception("Function does not contain variable referenced to be returned: " +
                                                   temporary_var_name);
  }
  auto tmpVar = this->parent_function.getTempVariable(temporary_var_name);
  this->addReturnStatement(tmpVar);
}

void StatementBuilder::addReturnVoidStatement() {
  if (this->parent_function.returnValueType != dcds::valueType::VOID) {
    throw dcds::exceptions::dcds_invalid_type_exception(
        "Return type mismatch between return variable and declared return type");
  }

  auto rs = new ReturnStatement(nullptr);
  statements.push_back(rs);
  this->doesReturn = false;  // FIXME: ??
}

void StatementBuilder::addLogStatement(const std::string &log_string) {
  auto rs = new LogStringStatement(log_string);
  statements.push_back(rs);
}
void StatementBuilder::addLogStatement(const std::string &log_string,
                                       const std::vector<std::shared_ptr<expressions::Expression>> &expr) {
  auto rs = new LogStringStatement(log_string, expr);
  statements.push_back(rs);
}

std::shared_ptr<expressions::TemporaryVariableExpression> StatementBuilder::addInsertStatement(
    const std::string &registered_type_name, const std::string &variable_name) {
  assert(this->parent_function.hasRegisteredType(registered_type_name));

  auto resultVar = this->parent_function.addTempVariable(variable_name,
                                                         this->parent_function.getRegisteredType(registered_type_name));
  // this would call the constructor of the registered_type,
  // and then assigns the returned record_reference to the variable name,

  auto rs = new InsertStatement(registered_type_name, variable_name);
  statements.push_back(rs);
  return resultVar;
}

std::shared_ptr<expressions::TemporaryVariableExpression> StatementBuilder::addInsertStatement(
    const std::shared_ptr<Builder> &object_type, const std::string &variable_name) {
  return this->addInsertStatement(object_type->getName(), variable_name);
}

void StatementBuilder::addMethodCall(const std::shared_ptr<Builder> &object_type, const std::string &reference_variable,
                                     const std::string &function_name, const std::string &return_destination_variable,
                                     std::vector<std::string> function_args) {
  valueType referenceVarType;
  if (this->parent_function.hasTempVariable(reference_variable)) {
    auto var = this->parent_function.getTempVariable(reference_variable);
    referenceVarType = var->getType();
  } else if (this->parent_function.hasArgument(reference_variable)) {
    referenceVarType = this->parent_function.findArgument(reference_variable)->get()->getType();
  } else {
    throw dcds::exceptions::dcds_dynamic_exception("Reference variable does not exists in the scope: " +
                                                   reference_variable);
  }

  if (referenceVarType != dcds::valueType::RECORD_PTR) {
    throw dcds::exceptions::dcds_invalid_type_exception("Reference variable is not of type RECORD_PTR ");
  }
  // FIXME: how to check if the record_ptr is of the correct type, that is, object_type!

  if (!object_type->hasFunction(function_name)) {
    throw dcds::exceptions::dcds_dynamic_exception("Function (" + function_name +
                                                   ") does not exists in the type: " + object_type->getName());
  }

  auto method = object_type->getFunction(function_name);
  const auto &expected_args = method->getArguments();

  std::erase_if(function_args, [&](const auto &item) { return item.empty(); });

  if (function_args.size() != expected_args.size()) {
    throw dcds::exceptions::dcds_dynamic_exception(
        "number of function arguments does not matched expected number of arguments by the target function."
        " expected: " +
        std::to_string(expected_args.size()) + " provided: " + std::to_string(function_args.size()));
  }

  std::vector<std::shared_ptr<expressions::LocalVariableExpression>> arg_vars;
  auto i = 0;
  for (const std::string &arg_name : function_args) {
    if (!arg_name.empty()) {
      if (this->parent_function.hasTempVariable(arg_name)) {
        auto varg = this->parent_function.getTempVariable(arg_name);

        if (expected_args[i]->getType() != varg->getType()) {
          throw dcds::exceptions::dcds_invalid_type_exception("Type mismatch for argument " + arg_name);
        }
        arg_vars.emplace_back(std::make_shared<expressions::TemporaryVariableExpression>(*varg));
      } else if (this->parent_function.hasArgument(arg_name)) {
        auto varg = this->parent_function.findArgument(arg_name)->get();
        if (expected_args[i]->getType() != varg->getType()) {
          throw dcds::exceptions::dcds_invalid_type_exception("Type mismatch for argument " + arg_name);
        }
        arg_vars.emplace_back(std::make_shared<expressions::FunctionArgumentExpression>(*varg));
      } else {
        throw dcds::exceptions::dcds_dynamic_exception(
            "Argument is neither a temporary variable, nor a function argument: " + arg_name);
      }
    }
    i++;
  }

  // return destination
  std::shared_ptr<expressions::LocalVariableExpression> return_dest = nullptr;
  if (!(return_destination_variable.empty())) {
    if (this->parent_function.hasTempVariable(return_destination_variable)) {
      return_dest = this->parent_function.getTempVariable(return_destination_variable);
    } else if (this->parent_function.hasArgument(return_destination_variable)) {
      return_dest = *(this->parent_function.findArgument(return_destination_variable));
      if (!(return_dest->is_var_writable)) {
        throw dcds::exceptions::dcds_invalid_type_exception(
            "Function argument is not a reference type/ cannot save return value to a function argument passed by "
            "value.");
      }

    } else {
      throw dcds::exceptions::dcds_dynamic_exception(
          "Argument is neither a temporary variable, nor a function argument: " + return_destination_variable);
    }

    if (return_dest->getType() != method->getReturnValueType()) {
      LOG(INFO) << return_dest->getType();
      LOG(INFO) << method->getReturnValueType();
      throw dcds::exceptions::dcds_invalid_type_exception("Type mismatch for return destination.");
    }
  }

  auto rs = new MethodCallStatement(object_type->getFunction(function_name), reference_variable, return_dest,
                                    std::move(arg_vars));
  statements.push_back(rs);
  doesHaveMethodCalls = true;
}

StatementBuilder::conditional_blocks StatementBuilder::addConditionalBranch(dcds::expressions::Expression *expr) {
  assert(expr->getResultType() == valueType::BOOL);

  auto ifBranch = std::make_shared<StatementBuilder>(this->parent_function, this);
  auto elseBranch = std::make_shared<StatementBuilder>(this->parent_function, this);
  this->child_blocks += 2;

  // child_branches.emplace_back(ifBranch, elseBranch);

  auto rs = new ConditionalStatement(expr, ifBranch, elseBranch);
  statements.push_back(rs);

  return conditional_blocks{ifBranch, elseBranch};
}

void StatementBuilder::extractReadWriteSet_recursive(rw_set_t &read_set, rw_set_t &write_set) {
  this->for_each_statement([&](const Statement *stmt) {
    auto typeName = this->parent_function.builder->getName();
    LOG(INFO) << "extractReadWriteSet_recursive: " << typeName << "::" << this->parent_function.getName() << ": "
              << stmt->stType;

    // What about dependent read-write?

    if (stmt->stType == statementType::CONDITIONAL_STATEMENT) {
      // recurse.
      auto conditional = reinterpret_cast<const ConditionalStatement *>(stmt);
      conditional->ifBlock->extractReadWriteSet_recursive(read_set, write_set);
      conditional->elseBLock->extractReadWriteSet_recursive(read_set, write_set);
    } else if (stmt->stType == statementType::READ) {
      auto readStmt = reinterpret_cast<const ReadStatement *>(stmt);
      read_set[typeName].insert(readStmt->source_attr);
    } else if (stmt->stType == statementType::UPDATE) {
      auto updStmt = reinterpret_cast<const UpdateStatement *>(stmt);
      write_set[typeName].insert(updStmt->destination_attr);
    } else if (stmt->stType == statementType::METHOD_CALL) {
      auto method = reinterpret_cast<const MethodCallStatement *>(stmt);
      method->function_instance->entryPoint->extractReadWriteSet_recursive(read_set, write_set);
    }
    //    else {
    //      LOG(INFO) << "[extractReadSet_recursive] Ignoring statement type: " << stmt->stType;
    //    }
  });
}

static void indent_p(std::ostream &out, size_t indent_level) {
  for (auto i = 0; i < indent_level; i++) {
    out << "\t";
  }
}

void StatementBuilder::print(std::ostream &out, size_t indent_level) {
  for (const auto &s : this->statements) {
    indent_p(out, indent_level);
    out << "[" << s->stType << "]\t";

    if (s->stType == dcds::statementType::CONDITIONAL_STATEMENT) {
      auto conditional = reinterpret_cast<const ConditionalStatement *>(s);
      // auto conditional = std::static_pointer_cast<ConditionalStatement>(s);
      out << std::endl;
      auto curr_indent = indent_level + 1;
      indent_p(out, curr_indent);
      out << "if: " << conditional->expr->toString() << std::endl;

      indent_p(out, curr_indent);
      out << "then:" << std::endl;
      conditional->ifBlock->print(out, curr_indent + 1);
      if (!(conditional->elseBLock->statements.empty())) {
        indent_p(out, curr_indent);
        out << "else:" << std::endl;
        conditional->elseBLock->print(out, curr_indent + 1);
      }

    } else if (s->stType == dcds::statementType::READ) {
      auto st = reinterpret_cast<const ReadStatement *>(s);
      // auto st = std::static_pointer_cast<ReadStatement>(s);
      //      LOG(INFO) << "HERE";
      //      LOG(INFO) << st;
      //      LOG(INFO) << st->dest_expr.operator bool();
      //      LOG(INFO) << st->dest_expr->getName();
      out << "src: " << st->source_attr << ", dst: " << st->dest_expr->toString();

    } else if (s->stType == statementType::UPDATE) {
      auto st = reinterpret_cast<const UpdateStatement *>(s);
      // auto st = std::static_pointer_cast<UpdateStatement>(s);
      out << "src: " << st->source_expr->toString() << ", dst: " << st->destination_attr;

    } else if (s->stType == statementType::CREATE) {
      auto st = reinterpret_cast<const InsertStatement *>(s);
      // auto st = std::static_pointer_cast<InsertStatement>(s);
      out << st->destination_var << " = " << st->type_name << "()";

    } else if (s->stType == dcds::statementType::METHOD_CALL) {
      auto st = reinterpret_cast<const MethodCallStatement *>(s);
      // auto st = std::static_pointer_cast<MethodCallStatement>(s);
      if (st->has_return_dest) {
        out << st->return_dest->toString() << " = ";
      }
      out << st->referenced_type_variable << "(" << st->function_instance->builder->getName() << ")";
      out << "->" << st->function_instance->getName() << "(";
      for (auto i = 0; i < st->function_arguments.size(); i++) {
        out << st->function_arguments[i]->toString();
        if (i != st->function_arguments.size() - 1) {
          out << ", ";
        }
      }
      out << ")" << std::endl;
      st->function_instance->print(out, indent_level + 1);

    } else if (s->stType == dcds::statementType::YIELD) {
      auto st = reinterpret_cast<const ReturnStatement *>(s);
      // auto st = std::static_pointer_cast<ReturnStatement>(s);
      if (st->expr)  // non-void return
        out << st->expr->toString();
      else
        out << "VOID";

    } else if (s->stType == dcds::statementType::CC_LOCK_EXCLUSIVE ||
               s->stType == dcds::statementType::CC_LOCK_SHARED) {
      auto st = reinterpret_cast<const LockStatement *>(s);
      // auto st = std::static_pointer_cast<LockStatement>(s);
      out << " attribute: " << st->type_name << "::" << st->attribute;
    }

    out << std::endl;
  }
}

std::shared_ptr<StatementBuilder> StatementBuilder::clone_deep() {
  // NOTE: this is not a clean way, if we can avoid this, that would be the best option! didn't go through copy
  // constructor route as quite doubtful on it for now.
  LOG(INFO) << "here";

  // do we need new name for it?
  auto ret = std::make_shared<StatementBuilder>(this->parent_function, this->parent_block, this->name);

  ret->child_blocks = this->child_blocks;
  ret->doesReturn = this->doesReturn;
  ret->doesHaveMethodCalls = this->doesHaveMethodCalls;

  // copy statements
  // what if statements within are keeping something important or shared ptr? shouldnt it call clone method of
  // statement?
  LOG(INFO) << "sz: " << this->statements.size();

  for (auto st : this->statements) {
    LOG(INFO) << "Cloning : " << st->stType;
    if (st->stType == statementType::CONDITIONAL_STATEMENT) {
      auto cd = reinterpret_cast<ConditionalStatement *>(st);

      ret->statements.emplace_back(new ConditionalStatement(const_cast<dcds::expressions::Expression *>(cd->expr),
                                                            cd->ifBlock->clone_deep(),
                                                            cd->elseBLock ? cd->elseBLock->clone_deep() : nullptr));

    } else {
      ret->statements.emplace_back(st->clone());
    }
  }
  LOG(INFO) << "here-done";

  return ret;
}
