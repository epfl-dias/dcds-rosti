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

std::atomic<size_t> StatementBuilder::variable_name_index = 0;
std::shared_ptr<expressions::TemporaryVariableExpression> StatementBuilder::add_temp_var(const std::string &name_prefix,
                                                                                         valueType type) {
  return this->parent_function.addTempVariable(name_prefix + "_" + std::to_string(variable_name_index.fetch_add(1)),
                                               type);
}

void StatementBuilder::addReadStatement(const std::shared_ptr<dcds::Attribute> &attribute,
                                        const std::string &destination) {
  if (this->parent_function.hasTempVariable(destination)) {
    return this->addReadStatement(attribute, this->parent_function.getTempVariable(destination));
  } else if (this->parent_function.hasArgument(destination)) {
    auto func_arg = this->parent_function.getArgument(destination);
    // CHECK(func_arg->is_var_writable) << "Function argument is not writeable";
    return this->addReadStatement(attribute, this->parent_function.getArgument(destination));
  } else {
    CHECK(false) << "Function does not have referenced destination variable: " << destination;
  }
}

void StatementBuilder::addReadStatement(const std::shared_ptr<dcds::Attribute> &attribute,
                                        const std::shared_ptr<expressions::LocalVariableExpression> &destination) {
  CHECK(this->parent_function.hasAttribute(attribute)) << "Attribute not registered in the data structure";
  CHECK(destination->getType() == attribute->type) << "Type mismatch between attribute and destination variable";

  auto rs = new ReadStatement(attribute->name, destination);
  statements.push_back(rs);
}

void StatementBuilder::addReadStatement(const std::shared_ptr<dcds::Attribute> &attribute,
                                        const std::shared_ptr<expressions::LocalVariableExpression> &destination,
                                        const std::shared_ptr<dcds::expressions::Expression> &key) {
  CHECK(attribute->type_category == ATTRIBUTE_TYPE_CATEGORY::ARRAY_LIST) << "Attribute is not a array/list type";

  auto attributeList = std::static_pointer_cast<AttributeList>(attribute);

  if (attributeList->is_primitive_type) {
    CHECK(destination->getType() == attributeList->simple_type->type)
        << "Type mismatch between source and destination: " << attributeList->simple_type->type
        << " != " << destination->getType();
  } else {
    // FIXME: can we check the inner-type of record_ptr
    CHECK(destination->getType() == dcds::valueType::RECORD_PTR)
        << "Destination is not of the type RECORD_PTR: " << destination->getType();
  }

  if (attributeList->is_fixed_size) {
    // FIXME: create a function for checking integral type.
    CHECK(key->getResultType() == valueType::INT64) << "Key should be integral type for fixed-sized arrays";
  } else {
    auto indexedTy = std::static_pointer_cast<AttributeIndexedList>(attributeList);

    CHECK(key->getResultType() == indexedTy->composite_type->getAttribute(indexedTy->key_attribute)->type)
        << "Mismatched key type: "
        << "Expected: " << key->getResultType()
        << " vs Input: " << indexedTy->composite_type->getAttribute(indexedTy->key_attribute)->type;
  }

  // what will be the statements then?
  //  e.g. sb->addReadStatement(rec_attribute, rec, key_arg);

  // we need the value at that index.

  // it will be translated into two statements. first is getting the pointer to actual table?

  if (attributeList->is_fixed_size) {
    // ARRAY access
    auto arrayTy = std::static_pointer_cast<AttributeArray>(attributeList);

    // $$ for array
    // get pointer to starting record, or index of starting record? and then add the index, and then do a read on that
    // statement.
    //    this->addReadStatement(attribute, add_temp_var("index_read_tmp_arTy", valueType::RECORD_PTR));
    //    this->addLogStatement("read the pointer to the actual table record\n");

    // now how to get the value?

    auto rs = new ReadIndexedStatement(attribute->name, destination, key, true);
    statements.push_back(rs);

  } else {
    // KV_MAP access
    auto indexedTy = std::static_pointer_cast<AttributeIndexedList>(attributeList);

    // $$ for indexedList
    // get the pointer to the actual index which is stored there.
    // then probe the index to get the value of it.

    // we need a name-generator.
    this->addReadStatement(attribute, add_temp_var("index_read_tmp_idxTy", valueType::RECORD_PTR));
    // now we have ptr to that index, now probe it for get.

    // how to handle if not found?
    auto rs = new ReadIndexedStatement(attribute->name, destination, key, false);
    statements.push_back(rs);
  }
}

