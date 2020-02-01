/*
    Harmonia -- High-performance elastic HTAP on heterogeneous hardware.

                            Copyright (c) 2017
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
#include "q04.hpp"

static int q_instance = 10;

PreparedStatement Q_4_cpar(DegreeOfParallelism dop, const aff_t &aff_parallel,
                           const aff_t &aff_reduce, DeviceType dev,
                           const scan_t &scan) {
  assert(dev == DeviceType::CPU);

  auto rel1968 =
      scan(
          "tpcc_order",
          {"o_id", "o_d_id", "o_w_id",
           "o_entry_d"})  // (table=[[SSB, ch100w_order]], fields=[[0, 1, 2,
                          // 4]],
                          // traits=[Pelago.[].X86_64.packed.homSingle.hetSingle.none])
          .membrdcst(dop, true, true)
          .router(
              [&](const auto &arg) -> std::optional<expression_t> {
                return arg["__broadcastTarget"];
              },
              dop, 1, RoutingPolicy::HASH_BASED, dev,
              aff_parallel())  // (trait=[Pelago.[].X86_64.packed.homBrdcst.hetSingle.none])
          .unpack()  // (trait=[Pelago.[].X86_64.unpckd.homBrdcst.hetSingle.cX86_64])
          .filter([&](const auto &arg) -> expression_t {
            return (ge(arg["$3"], expressions::DateConstant(1167696000000)) &
                    lt(arg["$3"], expressions::DateConstant(1325462400000)));
          })  // (condition=[AND(>=($3, 2007-01-02 00:00:00:TIMESTAMP(3)), <($3,
              // 2012-01-02 00:00:00:TIMESTAMP(3)))],
              // trait=[Pelago.[].X86_64.unpckd.homBrdcst.hetSingle.cX86_64],
              // isS=[false])
      ;
  auto rel1975 =
      scan(
          "tpcc_order",
          {"o_id", "o_d_id", "o_w_id", "o_entry_d",
           "o_ol_cnt"})  // (table=[[SSB, ch100w_order]], fields=[[0, 1, 2,
                         // 4, 6]],
                         // traits=[Pelago.[].X86_64.packed.homSingle.hetSingle.none])
          .router(
              dop, 1, RoutingPolicy::LOCAL, dev,
              aff_parallel())  // (trait=[Pelago.[].X86_64.packed.homRandom.hetSingle.none])
          .unpack()  // (trait=[Pelago.[].X86_64.unpckd.homRandom.hetSingle.cX86_64])
          .filter([&](const auto &arg) -> expression_t {
            return (ge(arg["$3"], expressions::DateConstant(1167696000000)) &
                    lt(arg["$3"], expressions::DateConstant(1325462400000)));
          })  // (condition=[AND(>=($3, 2007-01-02 00:00:00:TIMESTAMP(3)), <($3,
              // 2012-01-02 00:00:00:TIMESTAMP(3)))],
              // trait=[Pelago.[].X86_64.unpckd.homRandom.hetSingle.cX86_64],
              // isS=[false])
          .join(
              rel1968,
              [&](const auto &build_arg) -> expression_t {
                return expressions::RecordConstruction{
                    build_arg["$0"].as("PelagoJoin#1972", "bk_0"),
                    build_arg["$1"].as("PelagoJoin#1972", "bk_1"),
                    build_arg["$2"].as("PelagoJoin#1972", "bk_2"),
                    build_arg["$3"].as("PelagoJoin#1972", "bk_3")}
                    .as("PelagoJoin#1972", "bk");
              },
              [&](const auto &probe_arg) -> expression_t {
                return expressions::RecordConstruction{
                    probe_arg["$0"].as("PelagoJoin#1972", "pk_0"),
                    probe_arg["$1"].as("PelagoJoin#1972", "pk_1"),
                    probe_arg["$2"].as("PelagoJoin#1972", "pk_2"),
                    probe_arg["$3"].as("PelagoJoin#1972", "pk_3")}
                    .as("PelagoJoin#1972", "pk");
              },
              28,
              3000000)  // (condition=[AND(=($4, $0), =($5, $1), =($6, $2),
                        // =($7, $3))], joinType=[inner], rowcnt=[7.68E8],
                        // maxrow=[3000000.0], maxEst=[3000000.0], h_bits=[28],
                        // build=[RecordType(BIGINT o_id, INTEGER o_d_id,
                        // INTEGER o_w_id, TIMESTAMP(0) o_entry_d)],
                        // lcount=[6.7109666771656204E10], rcount=[3.84E8],
                        // buildcountrow=[7.68E8], probecountrow=[3.84E8])
          .project([&](const auto &arg) -> std::vector<expression_t> {
            return {(arg["$8"]).as("PelagoProject#1973", "$0"),
                    (arg["$0"]).as("PelagoProject#1973", "$1"),
                    (arg["$1"]).as("PelagoProject#1973", "$2"),
                    (arg["$2"]).as("PelagoProject#1973", "$3"),
                    (arg["$3"]).as("PelagoProject#1973", "$4")};
          })  // (o_ol_cnt=[$8], o_id0=[$0], o_d_id0=[$1], o_w_id0=[$2],
              // o_entry_d0=[$3],
              // trait=[Pelago.[].X86_64.unpckd.homRandom.hetSingle.cX86_64])
          .pack()
          .router(
              DegreeOfParallelism{1}, 128, RoutingPolicy::RANDOM,
              DeviceType::CPU,
              aff_reduce())  // (trait=[Pelago.[].X86_64.unpckd.homSingle.hetSingle.cX86_64])
          .membrdcst(dop, true, true)
          .router(
              [&](const auto &arg) -> std::optional<expression_t> {
                return arg["__broadcastTarget"];
              },
              dop, 1, RoutingPolicy::HASH_BASED, dev,
              aff_parallel())  // (trait=[Pelago.[].X86_64.unpckd.homBrdcst.hetSingle.cX86_64])
          .unpack();
  return scan(
             "tpcc_orderline",
             {"ol_o_id", "ol_d_id", "ol_w_id",
              "ol_delivery_d"})  // (table=[[SSB, ch100w_orderline]],
                                 // fields=[[0, 1, 2, 6]],
                                 // traits=[Pelago.[].X86_64.packed.homSingle.hetSingle.none])
      .router(
          dop, 1, RoutingPolicy::LOCAL, dev,
          aff_parallel())  // (trait=[Pelago.[].X86_64.packed.homRandom.hetSingle.none])
      .unpack()  // (trait=[Pelago.[].X86_64.unpckd.homRandom.hetSingle.cX86_64])
      .join(
          rel1975,
          [&](const auto &build_arg) -> expression_t {
            return expressions::RecordConstruction{
                build_arg["$1"].as("PelagoJoin#1978", "bk_1"),
                build_arg["$3"].as("PelagoJoin#1978", "bk_3"),
                build_arg["$2"].as("PelagoJoin#1978", "bk_2")}
                .as("PelagoJoin#1978", "bk");
          },
          [&](const auto &probe_arg) -> expression_t {
            return expressions::RecordConstruction{
                probe_arg["$0"].as("PelagoJoin#1978", "pk_0"),
                probe_arg["$2"].as("PelagoJoin#1978", "pk_2"),
                probe_arg["$1"].as("PelagoJoin#1978", "pk_1")}
                .as("PelagoJoin#1978", "pk");
          },
          28,
          67108864)  // (condition=[AND(=($1,
                     // $5), =($3, $7), =($2,
                     // $6))], joinType=[inner],
                     // rowcnt=[3.84E8],
                     // maxrow=[9.0E12],
                     // maxEst=[6.7108864E7],
                     // h_bits=[28],
                     // build=[RecordType(INTEGER
                     // o_ol_cnt, BIGINT o_id0,
                     // INTEGER o_d_id0, INTEGER
                     // o_w_id0, TIMESTAMP(0)
                     // o_entry_d0)],
                     // lcount=[4.1041134764133316E10],
                     // rcount=[1.536E10],
                     // buildcountrow=[3.84E8],
                     // probecountrow=[1.536E10])
      .filter([&](const auto &arg) -> expression_t {
        return ge(arg["$8"], arg["$4"]);
      })  // (condition=[>=($8, $4)],
          // trait=[Pelago.[].X86_64.unpckd.homRandom.hetSingle.cX86_64],
          // isS=[false])
      .project([&](const auto &arg) -> std::vector<expression_t> {
        return {(arg["$0"]).as("PelagoProject#1980", "$0")};
      })  // (o_ol_cnt=[$0],
          // trait=[Pelago.[].X86_64.unpckd.homRandom.hetSingle.cX86_64])
      .groupby(
          [&](const auto &arg) -> std::vector<expression_t> {
            return {arg["$0"].as("PelagoAggregate#1981", "$0")};
          },
          [&](const auto &arg) -> std::vector<GpuAggrMatExpr> {
            return {GpuAggrMatExpr{
                (expression_t{1}).as("PelagoAggregate#1981", "$1"), 1, 0, SUM}};
          },
          10,
          131072)  // (group=[{0}],
                   // order_count=[COUNT()],
                   // trait=[Pelago.[].X86_64.unpckd.homRandom.hetSingle.cX86_64],
                   // global=[false])
      .router(
          DegreeOfParallelism{1}, 128, RoutingPolicy::RANDOM, DeviceType::CPU,
          aff_reduce())  // (trait=[Pelago.[].X86_64.unpckd.homSingle.hetSingle.cX86_64])
      .groupby(
          [&](const auto &arg) -> std::vector<expression_t> {
            return {arg["$0"].as("PelagoAggregate#1983", "$0")};
          },
          [&](const auto &arg) -> std::vector<GpuAggrMatExpr> {
            return {GpuAggrMatExpr{(arg["$1"]).as("PelagoAggregate#1983", "$1"),
                                   1, 0, SUM}};
          },
          10,
          131072)  // (group=[{0}],
                   // order_count=[$SUM0($1)],
                   // trait=[Pelago.[].X86_64.unpckd.homSingle.hetSingle.cX86_64],
                   // global=[true])
      .sort(
          [&](const auto &arg) -> std::vector<expression_t> {
            return {arg["$0"], arg["$1"]};
          },
          {
              direction::NONE,
              direction::ASC,
          })  // (sort0=[$0], dir0=[ASC],
              // trait=[Pelago.[0].X86_64.unpckd.homSingle.hetSingle.cX86_64])
      .print([&](const auto &arg,
                 std::string outrel) -> std::vector<expression_t> {
        return {arg["$0"].as(outrel, "o_ol_cnt"),
                arg["$1"].as(outrel, "order_count")};
      })  // (trait=[ENUMERABLE.[0].X86_64.unpckd.homSingle.hetSingle.cX86_64])
      .prepare();
}