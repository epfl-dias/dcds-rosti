//
// Created by Aunn Raza on 18.04.23.
//

#ifndef DCDS_BUILDER_HPP
#define DCDS_BUILDER_HPP

#include <cassert>
#include <deque>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

#include "dcds/builder/attribute.hpp"
#include "dcds/builder/function-builder.hpp"
#include "dcds/common/types.hpp"
#include "statement.hpp"

namespace dcds {

class Builder {
 public:
  explicit Builder(std::string dataStructure_name) : ctx_name(std::move(dataStructure_name)) {}

  auto getName() { return ctx_name; }

  // FunctionBuilder ------- BEGIN

 public:
  static auto createFunction(const std::string& functionName) {
    return std::make_shared<FunctionBuilder>(functionName);
  }
  static auto createFunction(const std::string& functionName, dcds::valueType returnType) {
    return std::make_shared<FunctionBuilder>(functionName, returnType);
  }

  static auto createFunction(const std::string& functionName, dcds::valueType returnType,
                             std::same_as<dcds::valueType> auto... args) {
    return std::make_shared<FunctionBuilder>(functionName, returnType, args...);
  }

  static auto createFunction(const std::string& functionName, std::same_as<dcds::valueType> auto... args) {
    return std::make_shared<FunctionBuilder>(functionName, args...);
  }

  void addFunction(const std::shared_ptr<FunctionBuilder>& function) {
    assert(function->isFinal());
    assert(functions.contains(function->getName()) == false);
    functions.emplace(function->getName(), function);
  }
  auto getFunction(std::string functionName) { return functions[functionName]; }

 private:
  std::unordered_map<std::string, std::shared_ptr<FunctionBuilder>> functions;

  // FunctionBuilder ------- END

  // Attributes ------- BEGIN

 public:
  auto getAttribute(const std::string& attribute_name) { return attributes[attribute_name]; }

 private:
  std::unordered_map<std::string, std::shared_ptr<Attribute>> attributes;
  // std::deque<Attribute> attributes;

  // Attributes ------- END






  // Add Structure (tables essentially)
  // Add Attributes to that structure

  // Add statements

  // Utilities to include attributes to the data structure
 public:
  void add_attribute_int8(const std::string& name);

  void add_attribute_recordPtr(const std::string& name);

  Attribute* getAttributeByName(const std::string& name);

  // std::vector<Statement> statements;

 private:
  const std::string ctx_name;
};

}  // namespace dcds

#endif  // DCDS_BUILDER_HPP
