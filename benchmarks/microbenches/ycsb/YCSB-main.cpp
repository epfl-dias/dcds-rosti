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

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>

#include <dcds/dcds.hpp>

#include "ycsb.hpp"

ABSL_FLAG(uint16_t, num_columns, 1, "number of columns for YCSB table");
ABSL_FLAG(uint16_t, num_iterations, 1, "number of iterations for each test");
ABSL_FLAG(uint16_t, num_threads, 1, "number of threads");
ABSL_FLAG(uint16_t, rw_ratio, 0, "rw_ratio");
ABSL_FLAG(double, zipf_theta, 0, "zipf_theta");
ABSL_FLAG(bool, use_flag, false, "use the flags or ignore");

static void play() {
  LOG(INFO) << "play";
  constexpr auto num_columns = 4;
  constexpr size_t num_runs = 3;
  size_t rw_ratio = 0;

  size_t num_threads = 18;
  auto ycsb = YCSB(num_columns, num_threads * 1_M);

  dcds::profiling::Profile::resume();

  ycsb.test_MT_rw_random(num_threads, rw_ratio);

  ycsb.test_MT_rw_zipf(24, 0.5, 50);
  ycsb.test_MT_rw_zipf(24, 0.000001, 50);

  dcds::profiling::Profile::pause();

  dcds::storage::TableRegistry::getInstance().clear();
}

static void no_flags() {
  constexpr auto num_columns = 1;
  constexpr size_t num_runs = 3;
  //  std::vector<size_t> cores{1, 2, 4, 8, 16, 18, 24, 32, 36, 48, 64, 72, 84, 108, 120, 128, 144};
  std::vector<size_t> cores{1, 2, 4, 8, 12, 16, 18, 24, 32, 36, 48};
  std::vector<size_t> rw_ratios{0, 25, 50, 75, 100};

  // FIXME: more than 1 column deadlocks for write.

  //  for (auto c = 1; c <= num_columns; c++) {
  //    LOG(INFO) << "###### Number of columns: " << c;
  //    for (auto i : cores) {
  //      for (size_t r = 0; r < num_runs; r++) {
  //        auto ycsb = YCSB(c, i * 1_M);
  //        if(i == cores.front() || i == cores.back())
  //          dcds::profiling::Profile::resume();
  //
  //        //ycsb.test_MT_lookup_random(i);
  //
  //        if(i == cores.front() || i == cores.back())
  //          dcds::profiling::Profile::pause();
  //
  //
  //        dcds::storage::TableRegistry::getInstance().clear();
  //      }
  //    }
  //  }

  for (auto c = 1; c <= num_columns; c++) {
    LOG(INFO) << "###### Number of columns: " << c;
    for (auto rw : rw_ratios) {
      LOG(INFO) << "###### RW-RATIO: : " << rw;
      for (auto t : cores) {
        for (size_t r = 0; r < num_runs; r++) {
          auto ycsb = YCSB(c, t * 1_M);

          if (t == cores.front() || t == cores.back()) dcds::profiling::Profile::resume();

          // ycsb.test_MT_rw_random(t, rw);
          ycsb.test_MT_rw_zipf(t, 0.5, rw);

          if (t == cores.front() || t == cores.back()) dcds::profiling::Profile::pause();

          dcds::storage::TableRegistry::getInstance().clear();
        }
      }
    }
  }
}

static void use_flags(int argc, char** argv) {
  // absl::ParseCommandLine(argc, argv);
  auto num_columns = absl::GetFlag(FLAGS_num_columns);
  auto num_runs = absl::GetFlag(FLAGS_num_iterations);
  auto num_threads = absl::GetFlag(FLAGS_num_threads);
  auto rw_ratio = absl::GetFlag(FLAGS_rw_ratio);
  auto zipf_theta = absl::GetFlag(FLAGS_zipf_theta);

  if (zipf_theta >= 1) zipf_theta = zipf_theta / 100;

  //  // FIXME: more than 1 column deadlocks.

  LOG(INFO) << "num_columns: " << num_columns;
  LOG(INFO) << "num_runs: " << num_runs;
  LOG(INFO) << "num_threads: " << num_threads;
  LOG(INFO) << "rw_ratio: " << rw_ratio;
  LOG(INFO) << "zipf_theta: " << zipf_theta;

  assert(rw_ratio >= 0 && rw_ratio <= 100);

  for (size_t r = 0; r < num_runs; r++) {
    auto ycsb = YCSB(num_columns, num_threads * 1_M);
    if (zipf_theta > 0) {
      ycsb.test_MT_rw_zipf(num_threads, zipf_theta, rw_ratio);
    } else {
      ycsb.test_MT_rw_random(num_threads, rw_ratio);
    }

    dcds::storage::TableRegistry::getInstance().clear();
  }
}

int main(int argc, char** argv) {
  dcds::InitializeLog(argc, argv);
  absl::ParseCommandLine(argc, argv);

  LOG(INFO) << "YCSB";

  auto use_flag = absl::GetFlag(FLAGS_use_flag);
  if (use_flag) {
    use_flags(argc, argv);
  } else {
    play();
    //      no_flags();
  }

  // dcds::ScopedAffinityManager scopedAffinity(dcds::Core{0});

  // use_flags(argc, argv);

  // no_flags();

  return 0;
}
