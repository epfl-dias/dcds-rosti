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

#ifndef DCDS_GENERATOR_HPP
#define DCDS_GENERATOR_HPP

#include <deque>
#include <string>
#include <vector>

#include "dcds/util/erase-constructor-idioms.hpp"

namespace dcds {

/*
 * This class should export .hpp/.so file in the provided output directory.
 * Also, it should add license on the top.
 * */
class CodeExporter : public dcds::remove_copy {
 public:
  CodeExporter() = default;
  virtual ~CodeExporter() = default;

 public:
  void exportToFile(std::string filename);
  void createStruct(std::string name, std::vector<dcds::SimpleAttribute> attributes);

 private:
  std::deque<std::string> structs;

  // A .hpp file will have
  //  - License
  //  - Constructor
  //  - public functions
  //  - public struct (ds struct type)
  //  - private struct (ds container struct)
  //  - Destructor?
};

// class CodeExporterContext : public dcds::remove_copy {
//  public:
//   CodeExporterContext() {}
//   virtual ~CodeExporterContext();
//
//  public:
//   std::string getLicenseString();
// };

}  // namespace dcds

#endif  // DCDS_GENERATOR_HPP
