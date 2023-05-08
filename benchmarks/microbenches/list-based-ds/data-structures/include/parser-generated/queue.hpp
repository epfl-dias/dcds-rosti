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

#ifndef DCDS_BENCH_LIST_DS_QUEUE_HPP
#define DCDS_BENCH_LIST_DS_QUEUE_HPP


#include <dcds/builder/builder.hpp>

// Functionality based on https://en.cppreference.com/w/cpp/container/queue
class ParserGeneratedQueue{


    void init();



    void createStructsAndAttributes(dcds::Builder& builder);

    // Functions
    void createFunction_size(dcds::Builder& builder);
    void createFunction_empty(dcds::Builder& builder);
    void createFunction_front(dcds::Builder& builder);
    void createFunction_back(dcds::Builder& builder);
    void createFunction_pop(dcds::Builder& builder);
    void createFunction_push(dcds::Builder& builder);


};


#endif //DCDS_BENCH_LIST_DS_QUEUE_HPP
