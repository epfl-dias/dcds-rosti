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

#ifndef DCDS_PTR_PACKING_HPP
#define DCDS_PTR_PACKING_HPP

#include <iostream>

// 48-bits for actual stuff
// 16-bits for storing anything

// Check top-byte-ignore

namespace dcds {
class PackerPtr {
  static_assert(sizeof(uintptr_t) == 8, "Invalid size of uintptr_8, expected 8B");

 private:
  static constexpr auto const PTR_BITS = 48u;
  static constexpr auto const DATA_BITS = 16u;
  // static constexpr auto const PTR_MASK = ~(0xFFull << 56u);
  static constexpr auto const PTR_MASK = 0x0000FFFFFFFFFFFFu;

  static_assert((PTR_BITS + DATA_BITS) / 8 == sizeof(unsigned long long), "Invalid size of ULL, expected 8B");

 public:
  static constexpr auto const DATA_SZ_MAX = DATA_BITS / 8;

 public:
  explicit PackerPtr() : pt(0) {}
  explicit PackerPtr(void* ptr) : pt(0) { pt |= (reinterpret_cast<uintptr_t>(ptr) & PTR_MASK); }
  explicit PackerPtr(uintptr_t ptr) : pt(0) { pt = ptr; }
  explicit PackerPtr(uintptr_t ptr, uint16_t data) : pt(0) {
    pt = data;
    pt = (pt << PTR_BITS);
    pt |= (ptr & PTR_MASK);
  }

  auto operator->() const { return reinterpret_cast<void*>(pt & PTR_MASK); }

  void print() const {  // LOG(INFO) << "data: " << getData() << " | pt:  " << reinterpret_cast<void*>(pt & PTR_MASK);
  }

  [[nodiscard]] uint16_t getData() const { return (pt >> 56u); }
  [[nodiscard]] auto getPtr() const { return pt; }

  void setData(uint16_t data) {
    uint64_t tmp = data;
    pt &= PTR_MASK;         // clear existing data
    pt |= tmp << PTR_BITS;  // set new data
  }

 private:
  uintptr_t pt;
};

using packed_ptr_t = PackerPtr;
}  // namespace dcds

#endif  // DCDS_PTR_PACKING_HPP
