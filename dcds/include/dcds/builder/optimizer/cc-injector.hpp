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

#ifndef DCDS_CC_INJECTOR_HPP
#define DCDS_CC_INJECTOR_HPP

#include "dcds/builder/builder.hpp"
#include "dcds/builder/function-builder.hpp"
#include "dcds/builder/statement-builder.hpp"
#include "dcds/builder/statement.hpp"

namespace dcds {

class CCInjector {
 public:
  explicit CCInjector(Builder *root_builder) : builder(root_builder) {}

  void inject();

 private:
  struct attribute_trait_t {
    bool is_nascent;
  };

  using attribute_info = std::pair<std::string, std::string>;  // typeName, attributeName
  using attribute_locks = std::map<attribute_info, LockStatement *>;
  using attribute_traits = std::map<attribute_info, attribute_trait_t>;

 private:
  void injectCC_function(std::shared_ptr<FunctionBuilder> &fb, attribute_trait_t trait = {});
  void injectCC_statementBlock(std::shared_ptr<StatementBuilder> &s, const attribute_locks &locks_in_scope,
                               attribute_trait_t type_traits, attribute_traits &traits_in_scope);

 private:
  Builder *builder;

 private:
  template <typename Iterator>
  static void placeLockIfAbsent(std::map<attribute_info, LockStatement *> &lock_placed,
                                attribute_traits &traits_in_scope, Iterator &pos, std::deque<Statement *> &statements,
                                const std::string &attribute_name, const std::string &type_name, size_t typeID,
                                bool lock_exclusive = true) {
    // FIXME: if the previous lock was shared, it has to change that to exclusive! or add an upgrade lock statement.
    // Hacked for now, making all locks exclusive.

    // lock_exclusive = true;

    attribute_info attrInfo{type_name, attribute_name};
    if (!(lock_placed.contains(attrInfo))) {
      if (traits_in_scope.contains(attrInfo)) {
        LOG(INFO) << "trait-in-scope: " << attrInfo.second;
        LOG(INFO) << "\tis_nascent: " << traits_in_scope[attrInfo].is_nascent;
        return;
      }

      auto lkSt = new LockStatement(attrInfo.first, attrInfo.second, typeID, lock_exclusive);
      pos = statements.insert(pos, reinterpret_cast<Statement *>(lkSt));

      lock_placed.emplace(attrInfo, lkSt);
      pos++;
    }
  }
};

}  // namespace dcds

#endif  // DCDS_CC_INJECTOR_HPP
