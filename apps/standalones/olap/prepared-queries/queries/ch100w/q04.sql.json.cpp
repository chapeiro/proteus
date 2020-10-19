/*
    Proteus -- High-performance query processing on heterogeneous hardware.

                            Copyright (c) 2019
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

// AUTOGENERATED FILE. DO NOT EDIT.

constexpr auto query = "ch100w_Q04";

#include "query.cpp.inc"

PreparedStatement Query::prepare04(bool memmv) {
  auto rel2895 =
      getBuilder<Tplugin>()
          .scan(
              "inputs/ch100w/order.csv",
              {"o_id", "o_d_id", "o_w_id", "o_entry_d"}, getCatalog(),
              pg{Tplugin::
                     type})  // (table=[[SSB, ch100w_order]], fields=[[0, 1, 2,
                             // 4]],
                             // traits=[Pelago.[].packed.X86_64.homSingle.hetSingle])
          .to_gpu()  // (trait=[Pelago.[].packed.NVPTX.homSingle.hetSingle])
          .unpack()  // (trait=[Pelago.[].unpckd.NVPTX.homSingle.hetSingle])
          .filter([&](const auto &arg) -> expression_t {
            return (
                ge(arg["o_entry_d"], expressions::DateConstant(1167696000000)) &
                lt(arg["o_entry_d"], expressions::DateConstant(1325462400000)));
          })  // (condition=[AND(>=($3, 2007-01-02 00:00:00:TIMESTAMP(3)), <($3,
              // 2012-01-02 00:00:00:TIMESTAMP(3)))],
              // trait=[Pelago.[].unpckd.NVPTX.homSingle.hetSingle],
              // isS=[false])
      ;
  auto rel2906 =
      getBuilder<Tplugin>()
          .scan(
              "inputs/ch100w/order.csv",
              {"o_id", "o_d_id", "o_w_id", "o_entry_d", "o_ol_cnt"},
              getCatalog(),
              pg{Tplugin::
                     type})  // (table=[[SSB, ch100w_order]], fields=[[0, 1, 2,
                             // 4, 6]],
                             // traits=[Pelago.[].packed.X86_64.homSingle.hetSingle])
          .to_gpu()  // (trait=[Pelago.[].packed.NVPTX.homSingle.hetSingle])
          .unpack()  // (trait=[Pelago.[].unpckd.NVPTX.homSingle.hetSingle])
          .filter([&](const auto &arg) -> expression_t {
            return (
                ge(arg["o_entry_d"], expressions::DateConstant(1167696000000)) &
                lt(arg["o_entry_d"], expressions::DateConstant(1325462400000)));
          })  // (condition=[AND(>=($3, 2007-01-02 00:00:00:TIMESTAMP(3)), <($3,
              // 2012-01-02 00:00:00:TIMESTAMP(3)))],
              // trait=[Pelago.[].unpckd.NVPTX.homSingle.hetSingle],
              // isS=[false])
          .join(
              rel2895,
              [&](const auto &build_arg) -> expression_t {
                return expressions::RecordConstruction{
                    build_arg["$0"].as("PelagoJoin#2899", "bk_0"),
                    build_arg["$1"].as("PelagoJoin#2899", "bk_1"),
                    build_arg["$2"].as("PelagoJoin#2899", "bk_2"),
                    build_arg["$3"].as("PelagoJoin#2899", "bk_3")}
                    .as("PelagoJoin#2899", "bk");
              },
              [&](const auto &probe_arg) -> expression_t {
                return expressions::RecordConstruction{
                    probe_arg["$0"].as("PelagoJoin#2899", "pk_0"),
                    probe_arg["$1"].as("PelagoJoin#2899", "pk_1"),
                    probe_arg["$2"].as("PelagoJoin#2899", "pk_2"),
                    probe_arg["$3"].as("PelagoJoin#2899", "pk_3")}
                    .as("PelagoJoin#2899", "pk");
              },
              28,
              3000000)  // (condition=[AND(=($4, $0), =($5, $1), =($6, $2),
                        // =($7, $3))], joinType=[inner], rowcnt=[7.68E8],
                        // maxrow=[3000000.0], maxEst=[3000000.0], h_bits=[28],
                        // build=[RecordType(BIGINT o_id, INTEGER o_d_id,
                        // INTEGER o_w_id, TIMESTAMP(0) o_entry_d)],
                        // lcount=[6.7109666771656204E10], rcount=[7.68E8],
                        // buildcountrow=[7.68E8], probecountrow=[7.68E8])
          .project([&](const auto &arg) -> std::vector<expression_t> {
            return {(arg["$8"]).as("PelagoProject#2901", "$0"),
                    (arg["$0"]).as("PelagoProject#2901", "$1"),
                    (arg["$1"]).as("PelagoProject#2901", "$2"),
                    (arg["$2"]).as("PelagoProject#2901", "$3"),
                    (arg["$3"]).as("PelagoProject#2901", "$4")};
          })         // (o_ol_cnt=[$4], o_id0=[$5], o_d_id0=[$6], o_w_id0=[$7],
                     // o_entry_d0=[$8],
                     // trait=[Pelago.[].unpckd.NVPTX.homSingle.hetSingle])
          .pack()    // (trait=[Pelago.[].packed.NVPTX.homSingle.hetSingle],
                     // intrait=[Pelago.[].unpckd.NVPTX.homSingle.hetSingle],
                     // inputRows=[1.92E8], cost=[{368.368 rows, 368.0 cpu, 0.0
                     // io}])
          .to_cpu()  // (trait=[Pelago.[].packed.X86_64.homSingle.hetSingle])
          .membrdcst(dop, true, true)
          .router(
              [&](const auto &arg) -> std::optional<expression_t> {
                return arg["__broadcastTarget"];
              },
              dop, 1, RoutingPolicy::HASH_BASED, dev,
              aff_parallel())  // (trait=[Pelago.[].packed.X86_64.homBrdcst.hetSingle])
      ;

  if (memmv) rel2906 = rel2906.memmove(8, dev);

  rel2906 =
      rel2906
          .to_gpu()  // (trait=[Pelago.[].packed.NVPTX.homBrdcst.hetSingle])
          .unpack()  // (trait=[Pelago.[].unpckd.NVPTX.homBrdcst.hetSingle])
      ;

  auto rel =
      getBuilder<Tplugin>()
          .scan(
              "inputs/ch100w/orderline.csv",
              {"ol_o_id", "ol_d_id", "ol_w_id", "ol_delivery_d"}, getCatalog(),
              pg{Tplugin::
                     type})  // (table=[[SSB, ch100w_orderline]], fields=[[0, 1,
                             // 2, 6]],
                             // traits=[Pelago.[].packed.X86_64.homSingle.hetSingle])
          .router(
              dop, 1, RoutingPolicy::LOCAL, dev,
              aff_parallel())  // (trait=[Pelago.[].packed.X86_64.homRandom.hetSingle])
      ;

  if (memmv) rel = rel.memmove(8, dev);

  rel =
      rel.to_gpu()   // (trait=[Pelago.[].packed.NVPTX.homRandom.hetSingle])
          .unpack()  // (trait=[Pelago.[].unpckd.NVPTX.homRandom.hetSingle])
          .join(
              rel2906,
              [&](const auto &build_arg) -> expression_t {
                return expressions::RecordConstruction{
                    build_arg["$1"].as("PelagoJoin#2910", "bk_1"),
                    build_arg["$3"].as("PelagoJoin#2910", "bk_3"),
                    build_arg["$2"].as("PelagoJoin#2910", "bk_2")}
                    .as("PelagoJoin#2910", "bk");
              },
              [&](const auto &probe_arg) -> expression_t {
                return expressions::RecordConstruction{
                    probe_arg["$0"].as("PelagoJoin#2910", "pk_0"),
                    probe_arg["$2"].as("PelagoJoin#2910", "pk_2"),
                    probe_arg["$1"].as("PelagoJoin#2910", "pk_1")}
                    .as("PelagoJoin#2910", "pk");
              },
              28,
              67108864)  // (condition=[AND(=($1, $5), =($3, $7), =($2, $6))],
                         // joinType=[inner], rowcnt=[1.92937984E8],
                         // maxrow=[9.0E12], maxEst=[6.7108864E7], h_bits=[28],
                         // build=[RecordType(INTEGER o_ol_cnt, BIGINT o_id0,
                         // INTEGER o_d_id0, INTEGER o_w_id0, TIMESTAMP(0)
                         // o_entry_d0)], lcount=[1.9956846453055954E10],
                         // rcount=[1.536E10], buildcountrow=[1.92937984E8],
                         // probecountrow=[1.536E10])
          .filter([&](const auto &arg) -> expression_t {
            return ge(arg["ol_delivery_d"], arg["o_entry_d"]);
          })  // (condition=[>=($8, $4)],
              // trait=[Pelago.[].unpckd.NVPTX.homRandom.hetSingle],
              // isS=[false])
          .groupby(
              [&](const auto &arg) -> std::vector<expression_t> {
                return {arg["o_ol_cnt"].as("PelagoAggregate#2913", "$0")};
              },
              [&](const auto &arg) -> std::vector<GpuAggrMatExpr> {
                return {GpuAggrMatExpr{
                    (expression_t{1}).as("PelagoAggregate#2913", "$1"), 1, 0,
                    SUM}};
              },
              10,
              131072)  // (group=[{0}], order_count=[COUNT()],
                       // trait=[Pelago.[].unpckd.NVPTX.homRandom.hetSingle])
          .to_cpu()    // (trait=[Pelago.[].packed.X86_64.homRandom.hetSingle])
          .router(
              DegreeOfParallelism{1}, 128, RoutingPolicy::RANDOM,
              DeviceType::CPU,
              aff_reduce())  // (trait=[Pelago.[].packed.X86_64.homSingle.hetSingle])
          .groupby(
              [&](const auto &arg) -> std::vector<expression_t> {
                return {arg["$0"].as("PelagoAggregate#2919", "$0")};
              },
              [&](const auto &arg) -> std::vector<GpuAggrMatExpr> {
                return {GpuAggrMatExpr{
                    (arg["$1"]).as("PelagoAggregate#2919", "$1"), 1, 0, SUM}};
              },
              10,
              131072)  // (group=[{0}], order_count=[$SUM0($1)],
                       // trait=[Pelago.[].unpckd.NVPTX.homSingle.hetSingle])
          .sort(
              [&](const auto &arg) -> std::vector<expression_t> {
                return {arg["$0"].as("tmp", "o_ol_cnt"),
                        arg["$1"].as("tmp", "order_count")};
              },
              {
                  direction::NONE,
                  direction::ASC,
              })  // (sort0=[$0], dir0=[ASC],
                  // trait=[Pelago.[0].unpckd.X86_64.homSingle.hetSingle])
          .print(
              pg{"pm-csv"},
              std::string{query} +
                  (memmv
                       ? "mv"
                       : "nmv"))  // (trait=[ENUMERABLE.[0].unpckd.X86_64.homSingle.hetSingle])
      ;

  return rel.prepare();
}
