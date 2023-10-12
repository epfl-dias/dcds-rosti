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

#include "dcds/builder/function-builder.hpp"
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
  // FIXME: make sure that it is a simple type or for one we can actually generate.

  // Generates a function of name: {attribute.type} get_{attribute_name}()

  // sanity-check:
  assert(this->hasAttribute(attribute->name));

  std::string function_name = "get_" + attribute->name;
  auto fn = this->createFunction(function_name, attribute->type);

  fn->addTempVariable("tmp", attribute->type);
  fn->addReadStatement(attribute, "tmp");
  fn->addReturnStatement("tmp");
}

void Builder::generateGetter(const std::string& attribute_name) {
  auto attribute = this->getAttribute(attribute_name);
  this->generateGetter(attribute);
}

void Builder::generateSetter(std::shared_ptr<dcds::SimpleAttribute>& attribute) {
  // FIXME: make sure that it is a simple type or for one we can actually generate.

  // Generates a function of name: void set_{attribute_name}(attribute.type)

  // sanity-check:
  assert(this->hasAttribute(attribute->name));

  std::string function_name = "set_" + attribute->name;
  auto fn = this->createFunction(function_name);
  fn->addArgument("val", attribute->type);
  fn->addUpdateStatement(attribute, "val");
  fn->addReturnVoidStatement();
}

void Builder::generateSetter(const std::string& attribute_name) {
  auto attribute = this->getAttribute(attribute_name);
  this->generateSetter(attribute);
}

void Builder::build() {
  if (is_jit_generated) {
    throw dcds::exceptions::dcds_dynamic_exception("Data structure is already built");
  }

  // TO BE FIXED LATER
  //  if(!codegen_engine){
  //    codegen_engine = context->getCodegenEngine();
  //  }

  if (!codegen_engine) {
    codegen_engine = std::make_shared<LLVMCodegen>(*this);
  }

  codegen_engine->build();

  LOG(INFO) << "[Builder::build()] -- jit-before";
  codegen_engine->jitCompileAndLoad();
  LOG(INFO) << "[Builder::build()] -- jit-after";

  this->is_jit_generated = true;
}

JitContainer* Builder::createInstance() {
  if (!is_jit_generated) {
    throw dcds::exceptions::dcds_dynamic_exception("Data structure is not built yet");
  }

  LOG(INFO) << "[Builder::createInstance] creating instance";
  auto* ds_constructor = codegen_engine->getFunction(this->getName() + "_constructor");
  auto* ds_instance = reinterpret_cast<void* (*)()>(ds_constructor)();
  auto* ins = reinterpret_cast<JitContainer*>(ds_instance);
  ins->setCodegenEngine(this->codegen_engine);

  return ins;
}

}  // namespace dcds