void StatementBuilder::addUpdateStatement(const std::shared_ptr<dcds::Attribute> &attribute,
                                          const std::string &source) {
  // NOTE: Destination will always be DS-attribute, and source can be either temporary variable or function argument.

  CHECK(!(attribute->is_compile_time_constant)) << "Cannot update a compile-time constant attribute";
  CHECK(!(attribute->is_runtime_constant)) << "Cannot update a runtime constant attribute";

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
  this->parent_function._is_const = false;
}

void StatementBuilder::addUpdateStatement(const std::shared_ptr<dcds::Attribute> &attribute,
                                          const std::shared_ptr<expressions::Expression> &source) {
  CHECK(!(attribute->is_compile_time_constant)) << "Cannot update a compile-time constant attribute";
  CHECK(!(attribute->is_runtime_constant)) << "Cannot update a runtime constant attribute";

  // How do we know if expr is valid?
  // checks?
  if (source->getResultType() != attribute->type) {
    throw dcds::exceptions::dcds_invalid_type_exception("Type mismatch between attribute and source expression");
  }

  auto s = new UpdateStatement(attribute->name, source);
  statements.push_back(s);
  this->parent_function._is_const = false;
}

void StatementBuilder::addReturnStatement(const std::shared_ptr<expressions::Expression> &expr) {
  // How do we know if expr is valid?
  // checks?
  CHECK(expr->getResultType() == this->parent_function.returnValueType)
      << "Return type mismatch between return variable and declared return type in the function: "
      << this->parent_function.getName();

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
  CHECK(this->parent_function.returnValueType == dcds::valueType::VOID)
      << "Function (" << this->parent_function.getName() << ") expects non-void return";

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
  this->parent_function._is_const = false;
  return resultVar;
}

std::shared_ptr<expressions::TemporaryVariableExpression> StatementBuilder::addInsertStatement(
    const std::shared_ptr<Builder> &object_type, const std::string &variable_name) {
  return this->addInsertStatement(object_type->getName(), variable_name);
}

std::shared_ptr<expressions::LocalVariableExpression> StatementBuilder::MethodCall_getReturnDestination(
    const std::string &return_destination_variable) {
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
  }
  return return_dest;
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

  std::vector<std::shared_ptr<expressions::Expression>> arg_vars;
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
  std::shared_ptr<expressions::LocalVariableExpression> return_dest =
      MethodCall_getReturnDestination(return_destination_variable);
  if (!(return_destination_variable.empty())) {
    CHECK(return_dest->getType() == method->getReturnValueType()) << "Type mismatch for return destination.";
  }

  auto rs = new MethodCallStatement(object_type->getFunction(function_name), reference_variable, return_dest,
                                    std::move(arg_vars));
  statements.push_back(rs);
  doesHaveMethodCalls = true;
  if (object_type->getFunction(function_name)->_is_const == false) {
    this->parent_function._is_const = false;
  }
}

