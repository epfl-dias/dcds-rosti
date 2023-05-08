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

#include <benchmark/benchmark.h>
#include <iostream>
#include <dcds/util/logging.hpp>
#include <dcds/util/profiling.hpp>

#include <dcds/builder/builder.hpp>

#include "parser-generated/queue.hpp"

using namespace dcds;

//void ParserGeneratedQueue::init(){
//
//    // By default, builder itself is a struct.
//    dcds::Builder dsBuilder("parserGeneratedQueue");
//
//
//
//}
//
//void ParserGeneratedQueue::createStructsAndAttributes(dcds::Builder& builder){
//    auto listNodeStruct = Builder::createStruct("list_node");
//    listNodeStruct.addAttribute("payload", dcds::INTEGER, 0); // defaultValue
//    listNodeStruct.addAttribute("next", dcds::RECORD_PTR, "list_node"); // defaultValue should be nullPtr here
//
//    dsBuilder.addStruct(listNodeStruct);
//
//    dsBuilder.addAttribute("head", dcds::RECORD_PTR, "list_node" );
//    dsBuilder.addAttribute("tail", dcds::RECORD_PTR, "list_node" );
//    dsBuilder.addAttribute("size", dcds::INTEGER, "list_node" );
//
//    builder.addStruct(listNodeStruct);
//}
//
//// Functions
//void ParserGeneratedQueue::createFunction_size(dcds::Builder& builder){
//    // pushFunction:: functionBuilder(name, returnType)
//
//
//    auto sizeFunction = dcds::Builder::createFunction("size", builder.getAttribute("size")->type);
//
//
//    sizeFunction.loadValue(builder.getAttribute("size"));
//
//    auto sizeValue = builder.getAttribute("size").value;
//
//    sizeFunction.addReturnStatement(sizeValue);
//
//
//    builder.addFunction(sizeFunction);
//
//}
//void ParserGeneratedQueue::createFunction_empty(dcds::Builder& builder){}
//void ParserGeneratedQueue::createFunction_front(dcds::Builder& builder){}
//void ParserGeneratedQueue::createFunction_back(dcds::Builder& builder){}
//void ParserGeneratedQueue::createFunction_pop(dcds::Builder& builder){}
//void ParserGeneratedQueue::createFunction_push(dcds::Builder& builder){}



