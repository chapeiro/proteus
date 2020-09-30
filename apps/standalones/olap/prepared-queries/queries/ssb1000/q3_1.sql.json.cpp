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

constexpr auto query = "ssb1000_Q3_1";

#include "query.cpp.inc"

PreparedStatement Query::prepare31(bool memmv) {
  auto rel29287 =
      getBuilder<Tplugin>()
          .scan(
              "inputs/ssbm1000/date.csv", {"d_datekey", "d_year"}, getCatalog(),
              pg{Tplugin::
                     type})  // (table=[[SSB, ssbm_date]], fields=[[0, 4]],
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
            return (ge(arg["$1"], 1992) & le(arg["$1"], 1997));
          })  // (condition=[AND(>=($1, 1992), <=($1, 1997))],
              // trait=[Pelago.[].unpckd.NVPTX.homBrdcst.hetSingle],
              // isS=[false])
      ;
  auto rel29292 =
      getBuilder<Tplugin>()
          .scan(
              "inputs/ssbm1000/customer.csv",
              {"c_custkey", "c_nation", "c_region"}, getCatalog(),
              pg{Tplugin::
                     type})  // (table=[[SSB, ssbm_customer]], fields=[[0, 4,
                             // 5]],
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
            return eq(arg["$2"], "ASIA");
          })  // (condition=[=($2, 'ASIA')],
              // trait=[Pelago.[].unpckd.NVPTX.homBrdcst.hetSingle],
              // isS=[false])
          .project([&](const auto &arg) -> std::vector<expression_t> {
            return {(arg["$0"]).as("PelagoProject#29292", "$0"),
                    (arg["$1"]).as("PelagoProject#29292", "c_nation")};
          })  // (c_custkey=[$0], c_nation=[$1],
              // trait=[Pelago.[].unpckd.NVPTX.homBrdcst.hetSingle])
      ;
  auto rel29297 =
      getBuilder<Tplugin>()
          .scan(
              "inputs/ssbm1000/supplier.csv",
              {"s_suppkey", "s_nation", "s_region"}, getCatalog(),
              pg{Tplugin::
                     type})  // (table=[[SSB, ssbm_supplier]], fields=[[0, 4,
                             // 5]],
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
            return eq(arg["$2"], "ASIA");
          })  // (condition=[=($2, 'ASIA')],
              // trait=[Pelago.[].unpckd.NVPTX.homBrdcst.hetSingle],
              // isS=[false])
          .project([&](const auto &arg) -> std::vector<expression_t> {
            return {(arg["$0"]).as("PelagoProject#29297", "$0"),
                    (arg["$1"]).as("PelagoProject#29297", "s_nation")};
          })  // (s_suppkey=[$0], s_nation=[$1],
              // trait=[Pelago.[].unpckd.NVPTX.homBrdcst.hetSingle])
      ;
  auto rel =
      getBuilder<Tplugin>()
          .scan(
              "inputs/ssbm1000/lineorder.csv",
              {"lo_custkey", "lo_suppkey", "lo_orderdate", "lo_revenue"},
              getCatalog(),
              pg{Tplugin::
                     type})  // (table=[[SSB, ssbm_lineorder]], fields=[[2, 4,
                             // 5, 12]],
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
              rel29297,
              [&](const auto &build_arg) -> expression_t {
                return build_arg["$0"];
              },
              [&](const auto &probe_arg) -> expression_t {
                return probe_arg["lo_suppkey"];
              },
              20,
              524288)  // (condition=[=($3, $0)], joinType=[inner],
                       // rowcnt=[4.096E7], maxrow=[200000.0],
                       // maxEst=[200000.0], h_bits=[28],
                       // build=[RecordType(INTEGER s_suppkey, VARCHAR
                       // s_nation)], lcount=[1.4926851046814082E9],
                       // rcount=[3.0721953024E11], buildcountrow=[4.096E7],
                       // probecountrow=[3.0721953024E11])
          .join(
              rel29292,
              [&](const auto &build_arg) -> expression_t {
                return build_arg["$0"];
              },
              [&](const auto &probe_arg) -> expression_t {
                return probe_arg["lo_custkey"];
              },
              24,
              8388608)  // (condition=[=($2, $0)], joinType=[inner],
                        // rowcnt=[6.144E8], maxrow=[3000000.0],
                        // maxEst=[3000000.0], h_bits=[28],
                        // build=[RecordType(INTEGER c_custkey, VARCHAR
                        // c_nation)], lcount=[2.571792865733552E10],
                        // rcount=[6.1443906048E10], buildcountrow=[6.144E8],
                        // probecountrow=[6.1443906048E10])
          .join(
              rel29287,
              [&](const auto &build_arg) -> expression_t {
                return build_arg["$0"];
              },
              [&](const auto &probe_arg) -> expression_t {
                return probe_arg["lo_orderdate"];
              },
              14,
              2556)  // (condition=[=($2, $0)], joinType=[inner],
                     // rowcnt=[654336.0], maxrow=[2556.0], maxEst=[2556.0],
                     // h_bits=[22], build=[RecordType(INTEGER d_datekey,
                     // INTEGER d_year)], lcount=[1.8432021459974352E7],
                     // rcount=[1.22887812096E10], buildcountrow=[654336.0],
                     // probecountrow=[1.22887812096E10])
          .groupby(
              [&](const auto &arg) -> std::vector<expression_t> {
                return {arg["c_nation"].as("PelagoAggregate#29307", "$0"),
                        arg["s_nation"].as("PelagoAggregate#29307", "$1"),
                        arg["d_year"].as("PelagoAggregate#29307", "$2")};
              },
              [&](const auto &arg) -> std::vector<GpuAggrMatExpr> {
                return {GpuAggrMatExpr{
                    (arg["lo_revenue"]).as("PelagoAggregate#29307", "$3"), 1, 0,
                    SUM}};
              },
              10,
              131072)  // (group=[{0, 1, 2}], lo_revenue=[SUM($3)],
                       // trait=[Pelago.[].unpckd.NVPTX.homRandom.hetSingle])
          .pack()      // (trait=[Pelago.[].packed.NVPTX.homRandom.hetSingle],
                       // intrait=[Pelago.[].unpckd.NVPTX.homRandom.hetSingle],
          // inputRows=[3.0721953024E8], cost=[{469.26880000000006 rows,
          // 468.8 cpu, 0.0 io}])
          .to_cpu()  // (trait=[Pelago.[].packed.X86_64.homRandom.hetSingle])
          .router(
              DegreeOfParallelism{1}, 128, RoutingPolicy::RANDOM,
              DeviceType::CPU,
              aff_reduce())  // (trait=[Pelago.[].packed.X86_64.homSingle.hetSingle])
          .memmove(8, DeviceType::CPU)
          .unpack()  // (trait=[Pelago.[].unpckd.NVPTX.homSingle.hetSingle])
          .groupby(
              [&](const auto &arg) -> std::vector<expression_t> {
                return {arg["$0"].as("PelagoAggregate#29313", "$0"),
                        arg["$1"].as("PelagoAggregate#29313", "$1"),
                        arg["$2"].as("PelagoAggregate#29313", "$2")};
              },
              [&](const auto &arg) -> std::vector<GpuAggrMatExpr> {
                return {GpuAggrMatExpr{
                    (arg["$3"]).as("PelagoAggregate#29313", "$3"), 1, 0, SUM}};
              },
              10,
              131072)  // (group=[{0, 1, 2}], lo_revenue=[SUM($3)],
                       // trait=[Pelago.[].unpckd.NVPTX.homSingle.hetSingle])
          .sort(
              [&](const auto &arg) -> std::vector<expression_t> {
                return {arg["$0"], arg["$1"], arg["$2"], arg["$3"]};
              },
              {direction::NONE, direction::NONE, direction::ASC,
               direction::DESC})  // (sort0=[$2], sort1=[$3], dir0=[ASC],
                                  // dir1=[DESC], trait=[Pelago.[2, 3
                                  // DESC].unpckd.X86_64.homSingle.hetSingle])
          .print(
              [&](const auto &arg,
                  std::string outrel) -> std::vector<expression_t> {
                return {arg["$0"].as(outrel, "c_nation"),
                        arg["$1"].as(outrel, "s_nation"),
                        arg["$2"].as(outrel, "d_year"),
                        arg["$3"].as(outrel, "lo_revenue")};
              },
              std::string{query} +
                  (memmv ? "mv"
                         : "nmv"))  // (trait=[ENUMERABLE.[2, 3
                                    // DESC].unpckd.X86_64.homSingle.hetSingle])
      ;
  return rel.prepare();
}
