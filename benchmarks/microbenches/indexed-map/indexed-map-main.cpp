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

#include <dcds/dcds.hpp>
#include <random>

#include "dcds-generated/indexed-map.hpp"

int main(int argc, char** argv) {
  dcds::InitializeLog(argc, argv);
  LOG(INFO) << "INDEXED_MAP";

  // dcds::ScopedAffinityManager scopedAffinity(dcds::Core{0});
  auto map = dcds::datastructures::IndexedMap();
  map.dump();
  map.build(false, false);

  auto instance = map.createInstance();
  instance->listAllAvailableFunctions();

  int64_t val = 0;
  auto x = instance->op("lookup", 1, &val);
  LOG(INFO) << std::any_cast<bool>(x);

  return 0;
}
