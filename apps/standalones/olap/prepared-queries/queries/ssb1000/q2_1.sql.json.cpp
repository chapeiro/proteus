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

constexpr auto query = "ssb1000_Q2_1";

#include "query.cpp.inc"

PreparedStatement Query::prepare21(bool memmv) {
  auto rel11407 =
      getBuilder<Tplugin>()
          .scan<Tplugin>(
              "inputs/ssbm1000/date.csv", {"d_datekey", "d_year"},
              getCatalog())  // (table=[[SSB, ssbm_date]], fields=[[0, 4]],
                             // traits=[Pelago.[].packed.X86_64.homSingle.hetSingle])
          .membrdcst(dop, true, true)
          .router(
              [&](const auto &arg) -> std::optional<expression_t> {
                return arg["__broadcastTarget"];
              },
              dop, 1, RoutingPolicy::HASH_BASED, dev,
              aff_parallel())  // (trait=[Pelago.[].packed.X86_64.homBrdcst.hetSingle])
          .to_gpu()  // (trait=[Pelago.[].packed.NVPTX.homBrdcst.hetSingle])
          .unpack()  // (trait=[Pelago.[].unpckd.NVPTX.homBrdcst.hetSingle])
      ;
  auto rel11412 =
      getBuilder<Tplugin>()
          .scan<Tplugin>(
              "inputs/ssbm1000/supplier.csv", {"s_suppkey", "s_region"},
              getCatalog())  // (table=[[SSB, ssbm_supplier]], fields=[[0, 5]],
                             // traits=[Pelago.[].packed.X86_64.homSingle.hetSingle])
          .membrdcst(dop, true, true)
          .router(
              [&](const auto &arg) -> std::optional<expression_t> {
                return arg["__broadcastTarget"];
              },
              dop, 1, RoutingPolicy::HASH_BASED, dev,
              aff_parallel())  // (trait=[Pelago.[].packed.X86_64.homBrdcst.hetSingle])
          .to_gpu()  // (trait=[Pelago.[].packed.NVPTX.homBrdcst.hetSingle])
          .unpack()  // (trait=[Pelago.[].unpckd.NVPTX.homBrdcst.hetSingle])
          .filter([&](const auto &arg) -> expression_t {
            return eq(arg["$1"], "AMERICA");
          })  // (condition=[=($1, 'AMERICA')],
              // trait=[Pelago.[].unpckd.NVPTX.homBrdcst.hetSingle],
              // isS=[false])
          .project([&](const auto &arg) -> std::vector<expression_t> {
            return {(arg["$0"]).as("PelagoProject#11412", "$0")};
          })  // (s_suppkey=[$0],
              // trait=[Pelago.[].unpckd.NVPTX.homBrdcst.hetSingle])
      ;
  auto rel11417 =
      getBuilder<Tplugin>()
          .scan<Tplugin>(
              "inputs/ssbm1000/part.csv",
              {"p_partkey", "p_category", "p_brand1"},
              getCatalog())  // (table=[[SSB, ssbm_part]], fields=[[0, 3, 4]],
                             // traits=[Pelago.[].packed.X86_64.homSingle.hetSingle])
          .membrdcst(dop, true, true)
          .router(
              [&](const auto &arg) -> std::optional<expression_t> {
                return arg["__broadcastTarget"];
              },
              dop, 1, RoutingPolicy::HASH_BASED, dev,
              aff_parallel())  // (trait=[Pelago.[].packed.X86_64.homBrdcst.hetSingle])
          .to_gpu()  // (trait=[Pelago.[].packed.NVPTX.homBrdcst.hetSingle])
          .unpack()  // (trait=[Pelago.[].unpckd.NVPTX.homBrdcst.hetSingle])
          .filter([&](const auto &arg) -> expression_t {
            return eq(arg["$1"], "MFGR#12");
          })  // (condition=[=($1, 'MFGR#12')],
              // trait=[Pelago.[].unpckd.NVPTX.homBrdcst.hetSingle],
              // isS=[false])
          .project([&](const auto &arg) -> std::vector<expression_t> {
            return {(arg["$0"]).as("PelagoProject#11417", "$0"),
                    (arg["$2"]).as("PelagoProject#11417", "p_brand1")};
          })  // (p_partkey=[$0], p_brand1=[$2],
              // trait=[Pelago.[].unpckd.NVPTX.homBrdcst.hetSingle])
      ;
  auto rel =
      getBuilder<Tplugin>()
          .scan<Tplugin>(
              "inputs/ssbm1000/lineorder.csv",
              {"lo_partkey", "lo_suppkey", "lo_orderdate", "lo_revenue"},
              getCatalog())  // (table=[[SSB, ssbm_lineorder]], fields=[[3, 4,
                             // 5, 12]],
                             // traits=[Pelago.[].packed.X86_64.homSingle.hetSingle])
          .router(
              dop, 8, RoutingPolicy::LOCAL, dev,
              aff_parallel())  // (trait=[Pelago.[].packed.X86_64.homRandom.hetSingle])
      ;

  if (memmv) rel = rel.memmove(8, dev);

  rel =
      rel.to_gpu()   // (trait=[Pelago.[].packed.NVPTX.homRandom.hetSingle])
          .unpack()  // (trait=[Pelago.[].unpckd.NVPTX.homRandom.hetSingle])
          .join(
              rel11417,
              [&](const auto &build_arg) -> expression_t {
                return build_arg["$0"];
              },
              [&](const auto &probe_arg) -> expression_t {
                return probe_arg["lo_partkey"];
              },
              18,
              131072)  // (condition=[=($2, $0)], joinType=[inner],
                       // rowcnt=[5.7344E7], maxrow=[1400000.0],
                       // maxEst=[1400000.0], h_bits=[28],
                       // build=[RecordType(INTEGER p_partkey, VARCHAR
                       // p_brand1)], lcount=[2.1283484744275851E9],
                       // rcount=[3.0721953024E11], buildcountrow=[5.7344E7],
                       // probecountrow=[3.0721953024E11])
          .join(
              rel11412,
              [&](const auto &build_arg) -> expression_t {
                return build_arg["$0"];
              },
              [&](const auto &probe_arg) -> expression_t {
                return probe_arg["lo_suppkey"];
              },
              20,
              524288)  // (condition=[=($1, $0)], joinType=[inner],
                       // rowcnt=[4.096E7], maxrow=[200000.0],
                       // maxEst=[200000.0], h_bits=[28],
                       // build=[RecordType(INTEGER s_suppkey)],
                       // lcount=[7.179512438249687E8],
                       // rcount=[1.22887812096E10], buildcountrow=[4.096E7],
                       // probecountrow=[1.22887812096E10])
          .join(
              rel11407,
              [&](const auto &build_arg) -> expression_t {
                return build_arg["$0"];
              },
              [&](const auto &probe_arg) -> expression_t {
                return probe_arg["lo_orderdate"];
              },
              14,
              2556)  // (condition=[=($2, $0)], joinType=[inner],
                     // rowcnt=[2617344.0], maxrow=[2556.0], maxEst=[2556.0],
                     // h_bits=[24], build=[RecordType(INTEGER d_datekey,
                     // INTEGER d_year)], lcount=[8.098490429651935E7],
                     // rcount=[2.45775624192E9], buildcountrow=[2617344.0],
                     // probecountrow=[2.45775624192E9])
          .groupby(
              [&](const auto &arg) -> std::vector<expression_t> {
                return {arg["d_year"].as("PelagoAggregate#11427", "$0"),
                        arg["p_brand1"].as("PelagoAggregate#11427", "$1")};
              },
              [&](const auto &arg) -> std::vector<GpuAggrMatExpr> {
                return {GpuAggrMatExpr{
                    (arg["lo_revenue"]).as("PelagoAggregate#11427", "$2"), 1, 0,
                    SUM}};
              },
              10,
              131072)  // (group=[{0, 1}], EXPR$0=[SUM($2)],
                       // trait=[Pelago.[].unpckd.NVPTX.homRandom.hetSingle])
          .pack()      // (trait=[Pelago.[].packed.NVPTX.homRandom.hetSingle],
                       // intrait=[Pelago.[].unpckd.NVPTX.homRandom.hetSingle],
          // inputRows=[2.45775624192E8], cost=[{282.28200000000004 rows,
          // 282.0 cpu, 0.0 io}])
          .to_cpu()
          .router(
              DegreeOfParallelism{1}, 128, RoutingPolicy::RANDOM,
              DeviceType::CPU,
              aff_reduce())  // (trait=[Pelago.[].packed.X86_64.homSingle.hetSingle])
          .memmove(8, DeviceType::CPU)
          .unpack()  // (trait=[Pelago.[].unpckd.NVPTX.homSingle.hetSingle])
          .groupby(
              [&](const auto &arg) -> std::vector<expression_t> {
                return {arg["$0"].as("PelagoAggregate#11433", "$0"),
                        arg["$1"].as("PelagoAggregate#11433", "$1")};
              },
              [&](const auto &arg) -> std::vector<GpuAggrMatExpr> {
                return {GpuAggrMatExpr{
                    (arg["$2"]).as("PelagoAggregate#11433", "$2"), 1, 0, SUM}};
              },
              10,
              131072)  // (group=[{0, 1}], EXPR$0=[SUM($2)],
                       // trait=[Pelago.[].unpckd.NVPTX.homSingle.hetSingle])
          .project([&](const auto &arg) -> std::vector<expression_t> {
            return {(arg["$2"]).as("PelagoProject#11438", "EXPR$0"),
                    (arg["$0"]).as("PelagoProject#11438", "$1"),
                    (arg["$1"]).as("PelagoProject#11438", "$2")};
          })  // (EXPR$0=[$2], d_year=[$0], p_brand1=[$1],
              // trait=[Pelago.[].unpckd.X86_64.homSingle.hetSingle])
          .sort(
              [&](const auto &arg) -> std::vector<expression_t> {
                return {arg["EXPR$0"], arg["$1"], arg["$2"]};
              },
              {direction::NONE, direction::ASC,
               direction::ASC})  // (sort0=[$1], sort1=[$2], dir0=[ASC],
                                 // dir1=[ASC], trait=[Pelago.[1,
                                 // 2].unpckd.X86_64.homSingle.hetSingle])
          .print(
              [&](const auto &arg,
                  std::string outrel) -> std::vector<expression_t> {
                return {arg["EXPR$0"].as(outrel, "EXPR$0"),
                        arg["$1"].as(outrel, "d_year"),
                        arg["$2"].as(outrel, "p_brand1")};
              },
              std::string{query} +
                  (memmv ? "mv"
                         : "nmv"))  // (trait=[ENUMERABLE.[1,
                                    // 2].unpckd.X86_64.homSingle.hetSingle])
      ;
  return rel.prepare();
}