void StatementBuilder::addMethodCall(
    const std::shared_ptr<Builder> &object_type,
    const std::shared_ptr<dcds::expressions::LocalVariableExpression> &reference_variable,
    const std::string &function_name, const std::shared_ptr<expressions::LocalVariableExpression> &return_destination,
    const std::vector<std::shared_ptr<dcds::expressions::Expression>> &function_args) {
  CHECK(reference_variable != nullptr) << "Reference variable cannot be empty";
  CHECK((this->parent_function.hasTempVariable(reference_variable->getName()) ||
         this->parent_function.hasArgument(reference_variable->getName())))
      << "Reference variable does not exists in the scope: " + reference_variable->getName();

  CHECK(reference_variable->getType() == dcds::valueType::RECORD_PTR)
      << "Reference variable is not a type: " << reference_variable->getName();

  // FIXME: how to check if the record_ptr is of the correct type, that is, object_type!
  CHECK(object_type->hasFunction(function_name))
      << "Function (" << function_name << ") does not exists in the type: " + object_type->getName();

  auto method = object_type->getFunction(function_name);
  const auto &expected_args = method->getArguments();

  CHECK(function_args.size() == expected_args.size())
      << "number of function arguments does not matched expected number of arguments by the target function. expected: "
      << std::to_string(expected_args.size()) << " provided: " << std::to_string(function_args.size());

  auto i = 0;
  for (const auto &arg : function_args) {
    CHECK(arg->getResultType() == expected_args[i]->getType())
        << "Type mismatch for argument # " << i << " :" << arg->getResultType()
        << " != " << expected_args[i]->getType();
    i++;
  }

  if (return_destination) {
    CHECK(return_destination->getType() == method->getReturnValueType()) << "Type mismatch for return destination.";
  }

  auto rs = new MethodCallStatement(object_type->getFunction(function_name), reference_variable->getName(),
                                    return_destination, function_args);
  statements.push_back(rs);
  doesHaveMethodCalls = true;
  if (object_type->getFunction(function_name)->_is_const == false) {
    this->parent_function._is_const = false;
  }
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
    //    LOG(INFO) << "extractReadWriteSet_recursive: " << typeName << "::" << this->parent_function.getName() << ": "
    //              << stmt->stType;

    // What about dependent read-write?

    if (stmt->stType == statementType::CONDITIONAL_STATEMENT) {
      // recurse.
      auto conditional = reinterpret_cast<const ConditionalStatement *>(stmt);
      conditional->ifBlock->extractReadWriteSet_recursive(read_set, write_set);
      conditional->elseBLock->extractReadWriteSet_recursive(read_set, write_set);
    } else if (stmt->stType == statementType::READ) {
      auto readStmt = reinterpret_cast<const ReadStatement *>(stmt);
      read_set[typeName].insert(readStmt->source_attr);
    } else if (stmt->stType == statementType::READ_INDEXED) {
      auto readStmt = reinterpret_cast<const ReadIndexedStatement *>(stmt);
      read_set[typeName].insert(readStmt->source_attr + "[" + readStmt->index_expr->toString() + "]");
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
      out << "src: " << st->source_attr << ", dst: " << st->dest_expr->toString();

    } else if (s->stType == dcds::statementType::READ_INDEXED) {
      auto st = reinterpret_cast<const ReadIndexedStatement *>(s);
      out << "src: " << st->source_attr << "[" << st->index_expr->toString() << "]"
          << ", dst: " << st->dest_expr->toString();

    } else if (s->stType == statementType::UPDATE) {
      auto st = reinterpret_cast<const UpdateStatement *>(s);
      // auto st = std::static_pointer_cast<UpdateStatement>(s);
      out << "src: " << st->source_expr->toString() << ", dst: " << st->destination_attr;

    } else if (s->stType == statementType::CREATE) {
      auto st = reinterpret_cast<const InsertStatement *>(s);
      // auto st = std::static_pointer_cast<InsertStatement>(s);
      out << st->destination_var << " = " << st->type_name << "()";

    } else if (s->stType == statementType::TEMP_VAR_ASSIGN) {
      auto st = reinterpret_cast<const TempVarAssignStatement *>(s);
      out << st->dest->toString() << " = " << st->source->toString();

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
  // FIXME: this is not a clean way, if we can avoid this, that would be the best option! didn't go through copy
  // constructor route as quite doubtful on it for now given the shared_pointers.

  // do we need new name for it?
  auto ret = std::make_shared<StatementBuilder>(this->parent_function, this->parent_block, this->name);

  ret->child_blocks = this->child_blocks;
  ret->doesReturn = this->doesReturn;
  ret->doesHaveMethodCalls = this->doesHaveMethodCalls;

  // copy statements
  // what if statements within are keeping something important or shared ptr? shouldn't it call clone method of
  // statement?

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

  return ret;
}
