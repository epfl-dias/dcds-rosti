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

#ifndef DCDS_BUILDER_OPT_PASSES_HPP
#define DCDS_BUILDER_OPT_PASSES_HPP

#include <iostream>
#include <set>
#include <utility>

#include "dcds/builder/builder.hpp"
#include "dcds/builder/function-builder.hpp"
#include "dcds/builder/statement-builder.hpp"
#include "dcds/builder/statement.hpp"

namespace dcds {
class BuilderOptPasses {
 private:
  struct attribute_stat_t {
    size_t n_usage{};
    size_t n_read{};
    size_t n_write{};
    std::set<std::string> readBy_functions{};
    std::set<std::string> writeBy_functions{};
  };

  class AttributeStats {
   public:
    explicit AttributeStats(const std::shared_ptr<Builder>& _builder, bool log_stats = true);

    const auto& getAll() { return stats; }
    const auto& get(const std::string& attribute_name) {
      CHECK(stats.contains(attribute_name))
          << "Stats does not contain attribute: " << attribute_name << " in DS: " << ds_name;
      return stats[attribute_name];
    }

   private:
    const std::string ds_name;
    std::map<std::string, attribute_stat_t> stats;
  };

 public:
//  enum class BUILDER_OPT_PASS {
//    REMOVE_UNUSED_FUNCTIONS_FROM_COMPOSITE_TYPES,
//    REMOVE_UNUSED_ATTRIBUTES,
//    REMOVE_WRITE_ONLY_ATTRIBUTES,
//    FIND_DANGLING_RECORD_CREATIONS,
//  };

  explicit BuilderOptPasses(std::shared_ptr<Builder> _builder) : builder(std::move(_builder)) {}

  void runAll();

  void opt_pass_remove_unused_functions_from_composite_types(bool recursive);
  void opt_pass_removeUnusedAttributes();

 private:
  static void setParent(const std::shared_ptr<Builder>& currentBuilder);
  static void removeWriteOnlyAttribute(std::shared_ptr<Builder>& currentBuilder, const std::string& attribute_name,
                                       const BuilderOptPasses::attribute_stat_t& attributeStats);
  static bool removeAttributeStatements(const std::shared_ptr<StatementBuilder>& sb, const std::string& attribute_name);

 private:
  std::shared_ptr<Builder> builder;
};

}  // namespace dcds

#endif  // DCDS_BUILDER_OPT_PASSES_HPP
