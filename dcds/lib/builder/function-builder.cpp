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

#include "dcds/builder/function-builder.hpp"

#include "dcds/builder/statement-builder.hpp"

using namespace dcds;

std::atomic<size_t> FunctionBuilder::id_src = 1;

FunctionBuilder::FunctionBuilder(dcds::Builder *ds_builder, std::string functionName)
    : FunctionBuilder(ds_builder, std::move(functionName), dcds::valueType::VOID) {}

FunctionBuilder::FunctionBuilder(dcds::Builder *ds_builder, std::string functionName, dcds::valueType returnType)
    : builder(ds_builder),
      _name(std::move(functionName)),
      returnValueType(returnType),
      function_id(id_src.fetch_add(1)) {
  // TODO: function name should not start with get_/set_

  this->entryPoint = std::make_shared<StatementBuilder>(*this);
}

std::shared_ptr<FunctionBuilder> FunctionBuilder::cloneShared(dcds::Builder *ds_builder) {
  // whats the case for other builder?
  // we need to append the id to name, and add it to function list so it also gets built!
  LOG(INFO) << "Cloning function: " << this->_name;
  auto f =
      std::make_shared<FunctionBuilder>(ds_builder ? ds_builder : this->builder, this->_name, this->returnValueType);
  f->cloned_src_id = this->function_id;
  f->_name = f->_name + "__" + std::to_string(f->function_id);

  for (const auto &fa : this->function_args) {
    f->function_args.emplace_back(std::make_shared<expressions::FunctionArgumentExpression>(*(fa)));
  }
  for (const auto &tv : this->temp_variables) {
    f->temp_variables.emplace(tv.first, std::make_shared<expressions::TemporaryVariableExpression>(*(tv.second)));
  }
  // f->entryPoint = std::make_shared<StatementBuilder>(*(this->entryPoint));
  f->entryPoint = this->entryPoint->clone_deep();
  (ds_builder ? ds_builder : this->builder)->addFunction(f);
  return f;
}

std::pair<rw_set_t, rw_set_t> FunctionBuilder::extractReadWriteSet() {
  LOG(INFO) << "extractReadSet for " << this->getName();
  rw_set_t read_set;   // <typeName, std:set<attributeName>>
  rw_set_t write_set;  // <typeName, std::set<attributeName>>

  this->entryPoint->extractReadWriteSet_recursive(read_set, write_set);

  return {read_set, write_set};
}

bool FunctionBuilder::isReadOnly() {
  auto [readSet, writeSet] = this->extractReadWriteSet();
  return writeSet.empty();
}

static void indent_p(std::ostream &out, size_t indent_level) {
  for (auto i = 0; i < indent_level; i++) {
    out << "\t";
  }
}

void FunctionBuilder::print(std::ostream &out, size_t indent_level) {
  indent_p(out, indent_level);

  out << "Function: " << this->_name << " (id: " << this->function_id << ")"
      << " (parent: " << this->cloned_src_id << ")" << std::endl;

  indent_p(out, indent_level);
  out << "\tsignature: " << this->returnValueType << " " << this->_name << "(";
  for (auto i = 0; i < this->function_args.size(); i++) {
    out << this->function_args[i]->toString();
    if (i != this->function_args.size() - 1) {
      out << ",";
    }
  }

  out << ")" << std::endl;

  if (!this->temp_variables.empty()) {
    indent_p(out, indent_level);
    out << "\ttemporary-variables: " << std::endl;
    for (auto &arg : this->temp_variables) {
      indent_p(out, indent_level);
      out << "\t\t" << arg.second->toString() << std::endl;
    }
  }

  indent_p(out, indent_level);
  out << "\tstatements:" << std::endl;

  this->entryPoint->print(out, indent_level + 2);
}