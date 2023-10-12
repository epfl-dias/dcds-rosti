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

#ifndef DCDS_BUILDER_HPP
#define DCDS_BUILDER_HPP

#include <cassert>
#include <deque>
#include <iostream>
#include <map>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "dcds/builder/attribute.hpp"
#include "dcds/builder/hints/builder-hints.hpp"
#include "dcds/builder/statement.hpp"
#include "dcds/common/exceptions/exception.hpp"
#include "dcds/common/types.hpp"
#include "dcds/context/DCDSContext.hpp"

namespace dcds {

// forward declaration
class FunctionBuilder;
class Codegen;
class LLVMCodegen;
class JitContainer;

/// DCDS Frontend class
class Builder : remove_copy {
  friend class Codegen;
  friend class LLVMCodegen;

 public:
  ///
  /// \param builderContext        DCDS Context for this builder
  /// \param name    Name of the data structure for this builder
  explicit Builder(std::shared_ptr<dcds::DCDSContext>& builderContext, std::string name)
      : context(builderContext), dataStructureName(std::move(name)) {}

  ///
  /// \param name    Name of the data structure for this builder
  explicit Builder(std::string name) : context({}), dataStructureName(std::move(name)) {}

  ///
  /// \return Name of the data structure for this builder
  auto getName() { return dataStructureName; }

  ///
  /// \return DCDS context for this builder
  auto getContext() { return context; }

  ///
  /// \param functionName  Name of the function to be created
  /// \return              DCDS function representation
  std::shared_ptr<FunctionBuilder> createFunction(const std::string& functionName);

  ///
  /// \param functionName  Name of the function to be created
  /// \param returnType    Return type of the function to be created
  /// \return              DCDS function representation
  std::shared_ptr<FunctionBuilder> createFunction(const std::string& functionName, dcds::valueType returnType);

  //  void createCallStatement(tmpHead, "set_next", newNode)

  ///
  /// \param functionName  Name of the function to be created
  /// \param returnType    Return type of the function to be created
  /// \param argsTypes     Argument types for the function to be created
  /// \return              DCDS function representation
  //  static auto createFunction(const std::string& functionName, dcds::valueType returnType,
  //                             std::same_as<dcds::valueType> auto... argsTypes) {
  //    return std::make_shared<FunctionBuilder>(functionName, returnType, argsTypes...);
  //  }

  ///
  /// \param functionName  Name of the function to be created
  /// \param argsTypes     Argument types of the function to be created
  /// \return              DCDS function representation
  //  static auto createFunction(const std::string& functionName, std::same_as<dcds::valueType> auto... args) {
  //    return std::make_shared<FunctionBuilder>(functionName, args...);
  //  }

  ///
  /// \param function      DCDS function representation
  //  void addFunction(const std::shared_ptr<FunctionBuilder>& function) {
  //    // TODO: Add asserts for checking if the function is in a proper state before the codegen starts.
  //    assert(functions.contains(function->getName()) == false);
  //    functions.emplace(function->getName(), function);
  //  }

  ///
  /// \param functionName  Name of the function to be returned
  /// \return              DCDS function representation
  //  auto getFunction(std::string functionName) { return *functions[functionName]; }

 private:
  // Utilities to include attributes to the data structure
 public:
  ///
  /// \param attribute_name    Name of the attribute to be returned
  /// \return                  DCDS attributes representation
  auto getAttribute(const std::string& attribute_name) { return attributes[attribute_name]; }
  auto getAttributeIndex(const std::string& attribute_name) {
    assert(attributes.contains(attribute_name) && "how come index for un-registered attribute");
    return std::distance(std::begin(attributes), attributes.find(attribute_name));
  }

  ///
  /// \param name       Name of the attribute to be added to the data structure
  /// \param attrType   Type of the attribute to be added to the data structure
  auto addAttribute(const std::string& name, dcds::valueType attrType, const std::any& default_value) {
    auto pt = std::make_shared<dcds::SimpleAttribute>(name, attrType, default_value);
    attributes.emplace(name, pt);
    return pt;
  }
  bool hasAttribute(const std::string& name) { return attributes.contains(name); }
  bool hasAttribute(const std::shared_ptr<dcds::SimpleAttribute>& attribute) {
    return this->hasAttribute(attribute->name);
  }

  void generateGetter(const std::string& attribute_name);
  void generateSetter(const std::string& attribute_name);

  void generateGetter(std::shared_ptr<dcds::SimpleAttribute>& attribute);
  void generateSetter(std::shared_ptr<dcds::SimpleAttribute>& attribute);

  // Composability: Types

  std::shared_ptr<Builder> createType(const std::string& name) {
    if (registered_subtypes.contains(name)) {
      throw dcds::exceptions::dcds_dynamic_exception("Type name already exists");
    }
    auto nt = std::make_shared<Builder>(context, name);
    registered_subtypes.emplace(name, nt);
    return nt;
  }
  std::shared_ptr<Builder> registerType(std::shared_ptr<Builder> other) {
    if (registered_subtypes.contains(other->getName())) {
      throw dcds::exceptions::dcds_dynamic_exception("Type name already exists");
    }

    registered_subtypes.emplace(other->getName(), other);
    return other;
  }
  auto getType(const std::string& name) {
    if (!(registered_subtypes.contains(name))) {
      throw dcds::exceptions::dcds_dynamic_exception("Unknown/Unregistered type: " + name);
    }
    return registered_subtypes[name];
  }

  // CODEGEN
 public:
  void build();
  JitContainer* createInstance();

  void addHint(hints::BuilderHints hint) {
    switch (hint) {
      case hints::SINGLE_THREADED:
        is_multi_threaded = false;
        break;
      case hints::MULTI_THREADED:
        is_multi_threaded = true;
        break;
    }
  }

  std::shared_ptr<Builder> clone(std::string name);

 private:
  std::shared_ptr<DCDSContext> context;
  const std::string dataStructureName;

 private:
  std::map<std::string, std::shared_ptr<SimpleAttribute>> attributes;
  std::unordered_map<std::string, std::shared_ptr<FunctionBuilder>> functions;
  std::map<std::string, std::shared_ptr<Builder>> registered_subtypes;

 private:
  bool is_jit_generated = false;
  std::shared_ptr<Codegen> codegen_engine;

 private:
  bool is_multi_threaded = true;
};

}  // namespace dcds

#endif  // DCDS_BUILDER_HPP
