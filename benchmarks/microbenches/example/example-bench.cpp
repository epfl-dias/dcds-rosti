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
// Created by Hamish Nicholson on 13.04.23.
//

#include <benchmark/benchmark.h>

#include <dcds/util/logging.hpp>
#include <dcds/util/profiling.hpp>
#include <iostream>
#include <span>
#include <sstream>

using StridedAccessArgs = std::tuple<std::span<uint8_t>, std::span<uint8_t>, size_t, size_t>;

template <class... Args>
void BMStridedAccess(benchmark::State& state, Args&&... args) {
  auto args_tuple = std::make_tuple(std::move(args)...);
  std::stringstream args_log_str{"Running with args: "};
  static_assert(std::is_same<StridedAccessArgs, decltype(args_tuple)>::value);
  const std::span<uint8_t> source = std::get<0>(args_tuple);
  const std::span<uint8_t> destination = std::get<1>(args_tuple);
  const auto stride = std::get<2>(args_tuple);
  const auto read_size = std::get<3>(args_tuple);
  const int64_t bytes_per_iteration = (source.size() / stride) * read_size;

  dcds::profiling::ProfileRegion my_region("test_region");
  for (auto _ : state) {
    size_t index = 0;
    //        size_t sum = 0;
    while (index < source.size()) {
      std::memcpy(&destination[index], &source[index], read_size);
      index += stride;
    }
    //        // LOG(INFO) << "hi";
  }
  state.SetBytesProcessed(state.iterations() * bytes_per_iteration);
}

static void BM_SomeFunction(benchmark::State& state) {
  // Perform setup here
  dcds::profiling::ProfileRegion my_region("test_region");
  dcds::profiling::ProfileMarkPoint iter_start("iter_start");
  for (auto _ : state) {
    int sum = 0;
    iter_start.mark();
    // This code gets timed
    for (int i = 0; i < 100; i++) {
      sum += 1;
    }
    // LOG(INFO) << sum;
  }
}
// Register the function as a benchmark
BENCHMARK(BM_SomeFunction);
