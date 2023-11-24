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

#ifndef DCDS_AFFINITY_MANAGER_HPP
#define DCDS_AFFINITY_MANAGER_HPP

#include <numa.h>
#include <pthread.h>

#include <iostream>

#include "dcds/util/logging.hpp"

namespace dcds {

class Core {
 public:
  explicit Core(int _core_id) : core_id(_core_id) {}

  const int core_id;
};

class ScopedAffinityManager {
 public:
  explicit ScopedAffinityManager(Core coreId) : previousAffinitySet(false) {
    if (getAffinity(&previousAffinityMask)) {
      setAffinityToCore(coreId);
    }
  }

  ~ScopedAffinityManager() {
    if (previousAffinitySet) {
      pthread_t thread = pthread_self();
      pthread_setaffinity_np(thread, sizeof(cpu_set_t), &previousAffinityMask);
    }
  }

 private:
  static bool getAffinity(cpu_set_t* affinityMask) {
    pthread_t thread = pthread_self();
    if (pthread_getaffinity_np(thread, sizeof(cpu_set_t), affinityMask) != 0) {
      LOG(ERROR) << "Failed to get current affinity";
      return false;
    }
    return true;
  }

  void setAffinity(const cpu_set_t* affinityMask) {
    pthread_t thread = pthread_self();
    if (pthread_setaffinity_np(thread, sizeof(cpu_set_t), affinityMask) != 0) {
      LOG(ERROR) << "Failed to set affinity";
    } else {
      previousAffinitySet = true;
    }
  }

  void setAffinityToCore(Core coreId) {
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(coreId.core_id, &cpu_set);
    setAffinity(&cpu_set);
  }

  bool previousAffinitySet;
  cpu_set_t previousAffinityMask{};
};
}  // namespace dcds

#endif  // DCDS_AFFINITY_MANAGER_HPP
