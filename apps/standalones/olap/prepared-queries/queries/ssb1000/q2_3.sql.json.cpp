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

constexpr auto query = "ssb1000_Q2_3";

#include "query.cpp.inc"

PreparedStatement Query::prepare23(bool memmv) {
  auto rel23990 =
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
  auto rel23995 =
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
            return eq(arg["$1"], "EUROPE");
          })  // (condition=[=($1, 'EUROPE')],
              // trait=[Pelago.[].unpckd.NVPTX.homBrdcst.hetSingle],
              // isS=[false])
          .project([&](const auto &arg) -> std::vector<expression_t> {
            return {(arg["$0"]).as("PelagoProject#23995", "$0")};
          })  // (s_suppkey=[$0],
              // trait=[Pelago.[].unpckd.NVPTX.homBrdcst.hetSingle])
      ;
  auto rel23999 =
      getBuilder<Tplugin>()
          .scan<Tplugin>(
              "inputs/ssbm1000/part.csv", {"p_partkey", "p_brand1"},
              getCatalog())  // (table=[[SSB, ssbm_part]], fields=[[0, 4]],
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
            return eq(arg["$1"], "MFGR#2239");
          })  // (condition=[=($1, 'MFGR#2239')],
              // trait=[Pelago.[].unpckd.NVPTX.homBrdcst.hetSingle],
              // isS=[false])
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
          .filter([&](const auto &arg) -> expression_t {
            return eq(arg["lo_partkey"], expression_t{0});
          })  // (condition=[=($1, 'MFGR#2239')],
          .join(
              rel23999,
              [&](const auto &build_arg) -> expression_t {
                return build_arg["$0"];
              },
              [&](const auto &probe_arg) -> expression_t {
                return probe_arg["lo_partkey"];
              },
              12,
              2048)  // (condition=[=($2, $0)], joinType=[inner],
                     // rowcnt=[1433600.0], maxrow=[1400000.0],
                     // maxEst=[1400000.0], h_bits=[23],
                     // build=[RecordType(INTEGER p_partkey, VARCHAR
                     // p_brand1)], lcount=[4.263195668985415E7],
                     // rcount=[3.0721953024E11], buildcountrow=[1433600.0],
                     // probecountrow=[3.0721953024E11])
          .join(
              rel23995,
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
                       // lcount=[7.179512438249687E8], rcount=[3.0721953024E8],
                       // buildcountrow=[4.096E7],
                       // probecountrow=[3.0721953024E8])
          .join(
              rel23990,
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
                     // rcount=[6.144390604800001E7], buildcountrow=[2617344.0],
                     // probecountrow=[6.144390604800001E7])
          .groupby(
              [&](const auto &arg) -> std::vector<expression_t> {
                return {arg["d_year"].as("PelagoAggregate#24009", "$0"),
                        arg["p_brand1"].as("PelagoAggregate#24009", "$1")};
              },
              [&](const auto &arg) -> std::vector<GpuAggrMatExpr> {
                return {GpuAggrMatExpr{
                    (arg["lo_revenue"]).as("PelagoAggregate#24009", "$2"), 1, 0,
                    SUM}};
              },
              4,
              131072)  // (group=[{0, 1}], lo_revenue=[SUM($2)],
                       // trait=[Pelago.[].unpckd.NVPTX.homRandom.hetSingle])
                       //          .pack()      //
          //          (trait=[Pelago.[].packed.NVPTX.homRandom.hetSingle],
          // intrait=[Pelago.[].unpckd.NVPTX.homRandom.hetSingle],
          // inputRows=[6144390.604800001], cost=[{7.2072 rows, 7.2 cpu,
          // 0.0 io}])
          .to_cpu()  // (trait=[Pelago.[].packed.X86_64.homRandom.hetSingle])
          .router(
              DegreeOfParallelism{1}, 128, RoutingPolicy::RANDOM,
              DeviceType::CPU,
              aff_reduce())  // (trait=[Pelago.[].packed.X86_64.homSingle.hetSingle])
                             //          .memmove(8, DeviceType::CPU)
                             //          .unpack()  //
          //          (trait=[Pelago.[].unpckd.NVPTX.homSingle.hetSingle])
          .groupby(
              [&](const auto &arg) -> std::vector<expression_t> {
                return {arg["$0"].as("PelagoAggregate#24015", "$0"),
                        arg["$1"].as("PelagoAggregate#24015", "$1")};
              },
              [&](const auto &arg) -> std::vector<GpuAggrMatExpr> {
                return {GpuAggrMatExpr{
                    (arg["$2"]).as("PelagoAggregate#24015", "$2"), 1, 0, SUM}};
              },
              4,
              16)  // (group=[{0, 1}], lo_revenue=[SUM($2)],
                   // trait=[Pelago.[].unpckd.NVPTX.homSingle.hetSingle])
          .project([&](const auto &arg) -> std::vector<expression_t> {
            return {(arg["$2"]).as("PelagoProject#11437", "EXPR$0"),
                    (arg["$0"]).as("PelagoProject#11437", "$1"),
                    (arg["$1"]).as("PelagoProject#11437", "$2")};
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
                return {arg["EXPR$0"].as(outrel, "lo_revenue"),
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
