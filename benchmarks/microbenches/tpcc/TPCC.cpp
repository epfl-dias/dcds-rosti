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

#include "TPCC.hpp"

TPCC::TPCC() : ds_builder(std::make_shared<dcds::Builder>("TPCC")) {}

void TPCC::gen_stock_tbl() {
  auto builder = this->ds_builder->createType("tpcc_stock");
  // FIXME: create a char/string type

  builder->addAttribute("s_i_id", dcds::valueType::INT32, UINT32_C(0));
  builder->addAttribute("s_w_id", dcds::valueType::INT32, UINT32_C(0));
  builder->addAttribute("s_quantity", dcds::valueType::INT32, UINT32_C(0));

  // TODO: char s_dist[TPCC_NDIST_PER_WH][24];

  builder->addAttribute("s_ytd", dcds::valueType::INT32, UINT32_C(0));
  builder->addAttribute("s_order_cnt", dcds::valueType::INT32, UINT32_C(0));
  builder->addAttribute("s_remote_cnt", dcds::valueType::INT32, UINT32_C(0));

  // TODO: char s_data[51];

  builder->addAttribute("s_su_suppkey", dcds::valueType::INT32, UINT32_C(0));
}

void TPCC::gen_item_tbl() {
  auto builder = this->ds_builder->createType("tpcc_item");

  builder->addAttribute("i_id", dcds::valueType::INT32, UINT32_C(0));
  builder->addAttribute("i_im_id", dcds::valueType::INT32, UINT32_C(0));

  // TODO: char i_name[25];

  builder->addAttribute("i_price", dcds::valueType::DOUBLE, double_t(0));

  // TODO: char i_data[51];
}

void TPCC::gen_warehouse_tbl() {
  auto builder = this->ds_builder->createType("tpcc_warehouse");

  builder->addAttribute("w_id", dcds::valueType::INT32, UINT32_C(0));
  //  TODO: char w_name[11];
  //  TODO: char w_street[2][21];
  //  TODO: char w_city[21];
  //  TODO: char w_state[2];
  //  TODO: char w_zip[9];

  builder->addAttribute("w_tax", dcds::valueType::DOUBLE, double_t(0));
  builder->addAttribute("w_ytd", dcds::valueType::DOUBLE, double_t(0));
}

void TPCC::gen_district_tbl() {
  auto builder = this->ds_builder->createType("tpcc_district");

  builder->addAttribute("d_id", dcds::valueType::INT32, UINT32_C(0));
  builder->addAttribute("d_w_id", dcds::valueType::INT32, UINT32_C(0));
  // TODO: char d_name[11];
  // TODO: char d_street[2][21];
  // TODO: char d_city[21];
  // TODO: char d_state[2];
  // TODO: char d_zip[9];
  builder->addAttribute("d_tax", dcds::valueType::DOUBLE, double_t(0));
  builder->addAttribute("d_ytd", dcds::valueType::DOUBLE, double_t(0));
  builder->addAttribute("d_next_o_id", dcds::valueType::INT32, UINT32_C(0));
}

void TPCC::gen_history_tbl() {
  auto builder = this->ds_builder->createType("tpcc_history");

  // TODO: opportunity for spill-able structure

  builder->addAttribute("h_c_id", dcds::valueType::INT32, UINT32_C(0));
  builder->addAttribute("h_c_d_id", dcds::valueType::INT32, UINT32_C(0));
  builder->addAttribute("h_c_w_id", dcds::valueType::INT32, UINT32_C(0));
  builder->addAttribute("h_d_id", dcds::valueType::INT32, UINT32_C(0));
  builder->addAttribute("h_w_id", dcds::valueType::INT32, UINT32_C(0));
  builder->addAttribute("h_date", dcds::valueType::INT64, UINT64_C(0));
  builder->addAttribute("h_amount", dcds::valueType::DOUBLE, double_t(0));
  // TODO: char h_data[25];
}

