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
  using attribute_info = std::pair<std::string, std::string>;  // typeName, attributeName

  struct attribute_trait_t {
    bool is_nascent;
    bool is_const;
    attribute_info source_var;
  };

  using attribute_locks = std::map<attribute_info, LockStatement2 *>;
  using attribute_traits = std::map<attribute_info, attribute_trait_t>;

 private:
  void injectCC_function(std::shared_ptr<FunctionBuilder> &fb, attribute_trait_t trait = {});
  void injectCC_statementBlock(std::shared_ptr<StatementBuilder> &s, const attribute_locks &locks_in_scope,
                               attribute_trait_t type_traits, attribute_traits &traits_in_scope);

 private:
  Builder *builder;

 private:
  template <typename Iterator>
  static void placeLockIfAbsent(std::map<attribute_info, LockStatement2 *> &lock_placed,
                                attribute_traits &traits_in_scope, Iterator &pos, std::deque<Statement *> &statements,
                                const std::string &attribute_name, const std::string &type_name, size_t typeID,
                                bool lock_exclusive = true) {
    attribute_info attrInfo{type_name, attribute_name};
    if (!(lock_placed.contains(attrInfo))) {
      if (traits_in_scope.contains(attrInfo)) {
        //        LOG(INFO) << "trait-in-scope: " << attrInfo.second;
        //        LOG(INFO) << "\tis_nascent: " << traits_in_scope[attrInfo].is_nascent;
        //        LOG(INFO) << "\tis_const: " << traits_in_scope[attrInfo].is_const;
        if (traits_in_scope[attrInfo].is_const) {
          // LOG(INFO) << "No-need of lock as variable is a const";
          return;
        }

        if (lock_placed.contains(traits_in_scope[attrInfo].source_var)) {
          // LOG(INFO) << "source-lock already here";
          // if source var is already here, skip the lock;
          return;
        }
      }

      auto lkSt = new LockStatement2(attrInfo.first, attrInfo.second, typeID, lock_exclusive);
      pos = statements.insert(pos, reinterpret_cast<Statement *>(lkSt));

      lock_placed.emplace(attrInfo, lkSt);
      pos++;
    } else {
      // there is a lock already.
      // ==> verify the lock-compatibility.
      bool is_shared = !(lock_placed[attrInfo]->is_exclusive);

      if (lock_exclusive && is_shared) {
        // requested exclusive, however, existing one is shared
        // ==> convert to exclusive
        lock_placed[attrInfo]->is_exclusive = true;
      }
    }
  }

  friend std::ostream &operator<<(std::ostream &os, const CCInjector::attribute_trait_t &ty);
  friend std::ostream &operator<<(std::ostream &os, const CCInjector::attribute_traits &traits_in_scope);
};

inline std::ostream &operator<<(std::ostream &os, const CCInjector::attribute_trait_t &ty) {
  os << "\tis_nascent: " << ty.is_nascent << std::endl;
  os << "\tis_const: " << ty.is_const << std::endl;
  if (!ty.source_var.first.empty()) {
    os << "\tsource_var: " << ty.source_var.first << "::" << ty.source_var.second << std::endl;
  }

  return os;
}

inline std::ostream &operator<<(std::ostream &os, const CCInjector::attribute_traits &traits_in_scope) {
  for (auto &[k, v] : traits_in_scope) {
    os << "\t" << k.first << "::" << k.second << std::endl;
    ;
    os << v << std::endl;
    ;
  }
  return os;
}

}  // namespace dcds

#endif  // DCDS_CC_INJECTOR_HPP
