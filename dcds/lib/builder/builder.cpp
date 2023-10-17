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

#include "dcds/builder/builder.hpp"

#include <utility>

#include "dcds/builder/function-builder.hpp"
#include "dcds/builder/statement-builder.hpp"
#include "dcds/codegen/codegen.hpp"
#include "dcds/codegen/llvm-codegen/llvm-codegen.hpp"
#include "dcds/exporter/jit-container.hpp"

namespace dcds {

// Attribute* Builder::getAttributeByName(const std::string &name){
//   return this->attr_map[name];
// }
//
// void Builder::add_attribute_int8(const std::string& name){
//   attributes_.emplace_back(attribute_type::INTEGER, name);
// }
//
// void Builder::add_attribute_recordPtr(const std::string& name){
//   attributes_.emplace_back(attribute_type::RECORD_ID, name);
// }

static inline void isValidFunctionName(const std::string& functionName) {
  LOG(INFO) << "[isValidFunctionName]: " + functionName;
  if (functionName.starts_with("get_") || functionName.starts_with("set_")) {
    throw dcds::exceptions::dcds_dynamic_exception("Function name starts with a reserved identifier");
  }
}

std::shared_ptr<FunctionBuilder> Builder::createFunction(const std::string& functionName) {
  //  isValidFunctionName(functionName);
  if (this->functions.contains(functionName)) {
    throw dcds::exceptions::dcds_dynamic_exception("Function with same name already exists");
  }
  auto f = std::make_shared<FunctionBuilder>(this, functionName);
  this->functions.emplace(functionName, f);
  return f;
}

std::shared_ptr<FunctionBuilder> Builder::createFunction(const std::string& functionName, dcds::valueType returnType) {
  //  isValidFunctionName(functionName);
  if (this->functions.contains(functionName)) {
    throw dcds::exceptions::dcds_dynamic_exception("Function with same name already exists");
  }
  auto f = std::make_shared<FunctionBuilder>(this, functionName, returnType);
  this->functions.emplace(functionName, f);
  return f;
}

void Builder::generateGetter(std::shared_ptr<dcds::SimpleAttribute>& attribute) {
  // NOTE: make sure that it is a simple type or for one we can actually generate.
  assert(attribute->type_category == ATTRIBUTE_TYPE_CATEGORY::SIMPLE);

  // Generates a function of name: {attribute.type} get_{attribute_name}()

  // sanity-check:
  assert(this->hasAttribute(attribute->name));

  std::string function_name = "get_" + attribute->name;
  auto fn = this->createFunction(function_name, attribute->type);

  auto stmtBuilder = fn->getStatementBuilder();
  fn->addTempVariable("tmp", attribute->type);

  stmtBuilder->addReadStatement(attribute, "tmp");
  stmtBuilder->addReturnStatement("tmp");
  fn->setAlwaysInline(true);
}

void Builder::generateGetter(const std::string& attribute_name) {
  auto attribute = this->getAttribute(attribute_name);
  this->generateGetter(attribute);
}

void Builder::generateSetter(std::shared_ptr<dcds::SimpleAttribute>& attribute) {
  // NOTE: make sure that it is a simple type or for one we can actually generate.
  assert(attribute->type_category == ATTRIBUTE_TYPE_CATEGORY::SIMPLE);

  // Generates a function of name: void set_{attribute_name}(attribute.type)

  // sanity-check:
  assert(this->hasAttribute(attribute->name));

  std::string function_name = "set_" + attribute->name;
  auto fn = this->createFunction(function_name);
  auto stmtBuilder = fn->getStatementBuilder();
  fn->addArgument("val", attribute->type);
  stmtBuilder->addUpdateStatement(attribute, "val");
  stmtBuilder->addReturnVoidStatement();
  fn->setAlwaysInline(true);
}

void Builder::generateSetter(const std::string& attribute_name) {
  auto attribute = this->getAttribute(attribute_name);
  this->generateSetter(attribute);
}

void Builder::build_no_jit() {
  LOG(INFO) << "Generating \"" << this->getName() << "\" data structure";
  LOG(INFO) << "\tMulti-threaded: " << (is_multi_threaded ? "YES" : "NO");

  // FIXME: for composed types, this should cascade also.

  // FIXME:
  //  if(!codegen_engine){
  //    codegen_engine = context->getCodegenEngine();
  //  }

  if (!registered_subtypes.empty()) {
    LOG(INFO) << "Building sub-types first";
    // if there are registered subtypes, should be built them separate, or as you go?
    for (auto& rt : registered_subtypes) {
      rt.second->context = context;
      assert(rt.second.get());
      codegen_engine->build(rt.second.get());
    }
  }

  codegen_engine->build(this);
}

void Builder::build() {
  if (is_jit_generated) {
    throw dcds::exceptions::dcds_dynamic_exception("Data structure is already built");
  }

  if (!codegen_engine) {
    codegen_engine = std::make_shared<LLVMCodegen>(this);
  }

  this->build_no_jit();
  //  codegen_engine->build(this);

  LOG(INFO) << "[Builder::build()] -- jit-before";
  codegen_engine->jitCompileAndLoad();
  LOG(INFO) << "[Builder::build()] -- jit-after";

  this->is_jit_generated = true;
}

JitContainer* Builder::createInstance() {
  if (!is_jit_generated) {
    throw dcds::exceptions::dcds_dynamic_exception("Data structure is not built yet");
  }

  LOG(INFO) << "[Builder::createInstance] creating instance: " << this->getName();
  auto* ds_constructor = codegen_engine->getFunction(this->getName() + "_constructor");

  auto* ds_instance = reinterpret_cast<void* (*)()>(ds_constructor)();
  //  auto* ins = reinterpret_cast<JitContainer*>(ds_instance);
  auto* ins = new JitContainer(reinterpret_cast<dcds::JitContainer::dcds_jit_container_t*>(ds_instance));
  ins->setCodegenEngine(this->codegen_engine);
  return ins;
}

std::shared_ptr<Builder> Builder::clone(std::string name) {
  auto tmp = std::make_shared<dcds::Builder>(this->context, std::move(name));

  for (auto& a : attributes) {
    tmp->attributes.emplace(a.first, std::make_shared<SimpleAttribute>(*(a.second)));
  }

  for (auto& rt : registered_subtypes) {
    tmp->registered_subtypes.emplace(rt.first, rt.second);
  }

  for (auto& f : functions) {
    tmp->functions.emplace(f.first, f.second->cloneShared(tmp.get()));
  }

  return tmp;
}

}  // namespace dcds