void TPCC::gen_customer_tbl() {
  auto builder = this->ds_builder->createType("tpcc_customer");

  builder->addAttribute("c_id", dcds::valueType::INT32, UINT32_C(0));
  builder->addAttribute("c_w_id", dcds::valueType::INT32, UINT32_C(0));
  builder->addAttribute("c_d_id", dcds::valueType::INT32, UINT32_C(0));

  // TODO: char c_first[FIRST_NAME_LEN + 1];
  // TODO: char c_middle[2];
  // TODO: char c_last[LAST_NAME_LEN + 1];
  // TODO: char c_street[2][21];
  // TODO: char c_city[21];
  // TODO: char c_state[2];
  // TODO: char c_zip[9];
  // TODO: char c_phone[16];
  builder->addAttribute("c_since", dcds::valueType::INT64, UINT64_C(0));
  // TODO: char c_credit[2];
  builder->addAttribute("c_credit_lim", dcds::valueType::DOUBLE, double_t(0));
  builder->addAttribute("c_discount", dcds::valueType::DOUBLE, double_t(0));
  builder->addAttribute("c_balance", dcds::valueType::DOUBLE, double_t(0));
  builder->addAttribute("c_ytd_payment", dcds::valueType::DOUBLE, double_t(0));
  builder->addAttribute("c_payment_cnt", dcds::valueType::INT32, UINT32_C(0));
  builder->addAttribute("c_delivery_cnt", dcds::valueType::INT32, UINT32_C(0));
  // TODO: char c_data[501];
}

void TPCC::gen_order_tbl() {
  auto builder = this->ds_builder->createType("tpcc_order");

  builder->addAttribute("o_id", dcds::valueType::INT32, UINT32_C(0));
  builder->addAttribute("o_d_id", dcds::valueType::INT32, UINT32_C(0));
  builder->addAttribute("o_w_id", dcds::valueType::INT32, UINT32_C(0));
  builder->addAttribute("o_c_id", dcds::valueType::INT32, UINT32_C(0));
  builder->addAttribute("o_entry_d", dcds::valueType::INT64, UINT64_C(0));
  builder->addAttribute("o_carrier_id", dcds::valueType::INT32, UINT32_C(0));
  builder->addAttribute("o_ol_cnt", dcds::valueType::INT32, UINT32_C(0));
  builder->addAttribute("o_all_local", dcds::valueType::INT32, UINT32_C(0));
}

void TPCC::gen_order_line_tbl() {
  auto builder = this->ds_builder->createType("tpcc_order_line");

  builder->addAttribute("ol_o_id", dcds::valueType::INT32, UINT32_C(0));
  builder->addAttribute("ol_d_id", dcds::valueType::INT32, UINT32_C(0));
  builder->addAttribute("ol_w_id", dcds::valueType::INT32, UINT32_C(0));
  builder->addAttribute("ol_number", dcds::valueType::INT32, UINT32_C(0));
  builder->addAttribute("ol_i_id", dcds::valueType::INT32, UINT32_C(0));
  builder->addAttribute("ol_supply_w_id", dcds::valueType::INT32, UINT32_C(0));
  builder->addAttribute("ol_delivery_d", dcds::valueType::INT64, UINT64_C(0));
  builder->addAttribute("ol_quantity", dcds::valueType::INT32, UINT32_C(0));
  builder->addAttribute("ol_amount", dcds::valueType::DOUBLE, double_t(0));

  // TODO: char ol_dist_info[24];
}

void TPCC::gen_new_order_tbl() {
  auto builder = this->ds_builder->createType("tpcc_new_oder");

  builder->addAttribute("no_o_id", dcds::valueType::INT32, UINT32_C(0));
  builder->addAttribute("no_d_id", dcds::valueType::INT32, UINT32_C(0));
  builder->addAttribute("no_w_id", dcds::valueType::INT32, UINT32_C(0));
}
