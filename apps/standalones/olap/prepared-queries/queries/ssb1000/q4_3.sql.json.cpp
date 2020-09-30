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

constexpr auto query = "ssb1000_Q4_3";

#include "query.cpp.inc"

PreparedStatement Query::prepare43(bool memmv) {
  auto rel64283 =
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
            return (eq(arg["$1"], 1997) | eq(arg["$1"], 1998));
          })  // (condition=[OR(=($1, 1997), =($1, 1998))],
              // trait=[Pelago.[].unpckd.NVPTX.homBrdcst.hetSingle],
              // isS=[false])
      ;
  auto rel64288 =
      getBuilder<Tplugin>()
          .scan(
              "inputs/ssbm1000/customer.csv", {"c_custkey", "c_region"},
              getCatalog(),
              pg{Tplugin::
                     type})  // (table=[[SSB, ssbm_customer]], fields=[[0, 5]],
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
            return {(arg["$0"]).as("PelagoProject#64288", "$0")};
          })  // (c_custkey=[$0],
              // trait=[Pelago.[].unpckd.NVPTX.homBrdcst.hetSingle])
      ;
  auto rel64293 =
      getBuilder<Tplugin>()
          .scan(
              "inputs/ssbm1000/part.csv",
              {"p_partkey", "p_category", "p_brand1"}, getCatalog(),
              pg{Tplugin::
                     type})  // (table=[[SSB, ssbm_part]], fields=[[0, 3, 4]],
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
            return eq(arg["$1"], "MFGR#14");
          })  // (condition=[=($1, 'MFGR#14')],
              // trait=[Pelago.[].unpckd.NVPTX.homBrdcst.hetSingle],
              // isS=[false])
          .project([&](const auto &arg) -> std::vector<expression_t> {
            return {(arg["$0"]).as("PelagoProject#64293", "$0"),
                    (arg["$2"]).as("PelagoProject#64293", "p_brand1")};
          })  // (p_partkey=[$0], p_brand1=[$2],
              // trait=[Pelago.[].unpckd.NVPTX.homBrdcst.hetSingle])
      ;
  auto rel64298 =
      getBuilder<Tplugin>()
          .scan(
              "inputs/ssbm1000/supplier.csv",
              {"s_suppkey", "s_city", "s_nation"}, getCatalog(),
              pg{Tplugin::
                     type})  // (table=[[SSB, ssbm_supplier]], fields=[[0, 3,
                             // 4]],
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
            return eq(arg["$2"], "UNITED STATES");
          })  // (condition=[=($2, 'UNITED STATES')],
              // trait=[Pelago.[].unpckd.NVPTX.homBrdcst.hetSingle],
              // isS=[false])
          .project([&](const auto &arg) -> std::vector<expression_t> {
            return {(arg["$0"]).as("PelagoProject#64298", "$0"),
                    (arg["$1"]).as("PelagoProject#64298", "s_city")};
          })  // (s_suppkey=[$0], s_city=[$1],
              // trait=[Pelago.[].unpckd.NVPTX.homBrdcst.hetSingle])
      ;
  auto rel =
      getBuilder<Tplugin>()
          .scan(
              "inputs/ssbm1000/lineorder.csv",
              {"lo_custkey", "lo_partkey", "lo_suppkey", "lo_orderdate",
               "lo_revenue", "lo_supplycost"},
              getCatalog(),
              pg{Tplugin::
                     type})  // (table=[[SSB, ssbm_lineorder]], fields=[[2, 3,
                             // 4, 5, 12, 13]],
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
              rel64298,
              [&](const auto &build_arg) -> expression_t {
                return build_arg["$0"];
              },
              [&](const auto &probe_arg) -> expression_t {
                return probe_arg["lo_suppkey"];
              },
              17,
              128 *
                  1024)  // (condition=[=($4, $0)], joinType=[inner],
                         // rowcnt=[8192000.0], maxrow=[200000.0],
                         // maxEst=[200000.0], h_bits=[25],
                         // build=[RecordType(INTEGER s_suppkey, VARCHAR
                         // s_city)], lcount=[2.7216799017896134E8],
                         // rcount=[3.0721953024E11], buildcountrow=[8192000.0],
                         // probecountrow=[3.0721953024E11])
          .join(
              rel64293,
              [&](const auto &build_arg) -> expression_t {
                return build_arg["$0"];
              },
              [&](const auto &probe_arg) -> expression_t {
                return probe_arg["lo_partkey"];
              },
              17,
              128 *
                  1024)  // (condition=[=($3, $0)], joinType=[inner],
                         // rowcnt=[5.7344E7], maxrow=[1400000.0],
                         // maxEst=[1400000.0], h_bits=[28],
                         // build=[RecordType(INTEGER p_partkey, VARCHAR
                         // p_brand1)], lcount=[2.1283484744275851E9],
                         // rcount=[1.22887812096E10], buildcountrow=[5.7344E7],
                         // probecountrow=[1.22887812096E10])
          .join(
              rel64288,
              [&](const auto &build_arg) -> expression_t {
                return build_arg["$0"];
              },
              [&](const auto &probe_arg) -> expression_t {
                return probe_arg["lo_custkey"];
              },
              24,
              8 * 1024 *
                  1024)  // (condition=[=($1, $0)], joinType=[inner],
                         // rowcnt=[6.144E8], maxrow=[3000000.0],
                         // maxEst=[3000000.0], h_bits=[28],
                         // build=[RecordType(INTEGER c_custkey)],
                         // lcount=[1.2433094700931728E10],
                         // rcount=[4.91551248384E8], buildcountrow=[6.144E8],
                         // probecountrow=[4.91551248384E8])
          .join(
              rel64283,
              [&](const auto &build_arg) -> expression_t {
                return build_arg["$0"];
              },
              [&](const auto &probe_arg) -> expression_t {
                return probe_arg["lo_orderdate"];
              },
              11,
              1024)  // (condition=[=($2, $0)], joinType=[inner],
                     // rowcnt=[785203.2], maxrow=[2556.0], maxEst=[2556.0],
                     // h_bits=[22], build=[RecordType(INTEGER d_datekey,
                     // INTEGER d_year)], lcount=[2.2404744691616405E7],
                     // rcount=[983040.0], buildcountrow=[785203.2],
                     // probecountrow=[983040.0])
          .groupby(
              [&](const auto &arg) -> std::vector<expression_t> {
                return {arg["d_year"].as("PelagoAggregate#64311", "$0"),
                        arg["s_city"].as("PelagoAggregate#64311", "$1"),
                        arg["p_brand1"].as("PelagoAggregate#64311", "$2")};
              },
              [&](const auto &arg) -> std::vector<GpuAggrMatExpr> {
                return {
                    GpuAggrMatExpr{(arg["lo_revenue"] - arg["lo_supplycost"])
                                       .as("PelagoAggregate#64311", "$3"),
                                   1, 0, SUM}};
              },
              10,
              131072)  // (group=[{0, 1, 2}], profit=[SUM($3)],
                       // trait=[Pelago.[].unpckd.NVPTX.homRandom.hetSingle])
          .pack()      // (trait=[Pelago.[].packed.NVPTX.homRandom.hetSingle],
                       // intrait=[Pelago.[].unpckd.NVPTX.homRandom.hetSingle],
          // inputRows=[29491.2], cost=[{1.6016 rows, 1.6 cpu, 0.0 io}])
          .to_cpu()  // (trait=[Pelago.[].packed.X86_64.homRandom.hetSingle])
          .router(
              DegreeOfParallelism{1}, 128, RoutingPolicy::RANDOM,
              DeviceType::CPU,
              aff_reduce())  // (trait=[Pelago.[].packed.X86_64.homSingle.hetSingle])
          .memmove(8, DeviceType::CPU)
          .unpack()  // (trait=[Pelago.[].unpckd.NVPTX.homSingle.hetSingle])
          .groupby(
              [&](const auto &arg) -> std::vector<expression_t> {
                return {arg["$0"].as("PelagoAggregate#64317", "$0"),
                        arg["$1"].as("PelagoAggregate#64317", "$1"),
                        arg["$2"].as("PelagoAggregate#64317", "$2")};
              },
              [&](const auto &arg) -> std::vector<GpuAggrMatExpr> {
                return {GpuAggrMatExpr{
                    (arg["$3"]).as("PelagoAggregate#64317", "$3"), 1, 0, SUM}};
              },
              10,
              131072)  // (group=[{0, 1, 2}], profit=[SUM($3)],
                       // trait=[Pelago.[].unpckd.NVPTX.homSingle.hetSingle])
          .sort(
              [&](const auto &arg) -> std::vector<expression_t> {
                return {arg["$0"], arg["$1"], arg["$2"], arg["$3"]};
              },
              {direction::NONE, direction::ASC, direction::ASC, direction::ASC})
          // (sort0=[$0], sort1=[$1], sort2=[$2],
          // dir0=[ASC], dir1=[ASC], dir2=[ASC],
          // trait=[Pelago.[0, 1,
          // 2].unpckd.X86_64.homSingle.hetSingle])
          .print(
              [&](const auto &arg,
                  std::string outrel) -> std::vector<expression_t> {
                return {arg["$0"].as(outrel, "d_year"),
                        arg["$1"].as(outrel, "s_city"),
                        arg["$2"].as(outrel, "p_brand1"),
                        arg["$3"].as(outrel, "profit")};
              },
              std::string{query} +
                  (memmv ? "mv" : "nmv"))  // (trait=[ENUMERABLE.[2, 3
      // DESC].unpckd.X86_64.homSingle.hetSingle])
      ;
  return rel.prepare();
}
