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

#ifndef DCDS_PROFILING_HPP
#define DCDS_PROFILING_HPP

#include <string>
// support building without vtune available
#if __has_include("ittnotify.h")
#include <ittnotify.h>
#else
#define __itt_event() ((void)0)
#endif

namespace dcds::profiling {
    class ProfileRegion {
    public:
        explicit ProfileRegion(std::string region_name);

        ~ProfileRegion();

    private:
        __itt_event m_event;
    };

    class ProfileMarkPoint {
    public:
        explicit ProfileMarkPoint(std::string point_name);

        void mark();

    private:
        __itt_event m_event;
    };
}

#endif //DCDS_PROFILING_HPP
