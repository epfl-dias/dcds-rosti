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

#include "dcds/builder/function-builder.hpp"

using namespace dcds;

void StatementBuilder::addReadStatement(const std::shared_ptr<dcds::SimpleAttribute> &attribute,
                                        const std::string &destination) {
  // Ideally this should return a valueType or something operate-able so the user knows what is the return type?
  // NOTE: Source will always be DS-attribute, and destination will be a temporary variable always.

  if (!(this->parent_function.hasAttribute(attribute))) {
    throw dcds::exceptions::dcds_dynamic_exception("Attribute not registered in the data structure");
  }

  if (!(this->parent_function.hasTempVariable(destination))) {
    throw dcds::exceptions::dcds_dynamic_exception("Function does not referenced source variable referenced: " +
                                                   destination);
  }
  auto s = std::make_shared<Statement>(dcds::statementType::READ, attribute->name, destination);
  statements.push_back(s);
}

void StatementBuilder::addUpdateStatement(const std::shared_ptr<dcds::SimpleAttribute> &attribute,
                                          const std::string &source) {
  // NOTE: Destination will always be DS-attribute, and source can be either temporary variable or function argument.

  if (!(this->parent_function.hasAttribute(attribute))) {
    throw dcds::exceptions::dcds_dynamic_exception("Attribute not registered in the data structure");
  }

  if (!(this->parent_function.hasTempVariable(source)) && !(this->parent_function.hasArgument(source))) {
    throw dcds::exceptions::dcds_dynamic_exception("Function does not referenced source variable referenced: " +
                                                   source);
  }

  auto sourceType =
      this->parent_function.hasTempVariable(source) ? VAR_SOURCE_TYPE::TEMPORARY_VARIABLE : FUNCTION_ARGUMENT;

  auto s = std::make_shared<UpdateStatement>(attribute->name, source, sourceType);
  statements.push_back(s);
}

void StatementBuilder::addReturnStatement(const std::string &var_name) {
  // The reference variable should be a temporary variable.
  // what about the case of returning a constant?

  if (!(this->parent_function.hasTempVariable(var_name))) {
    throw dcds::exceptions::dcds_dynamic_exception("Function does not contain variable referenced to be returned: " +
                                                   var_name);
  }

  if (this->parent_function.getTempVariable(var_name)->getType() != this->parent_function.getReturnValueType()) {
    throw dcds::exceptions::dcds_invalid_type_exception(
        "Return type mismatch between return variable and declared return type");
  }

  auto rs = std::make_shared<Statement>(dcds::statementType::YIELD, "", var_name);
  statements.push_back(rs);
  this->doesReturn = true;
}

void StatementBuilder::addReturnVoidStatement() {
  auto rs = std::make_shared<Statement>(dcds::statementType::YIELD, "", "");
  statements.push_back(rs);
  this->doesReturn = true;
}

void StatementBuilder::addLogStatement(const std::string &log_string) {
  auto rs = std::make_shared<LogStringStatement>(log_string);
  statements.push_back(rs);
}

void StatementBuilder::addInsertStatement(const std::string &registered_type_name, const std::string &variable_name) {
  assert(this->parent_function.hasRegisteredType(registered_type_name));

  this->parent_function.addTempVariable(variable_name, dcds::valueType::RECORD_PTR);
  // this would call the constructor of the registered_type,
  // and then assigns the returned record_reference to the variable name,

  auto rs = std::make_shared<InsertStatement>(registered_type_name, variable_name);
  statements.push_back(rs);
}

void StatementBuilder::addInsertStatement(const std::shared_ptr<Builder> &object_type,
                                          const std::string &variable_name) {
  return this->addInsertStatement(object_type->getName(), variable_name);
}

void StatementBuilder::addMethodCall(const std::shared_ptr<Builder> &object_type, const std::string &reference_variable,
                                     const std::string &function_name, const std::string &return_destination_variable) {
  this->addMethodCall(object_type, reference_variable, function_name, return_destination_variable,
                      std::vector<std::string>{});
}

