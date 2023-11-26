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

#ifndef DCDS_TPCC_HPP
#define DCDS_TPCC_HPP

#include <dcds/dcds.hpp>
#include <memory>


using date_t = uint64_t;

// From TPCC-SPEC
constexpr size_t TPCC_MAX_ITEMS = 100000;
constexpr size_t TPCC_NCUST_PER_DIST = 3000;
constexpr size_t TPCC_NDIST_PER_WH = 10;
constexpr size_t TPCC_ORD_PER_DIST = 3000;


constexpr size_t TPCC_MAX_OL_PER_ORDER = 15;
constexpr size_t FIRST_NAME_MIN_LEN = 8;
constexpr size_t FIRST_NAME_LEN = 16;
constexpr size_t LAST_NAME_LEN = 16;


// Standard TPC-C Mix
/*
#define NO_MIX 45
#define P_MIX 43
#define OS_MIX 4
#define D_MIX 4
#define SL_MIX 4
#define MIX_COUNT 100
*/


// number_of_warehouse as runtime-parameter or build-time?

class TPCC{
 public:
  TPCC();

  void test_st();
  void test_mt();


 private:
  void gen_init_fn();

 private:
  // TPC-C transactions
  void gen_payment_txn();
  void gen_new_order_txn();
  void gen_order_status_txn();
  void gen_delivery_txn();
  void gen_stock_level_txn();

 private:
  // generate sub-structures
  void gen_stock_tbl();
  void gen_item_tbl();
  void gen_warehouse_tbl();
  void gen_district_tbl();
  void gen_history_tbl();
  void gen_customer_tbl();
  void gen_order_tbl();
  void gen_order_line_tbl();
  void gen_new_order_tbl();


 private:
  std::shared_ptr<dcds::Builder> ds_builder;


};

#endif  // DCDS_TPCC_HPP
