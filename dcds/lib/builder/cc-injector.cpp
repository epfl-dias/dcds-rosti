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

#include "dcds/builder/optimizer/cc-injector.hpp"

using namespace dcds;

// TODO: can also inject early release of locks if we certainly know that the DS
//  attribute is not gonna be needed and there is no side-effect.

void CCInjector::inject() {
  LOG(INFO) << "[CCInjector::inject] begin";
  // assuming builder-opt has run already, and remove unused variables, etc. etc.

  // 1) first create a list of attributes which actually requires CC, that is, used across different DS. but this can
  // cause issue as [head, tail] are in the same record, if two records write these together, won't it be an issue? no
  // idea?

  for (auto &[f_name, fptr] : builder->functions) {
    LOG(INFO) << "Injecting CC in function: " << f_name;
    this->injectCC_function(fptr);
  }

  LOG(INFO) << "[CCInjector::inject] end";
}

void CCInjector::injectCC_statementBlock(std::shared_ptr<StatementBuilder> &s, const attribute_locks &locks_in_scope,
                                         attribute_trait_t type_traits, attribute_traits &traits_in_scope) {
  std::map<attribute_info, LockStatement *> lock_placed(locks_in_scope);

  auto typeName = s->getFunction()->builder->getName();
  auto typeId = s->getFunction()->builder->getTypeID();

  auto it = s->statements.begin();
  while (it != s->statements.end()) {
    auto *st = (*it);

    if (st->stType == statementType::READ) {
      auto rd_st = reinterpret_cast<ReadStatement *>(st);
      if (!type_traits.is_nascent)
        placeLockIfAbsent(lock_placed, traits_in_scope, it, s->statements, rd_st->source_attr, typeName, typeId, false);

    } else if (st->stType == statementType::UPDATE) {
      auto upd_st = reinterpret_cast<UpdateStatement *>(st);
      if (!type_traits.is_nascent)
        placeLockIfAbsent(lock_placed, traits_in_scope, it, s->statements, upd_st->destination_attr, typeName, typeId,
                          true);

      attribute_info x{typeName, upd_st->destination_attr};
      if (traits_in_scope.contains(x)) {
        // as soon as we wrote this variable somewhere else, we don't know about nascence.
        traits_in_scope[x].is_nascent = false;
      }

    } else if (st->stType == statementType::METHOD_CALL) {
      auto mc_st = reinterpret_cast<MethodCallStatement *>(st);
      mc_st->function_instance = mc_st->function_instance->cloneShared();
      // new traits actually here, as we are going deeper?
      // also, pass if reference variable is nascent, then no need to do CC inside.
      if (!type_traits.is_nascent) {
        placeLockIfAbsent(lock_placed, traits_in_scope, it, s->statements, mc_st->referenced_type_variable,
                          mc_st->function_instance->builder->getName(), mc_st->function_instance->builder->getTypeID(),
                          true);
      }

      attribute_info x{mc_st->function_instance->builder->getName(), mc_st->referenced_type_variable};
      if (traits_in_scope.contains(x)) {
        this->injectCC_function(mc_st->function_instance, traits_in_scope[x]);
      } else {
        this->injectCC_function(mc_st->function_instance);
      }

    } else if (st->stType == statementType::CONDITIONAL_STATEMENT) {
      auto cnd_st = reinterpret_cast<ConditionalStatement *>(st);
      auto ifBlock = cnd_st->ifBlock;

      injectCC_statementBlock(ifBlock, lock_placed, type_traits, traits_in_scope);
      if (cnd_st->elseBLock && !(cnd_st->elseBLock->statements.empty())) {
        auto elseBlock = cnd_st->elseBLock;
        injectCC_statementBlock(elseBlock, lock_placed, type_traits, traits_in_scope);
      }
    } else if (st->stType == statementType::CREATE) {
      auto ins_st = reinterpret_cast<InsertStatement *>(st);
      auto t = traits_in_scope.emplace(std::pair{ins_st->type_name, ins_st->destination_var}, attribute_trait_t{});
      t.first->second.is_nascent = true;
    }

    ++it;  // move on to next statement
  }
}

void CCInjector::injectCC_function(std::shared_ptr<FunctionBuilder> &fb, attribute_trait_t type_trait) {
  LOG(INFO) << "[CCInjector::injectCC_function] begin: " << fb->getName() << " id: " << fb->function_id;
  fb->print(std::cout, 0);

  //  auto [readSet, writeSet] = fb->extractReadWriteSet();
  //  LOG(INFO) << "ReadSet:";
  //  for (auto &[k, v] : readSet) {
  //    LOG(INFO) << "\t\tType: " << k << " | attributes: " << joinString(v);
  //  }
  //  LOG(INFO) << "WriteSet:";
  //  for (auto &[k, v] : writeSet) {
  //    LOG(INFO) << "\t\tType: " << k << " | attributes: " << joinString(v);
  //  }

  attribute_locks lock_placed;
  attribute_traits traits;

  injectCC_statementBlock(fb->entryPoint, lock_placed, type_trait, traits);

  LOG(INFO) << "[CCInjector::injectCC_function] ##################: " << fb->getName();
  LOG(INFO) << "[CCInjector::injectCC_function] ##################";
  LOG(INFO) << "[CCInjector::injectCC_function] ##################";

  fb->print(std::cout, 0);

  LOG(INFO) << "[CCInjector::injectCC_function]: " << fb->getName();
}