void StatementBuilder::addMethodCall(const std::shared_ptr<Builder> &object_type, const std::string &reference_variable,
                                     const std::string &function_name, const std::string &return_destination_variable,
                                     const std::vector<std::string> &args) {
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

  if (referenceVarType != RECORD_PTR) {
    throw dcds::exceptions::dcds_invalid_type_exception("Reference variable is not of type RECORD_PTR ");
  }
  // FIXME: how to check if the record_ptr is of the correct type, that is, object_type!

  if (!object_type->hasFunction(function_name)) {
    throw dcds::exceptions::dcds_dynamic_exception("Function (" + function_name +
                                                   ") does not exists in the type: " + object_type->getName());
  }

  const auto &expected_args = object_type->getFunction(function_name)->getArguments();
  std::vector<std::string> arg_list;
  // filter out for the empty.
  for (const std::string &a : args) {
    if (!(a.empty())) {
      arg_list.emplace_back(a);
    }
  }

  if (arg_list.size() != expected_args.size()) {
    throw dcds::exceptions::dcds_dynamic_exception(
        "number of function arguments does not matched expected number of arguments by the target function."
        " expected: " +
        std::to_string(object_type->getFunction(function_name)->getArguments().size()) +
        " provided: " + std::to_string(arg_list.size()));
  }

  // std::vector<std::pair<std::string, dcds::VAR_SOURCE_TYPE>> arg_variables;
  std::vector<std::shared_ptr<expressions::LocalVariableExpression>> arg_vars;
  auto i = 0;
  for (const std::string &arg_name : arg_list) {
    if (!arg_name.empty()) {
      if (this->parent_function.hasTempVariable(arg_name)) {
        auto varg = this->parent_function.getTempVariable(arg_name);

        if (expected_args[i]->getType() != varg->getType()) {
          throw dcds::exceptions::dcds_invalid_type_exception("Type mismatch for argument " + arg_name);
        }

        // arg_variables.emplace_back(arg_name, VAR_SOURCE_TYPE::TEMPORARY_VARIABLE);

        arg_vars.emplace_back(std::make_shared<expressions::TemporaryVariableExpression>(*varg));
        //        arg_vars.emplace_back(varg);

      } else if (this->parent_function.hasArgument(arg_name)) {
        auto varg = this->parent_function.findArgument(arg_name)->get();
        if (expected_args[i]->getType() != varg->getType()) {
          throw dcds::exceptions::dcds_invalid_type_exception("Type mismatch for argument " + arg_name);
        }

        arg_vars.emplace_back(std::make_shared<expressions::FunctionArgumentExpression>(*varg));
        // arg_vars.emplace_back(varg);
        // arg_variables.emplace_back(arg_name, VAR_SOURCE_TYPE::FUNCTION_ARGUMENT);
      } else {
        throw dcds::exceptions::dcds_dynamic_exception(
            "Argument is neither a temporary variable, nor a function argument: " + arg_name);
      }
    }
    i++;
  }

  //  auto rs = std::make_shared<MethodCallStatement>(object_type, reference_variable, function_name,
  //                                                  return_destination_variable, std::move(arg_variables));
  auto rs = std::make_shared<MethodCallStatement>(object_type, reference_variable, function_name,
                                                  return_destination_variable, std::move(arg_vars));
  statements.push_back(rs);
  doesHaveMethodCalls = true;
}

StatementBuilder::conditional_blocks StatementBuilder::addConditionalBranch(dcds::expressions::Expression *expr) {
  assert(expr->getResultType() == valueType::BOOL);

  auto ifBranch = std::make_shared<StatementBuilder>(this->parent_function, this);
  auto elseBranch = std::make_shared<StatementBuilder>(this->parent_function, this);
  this->child_blocks++;

  child_branches.emplace_back(ifBranch, elseBranch);

  auto rs = std::make_shared<ConditionalStatement>(expr, ifBranch, elseBranch);
  statements.push_back(rs);

  return conditional_blocks{ifBranch, elseBranch};
}