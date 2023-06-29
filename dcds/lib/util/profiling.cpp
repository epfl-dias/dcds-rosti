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

#include "dcds/util/profiling.hpp"

#include "dcds/util/logging.hpp"

using namespace dcds::profiling;

ProfileRegion::ProfileRegion(std::string region_name) {
#if __has_include("ittnotify.h")
  m_event = __itt_event_create(region_name.c_str(), region_name.length());
  __itt_event_start(m_event);
#else
  LOG(WARNING) << "ProfileRegion noop: vtune headers unavailable at build time";
#endif
}

ProfileRegion::~ProfileRegion() {
#if __has_include("ittnotify.h")
  __itt_event_end(m_event);
#endif
}

ProfileMarkPoint::ProfileMarkPoint(std::string point_name) {
#if __has_include("ittnotify.h")
  m_event = __itt_event_create(point_name.c_str(), point_name.length());
#else
  LOG(WARNING) << "ProfileRegion noop: vtune headers unavailable at build time";
#endif
}

void ProfileMarkPoint::mark() {
#if __has_include("ittnotify.h")
  __itt_event_start(m_event);
#endif
}
