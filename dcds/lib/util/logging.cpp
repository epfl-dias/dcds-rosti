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

#include <absl/debugging/failure_signal_handler.h>
#include <absl/debugging/symbolize.h>
#include <absl/log/globals.h>
#include <absl/log/initialize.h>

#include <dcds/util/logging.hpp>

namespace dcds {

void InitializeLog(int argc, char** argv) {
  static bool initialized = false;
  if (!initialized) {
    initialized = true;
    absl::InitializeLog();
    // hardcoding to log everything to stderr for now
    // this can be configured to use a log file as well
    absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);
    absl::InitializeSymbolizer(argv[0]);
    auto failure_sig_handler_opts = absl::FailureSignalHandlerOptions{};
    absl::InstallFailureSignalHandler(failure_sig_handler_opts);
  }
}
}  // namespace dcds
