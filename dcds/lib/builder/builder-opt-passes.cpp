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

#include "dcds/builder/optimizer/builder-opt-passes.hpp"

#include <set>

#include "dcds/builder/statement.hpp"

using namespace dcds;

void BuilderOptPasses::opt_pass_remove_unused_functions_from_composite_types(bool recursive) {
  if (builder->registered_subtypes.empty()) return;

  LOG(INFO) << "PASS: opt_pass_remove_unused_functions_from_composite_types: " << builder->getName();

  std::map<std::string, std::set<std::string>> type_fn_usage;
  for (auto& t : builder->registered_subtypes) {
    type_fn_usage.emplace(t.first, std::set<std::string>{});
  }

  builder->for_each_function([&](const std::shared_ptr<FunctionBuilder>& fb) {
    fb->entryPoint->for_each_statement([&](const Statement* stmt) {
      switch (stmt->stType) {
        case statementType::METHOD_CALL: {
          auto methodCall = reinterpret_cast<const MethodCallStatement*>(stmt);
          auto typeName = methodCall->function_instance->builder->getName();
          CHECK(type_fn_usage.contains(typeName))
              << "Method call to an unregistered type in builder? builder: " << builder->getName()
              << ", unregistered_type: " << methodCall->function_instance->builder->getName();
          type_fn_usage[typeName].insert(methodCall->function_instance->getName());
          break;
        }
        case statementType::CONDITIONAL_STATEMENT:
        case statementType::CREATE:  // insert means constructor, but that is not exposed as function.
        case statementType::READ:
        case statementType::UPDATE:
        case statementType::YIELD:
        case statementType::LOG_STRING:
        case statementType::TEMP_VAR_ASSIGN:
        case statementType::READ_INDEXED:
          break;

        case statementType::CC_LOCK_SHARED:
        case statementType::CC_LOCK_EXCLUSIVE:
          break;
      }
    });
  });

  LOG(INFO) << "functions used in " << builder->getName();
  for (auto& [type, fUsage] : type_fn_usage) {
    LOG(INFO) << "Type: " << type;
    for (auto& fn : fUsage) {
      LOG(INFO) << "\t" << fn;
    }
  }
  LOG(INFO) << "Cleaning unused functions now.";

  // FIXME: BIG TODO: clone the builder before cleanup. and also freeze/finalize the builder.

  for (auto& t : builder->registered_subtypes) {
    t.second->dropAllFunctionsExceptList(type_fn_usage[t.first]);
  }

  if (recursive) {
    for (auto& t : builder->registered_subtypes) {
      BuilderOptPasses ty(t.second);
      ty.opt_pass_remove_unused_functions_from_composite_types(true);
    }
  }
}
BuilderOptPasses::AttributeStats::AttributeStats(const std::shared_ptr<Builder>& _builder, bool log_stats)
    : ds_name(_builder->getName()) {
  if (_builder->attributes.empty()) return;

  for (auto& [name, attr] : _builder->attributes) {
    stats.emplace(name, attribute_stat_t{});
  }

  _builder->for_each_function([&](const std::shared_ptr<FunctionBuilder>& fb) {
    fb->entryPoint->for_each_statement([&](const Statement* stmt) {
      switch (stmt->stType) {
        case statementType::READ: {
          auto readStmt = reinterpret_cast<const ReadStatement*>(stmt);
          auto action_attribute = readStmt->source_attr;
          assert(stats.contains(action_attribute));
          stats[action_attribute].n_usage++;
          stats[action_attribute].n_read++;
          stats[action_attribute].readBy_functions.insert(fb->getName());
          break;
        }
        case statementType::UPDATE: {
          auto updStmt = reinterpret_cast<const UpdateStatement*>(stmt);
          auto action_attribute = updStmt->destination_attr;
          assert(stats.contains(action_attribute));
          stats[action_attribute].n_usage++;
          stats[action_attribute].n_write++;
          stats[action_attribute].writeBy_functions.insert(fb->getName());
          break;
        }

        case statementType::CREATE:  // insert means constructor, but that is not exposed as function.
        // ??
        case statementType::METHOD_CALL:
        case statementType::CONDITIONAL_STATEMENT:
        case statementType::YIELD:
        case statementType::LOG_STRING:
        case statementType::TEMP_VAR_ASSIGN:
        case statementType::READ_INDEXED:
          break;
        case statementType::CC_LOCK_SHARED:
        case statementType::CC_LOCK_EXCLUSIVE:
          break;
      }
    });
  });

  if (log_stats) {
    LOG(INFO) << "Attribute stats for " << _builder->getName();
    for (auto& [name, st] : this->stats) {
      LOG(INFO) << "Attribute: " << name << " | usage_count: " << st.n_usage;
      LOG(INFO) << "\tread_count: " << st.n_read;
      if (st.n_read > 0) {
        LOG(INFO) << "\tread-by: " << joinString(st.readBy_functions);
      }

      LOG(INFO) << "\twrite_count: " << st.n_write;
      if (st.n_write > 0) {
        LOG(INFO) << "\twrite-by: " << joinString(st.writeBy_functions);
      }
    }
  }
}

