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

#ifndef DCDS_TIMING_HPP
#define DCDS_TIMING_HPP

#include <absl/base/log_severity.h>
#include <absl/log/absl_log.h>
#include <absl/log/check.h>
#include <absl/log/log.h>

#include <chrono>
#include <functional>
#include <string>
#include <thread>

/** Original code of the timeblock/timebomb  derived from
 *  the source code of Proteus (https://github.com/epfl-dias/proteus):
 */

template <typename Tunit>
constexpr const char *asUnit();

template <>
constexpr const char *asUnit<std::chrono::milliseconds>() {
  return "ms";
}

template <>
constexpr const char *asUnit<std::chrono::microseconds>() {
  return "us";
}

template <>
constexpr const char *asUnit<std::chrono::nanoseconds>() {
  return "ns";
}

template <>
constexpr const char *asUnit<std::chrono::seconds>() {
  return "s";
}

template <class Rep, class Period>
std::ostream &operator<<(std::ostream &out, const std::chrono::duration<Rep, Period> &duration) {
  out << duration.count() << asUnit<std::chrono::duration<Rep, Period>>();
  return out;
}

template <class Rep, class Period>
std::string toString(const std::chrono::duration<Rep, Period> &duration) {
  std::stringstream out;
  out << duration.count() << asUnit<std::chrono::duration<Rep, Period>>();
  return out.str();
}

class nested_time_block {
 public:
  static size_t &getNestLevel() {
    static thread_local size_t nest_level = 0;
    return nest_level;
  }
};

class AbslLog {
 private:
  absl::LogSeverity severity;

 public:
  inline constexpr explicit AbslLog(absl::LogSeverity s) : severity(s) {}

  friend class Severity;
  template <typename Tduration, typename Tclock>
  friend class time_blockT;
};

class Severity {
 public:
  static constexpr AbslLog INFO{absl::LogSeverity::kInfo};
  static constexpr AbslLog WARNING{absl::LogSeverity::kWarning};
  static constexpr AbslLog ERROR{absl::LogSeverity::kError};
  static constexpr AbslLog FATAL{absl::LogSeverity::kFatal};
};

template <typename Tduration = std::chrono::milliseconds, typename Tclock = std::chrono::system_clock>
class [[nodiscard]] time_blockT {
 protected:
  using clock = Tclock;
  using dur = typename Tclock::duration;

  std::function<void(Tduration)> reg;
  std::chrono::time_point<clock> start;

  static_assert(dur{1} <= Tduration{1}, "clock not precise enough");

 public:
  inline explicit time_blockT(decltype(reg) _reg) : reg(std::move(_reg)), start(clock::now()) {}

  inline explicit time_blockT(std::string text = "", AbslLog logSeverity = Severity::INFO,
                              decltype(__builtin_FILE()) file = __builtin_FILE(),
                              decltype(__builtin_LINE()) line = __builtin_LINE())
      : time_blockT([_text{std::move(text)}, file, line, logSeverity](const auto &t) {
          auto s = --nested_time_block::getNestLevel();
          absl::log_internal::LogMessage(file, line, logSeverity.severity)
              << '\t' << std::string(s, '|') << _text << toString(t);
          // LOG(INFO).AtLocation(file, line) << '\t' << std::string(s, '|') << _text << toString(t);
        }) {
    ++nested_time_block::getNestLevel();
  }

  inline explicit time_blockT() : reg([](const auto &t) {}), start(clock::now()) {}

  inline ~time_blockT() {
    auto end = clock::now();
    auto d = std::chrono::duration_cast<Tduration>(end - start);
    reg(d);
  }
};

class time_block : public time_blockT<std::chrono::milliseconds> {
  using time_blockT::time_blockT;
};

class time_block_us : public time_blockT<std::chrono::microseconds> {
  using time_blockT::time_blockT;
};

class time_block_ns : public time_blockT<std::chrono::nanoseconds, std::chrono::high_resolution_clock> {
  using time_blockT::time_blockT;
};

#endif  // DCDS_TIMING_HPP
