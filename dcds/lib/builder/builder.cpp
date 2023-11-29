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
#include "dcds/builder/optimizer/cc-injector.hpp"
#include "dcds/builder/statement-builder.hpp"
#include "dcds/codegen/codegen.hpp"
#include "dcds/codegen/llvm-codegen/llvm-codegen.hpp"
#include "dcds/exporter/jit-container.hpp"

namespace dcds {

std::atomic<size_t> Builder::type_id_src = 0;

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
void Builder::addFunction(std::shared_ptr<FunctionBuilder>& f) {
  if (this->functions.contains(f->getName())) {
    throw dcds::exceptions::dcds_dynamic_exception("Function with same name already exists");
  }
  assert(f->builder->getTypeID() == this->getTypeID());
  this->functions.emplace(f->getName(), f);
}

void Builder::generateGetter(const std::shared_ptr<dcds::SimpleAttribute>& attribute) {
  // NOTE: make sure that it is a simple type or for one we can actually generate.
  assert(attribute->type_category == ATTRIBUTE_TYPE_CATEGORY::PRIMITIVE);

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
  // NOTE: make sure that it is a simple type or for one we can actually generate.
  this->generateGetter(std::static_pointer_cast<SimpleAttribute>(attribute));
}

void Builder::generateSetter(const std::shared_ptr<dcds::SimpleAttribute>& attribute) {
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
  assert(attribute->type_category == ATTRIBUTE_TYPE_CATEGORY::PRIMITIVE);
  // NOTE: make sure that it is a simple type or for one we can actually generate.
  this->generateSetter(std::static_pointer_cast<SimpleAttribute>(attribute));
}

void Builder::build_no_jit(std::shared_ptr<Codegen>& engine, bool is_nested_type) {
  // FIXME: do we need this to clone the sub-type builders?

  if (!registered_subtypes.empty()) {
    for (auto& rt : registered_subtypes) {
      // LOG(INFO) << "Building sub-type: " << rt.first;
      rt.second->context = context;
      assert(rt.second);
      rt.second->build_no_jit(engine, true);
    }
  }

  // after all the syb types have been built, build this.
  engine->build(this, is_nested_type);
}

void Builder::build() {
  //  LOG(INFO) << "Generating \"" << this->getName() << "\" data structure";
  //  LOG(INFO) << "\tMulti-threaded: " << (is_multi_threaded ? "YES" : "NO");

  if (is_jit_generated) {
    throw dcds::exceptions::dcds_dynamic_exception("Data structure is already built");
  }

  if (!codegen_engine) {
    codegen_engine = std::make_shared<LLVMCodegen>(this);
  }

  this->build_no_jit(codegen_engine, false);

  // LOG(INFO) << "[Builder::build()] -- jit-before";
  codegen_engine->jitCompileAndLoad();
  // LOG(INFO) << "[Builder::build()] -- jit-after";

  this->is_jit_generated = true;
}

JitContainer* Builder::createInstance() {
  if (!is_jit_generated) {
    throw dcds::exceptions::dcds_dynamic_exception("Data structure is not built yet");
  }

  // LOG(INFO) << "[Builder::createInstance] creating instance: " << this->getName();
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
    tmp->attributes.emplace(a.first, std::make_shared<Attribute>(*(a.second)));
  }

  for (auto& rt : registered_subtypes) {
    tmp->registered_subtypes.emplace(rt.first, rt.second);
  }

  for (auto& f : functions) {
    tmp->functions.emplace(f.first, f.second->cloneShared(tmp.get()));
  }

  return tmp;
}

void Builder::injectCC() {
  if (!(this->cc_injector)) {
    this->cc_injector = std::make_shared<CCInjector>(this);
  }

  // TODO: check this is called only once. could be done during build instead of providing separate function.
  this->cc_injector->inject();
}

void Builder::dump() {
  for (auto& f : functions) {
    f.second->print(std::cout, 0);
    std::cout << "\n\n";
  }
}

}  // namespace dcds