void BuilderOptPasses::opt_pass_removeUnusedAttributes() {
  LOG(INFO) << "BuilderOptPasses::opt_pass_removeUnusedAttributes: " << builder->getName();
  AttributeStats attribute_stats(builder, true);

  // DONE: 1- remove the attribute which is absolutely not used.
  // TODO: 2- remove writeOnly attributes but be careful as removing statement may cause domino effect.
  //  also, if it is composite, make sure it is correct. as set_next is called on LL from outside, it shows 'next'
  //  write_only, but you cannot remove it semantically. or can you?
  //  i think we can because we dont have pop function, so what matters is the head actually, the other next does not.
  //  which gives rise to another opt/issue: dangling records. we know we can remove the next attribute entirely as
  //  nobody is reading it at the moment, but when we remove set_next from statements, then whenever somebody calls
  //  push_front, it replace the head with new record, what about the old one? how to find the dangling/orphaned one?

  // insert followed (maybe few steps after) by an update statement means that newly created one is actually used.

  // naive idea: do reference counting.

  // also, if we know there is one to one relation, can we compose like really combine the two DS?
  // like we currently have push_front and front functions only, which in reality means the DS is actually BS,
  // and in its correct optimized form, it should just be single attribute 'payload', and push_front updates it,
  // and front returns it.
  // How can we reach from the current spec to this one ideally/formally?

  // for the write-only, it would be not top-down, it will be bottom up.
  // for example, we remove next attribute, then we remove the statement where it was being written,
  // and if that statement block becomes empty we remove that: if that is function's main block, we remove function,
  // and if it is only statement in a conditional branch, we remove that branch.
  // in case it causes the function to be removed, then we bottom-up recursive remove the function call, and
  // do the statement check upwards again.

  // what about read-only attributes, convert them to constant/non-attribute?

  for (auto& [name, attr] : builder->attributes) {
    const auto& stats = attribute_stats.get(name);
    if (stats.n_usage == 0) {
      // remove absolutely unused attributes.
      builder->removeAttribute(name);
    }

    // FIXME: for now, just remove unused attributes only

    //    if (stats.n_read == 0 && stats.n_write > 0) {
    //      // write only attribute.
    //      // remove attribute.
    //      // remove any update calls to it.
    //      // see it causes a function to be removed. if yes, then return it.
    //      // the callee should do the statement thing recursively
    //
    //      BuilderOptPasses::removeWriteOnlyAttribute(builder, name, stats);
    //    }
  }
}

void BuilderOptPasses::setParent(const std::shared_ptr<Builder>& currentBuilder) {
  for (auto& t : currentBuilder->registered_subtypes) {
    t.second->parentType = t.second;
    setParent(t.second);
  }
}

