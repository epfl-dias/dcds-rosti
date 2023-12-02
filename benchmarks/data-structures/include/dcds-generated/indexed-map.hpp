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

#ifndef DCDS_INDEXED_MAP_HPP
#define DCDS_INDEXED_MAP_HPP

#include "dcds-generated/base.hpp"

class IndexedMap : public dcds_generated_ds {
 public:
  IndexedMap();

 private:
  void generateLookupFunction();

  /**
   * Insert a value into the IndexedMap. Both the key and value will be copied.
   * If there was already an element in the map with the same key, it
   * will not be updated, and false will be returned. Otherwise, true will be
   * returned.
   */
  void generateInsertFunction();

  void generateUpdateFunction();

 protected:
  std::shared_ptr<dcds::Builder> generate_item();
};

#endif  // DCDS_INDEXED_MAP_HPP
