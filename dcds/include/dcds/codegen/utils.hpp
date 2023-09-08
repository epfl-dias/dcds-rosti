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

//
// Created by prathamesh on 27/7/23.
//

#ifndef DCDS_CODEGEN_UTILS_HPP
#define DCDS_CODEGEN_UTILS_HPP

#include <llvm/Support/Error.h>
#include <llvm/Support/raw_ostream.h>

namespace dcds {
namespace llvmutil {
template <typename ReturnType, typename... Arguments>
class function_ref {
  std::string name_;
  llvm::Function *function_;

 public:
  explicit function_ref(std::string const &name, llvm::Function *fn) : name_(name), function_(fn) {}

  operator llvm::Function *() const { return function_; }

  std::string const &getName() const { return name_; }
};

template <typename ReturnType, typename... Arguments>
class function_ref_creator {
 public:
  function_ref<ReturnType, Arguments...> operator()(std::string const &name, llvm::Function *fn) {
    return function_ref<ReturnType, Arguments...>{name, fn};
  }
};

template <typename T>
T unwrap(llvm::Expected<T> value) {
  if (!value) {
    std::cout << "Error encountered :'(\n";
  }
  return std::move(*value);
}

static void CreateIfElseBlocks(std::unique_ptr<llvm::LLVMContext> &theContext, llvm::Function *fn,
                               const std::string &ifLabel, const std::string &elseLabel, llvm::BasicBlock **ifBlock,
                               llvm::BasicBlock **elseBlock, llvm::BasicBlock *insertBefore) {
  *ifBlock = llvm::BasicBlock::Create(*theContext, ifLabel, fn, insertBefore);
  *elseBlock = llvm::BasicBlock::Create(*theContext, elseLabel, fn, insertBefore);
}

static std::string getFunctionName(void *f) {
  Dl_info info{};

  int ret = dladdr(f, &info);
  assert(ret && "Looking for function failed");
  assert(info.dli_saddr == static_cast<decltype(Dl_info::dli_saddr)>(f));
  assert(info.dli_sname);
  return info.dli_sname;
}

static int64_t findInMap(auto map, std::string key) {
  uint64_t index = 0;
  for (auto m : map) {
    if (m.first == key) return index;
    ++index;
  }
  return -1;
}

static int64_t findInVector(auto vec, std::string elem) {
  uint64_t index = 0;
  for (auto v : vec) {
    if (std::get<0>(v) == elem) return index;
    ++index;
  }
  return static_cast<int64_t>(-1);
}

static int64_t findInMapOfVectors(auto map, std::string elem) {
  uint64_t index = 0;
  for (auto m : map) {
    index = 0;
    for (auto v : m.second) {
      if (v.first == elem) return index;
      ++index;
    }
  }
  return static_cast<int64_t>(-1);
}

static auto attrTypeMatching(std::shared_ptr<dcds::Attribute> attr, std::unique_ptr<llvm::LLVMContext> &theContext) {
  int64_t val = 0;
  if (std::holds_alternative<int64_t>(attr->initVal))
    val = 0;
  else if (std::holds_alternative<void *>(attr->initVal))
    val = 1;
  return llvm::ConstantInt::get(*theContext, llvm::APInt(64, val));
}

}  // namespace llvmutil
}  // namespace dcds

#endif  // DCDS_CODEGEN_UTILS_HPP