bool BuilderOptPasses::removeAttributeStatements(const std::shared_ptr<StatementBuilder>& sb,
                                                 const std::string& attribute_name) {
  return false;
  // Scratchpad, TODO: implement & fix.
  //
  //  // returns if the block is empty now.
  //
  //  // Caveat: if the update is actually linked to previous insert, then insert is a dangling one now.
  //
  //  std::deque<std::shared_ptr<Statement>> removedStatements;
  //
  //  std::erase_if(sb->statements, [&](const std::shared_ptr<Statement>& s) {
  //    if (s->stType == statementType::UPDATE &&
  //        std::static_pointer_cast<UpdateStatement>(s)->destination_attr == attribute_name) {
  //      // blindly remove it? or save it like pos, or
  //
  //      removedStatements.emplace_back(s);
  //      return true;
  //    } else {
  //      return false;
  //    }
  //  });
  //
  //  for (const auto& s : sb->statements) {
  //    if (s->stType == statementType::CONDITIONAL_STATEMENT) {
  //      auto conditional = std::static_pointer_cast<ConditionalStatement>(s);
  //      // now there are two returns here!
  //      auto toRemoveIfBlock = removeAttributeStatements(conditional->ifBlock, attribute_name);
  //      auto toRemoveElseBlock = removeAttributeStatements(conditional->elseBLock, attribute_name);
  //      LOG(INFO) << "toRemoveIfBlock: " << toRemoveIfBlock;
  //      LOG(INFO) << "toRemoveElseBlock: " << toRemoveElseBlock;
  //
  //      // removing else block does not matter, as it will not create the else branch essentially.
  //      // or for that matter, empty if would be optimized away, no?
  //
  //      // what about leading to an empty funtion?
  //    }
  //  }
  //
  //  LOG(INFO) << "SB SIZE: " << sb->statements.size();
  //
  //  // e.g.: for set_next, there is only one statement remaining, that is, returnVoid. so this function can be
  //  removed.
  //  // but do we want to? clang would do it for us or no?
  //
  //  // what about finding dangling records?
  //
  //  return sb->statements.empty();  // return true if block is empty now.
}

void BuilderOptPasses::removeWriteOnlyAttribute(std::shared_ptr<Builder>& currentBuilder,
                                                const std::string& attribute_name,
                                                const BuilderOptPasses::attribute_stat_t& stats) {
  // Scratchpad, TODO: implement & fix.
  /*
  assert(stats.n_usage > 0);
  assert(stats.n_read == 0);
  assert(stats.n_write > 0);

  LOG(INFO) << "removeAttribute: " << attribute_name;
  // 1- remove the attribute
  assert(currentBuilder->hasAttribute(attribute_name));
  currentBuilder->removeAttribute(attribute_name);

  // 2- remove any method calls

  for (auto& fName : stats.writeBy_functions) {
    auto fn = currentBuilder->getFunction(fName);  // this function uses it.

    auto x = removeAttributeStatements(fn->entryPoint, attribute_name);
    LOG(INFO) << "X is " << x;
    // then what? is x is true, then we should remove this function as this is empty essentially?
  }

  currentBuilder->for_each_function([&](const std::shared_ptr<FunctionBuilder>& fb) {
    fb->entryPoint->for_each_statement([&](const std::shared_ptr<Statement>& stmt) {
      if (stmt->stType == statementType::UPDATE &&
          std::static_pointer_cast<UpdateStatement>(stmt)->destination_attr == attribute_name) {
      }
    });
  });
  */
}

void BuilderOptPasses::runAll() {
  LOG(INFO) << "BuilderOptPasses::runAll: " << builder->getName();
  // first set the parent.
  setParent(builder);

  // NOTE: order matters
  opt_pass_remove_unused_functions_from_composite_types(true);

  for (auto& t : builder->registered_subtypes) {
    BuilderOptPasses ty(t.second);
    ty.opt_pass_removeUnusedAttributes();
  }

  this->opt_pass_removeUnusedAttributes();
}