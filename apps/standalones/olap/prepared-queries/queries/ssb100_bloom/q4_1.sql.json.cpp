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

constexpr auto query = "ssb100_Q4_1";

#include "query.cpp.inc"

auto aff_parallel_cpu = []() -> std::unique_ptr<Affinitizer> {
  return std::make_unique<CpuNumaNodeAffinitizer>();
};

PreparedStatement Query::prepare41(bool memmv, size_t bloomSize) {
  auto rel51013 =
      getBuilder<Tplugin>()
          .scan<Tplugin>(
              "inputs/ssbm100/date.csv", {"d_datekey", "d_year"},
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
  auto rel51018 =
      getBuilder<Tplugin>()
          .scan<Tplugin>(
              "inputs/ssbm100/part.csv", {"p_partkey", "p_mfgr"},
              getCatalog())  // (table=[[SSB, ssbm_part]], fields=[[0, 2]],
                             // traits=[Pelago.[].packed.X86_64.homSingle.hetSingle])
          .unpack()  // (trait=[Pelago.[].unpckd.NVPTX.homBrdcst.hetSingle])
          .filter([&](const auto &arg) -> expression_t {
            return (eq(arg["p_mfgr"], "MFGR#1") | eq(arg["p_mfgr"], "MFGR#2"));
          })  // (condition=[OR(=($1, 'MFGR#1'), =($1, 'MFGR#2'))],
              // trait=[Pelago.[].unpckd.NVPTX.homBrdcst.hetSingle],
              // isS=[false])
              //          .bloomfilter_build([&](const auto &arg) { return
              //          arg["p_partkey"]; },
              //                             bloomSize, -1)
          .project([&](const auto &arg) -> std::vector<expression_t> {
            return {arg["p_partkey"]};
          })  // (p_partkey=[$0],
              // trait=[Pelago.[].unpckd.NVPTX.homBrdcst.hetSingle])
          .pack()
          .membrdcst(dop, true, true)
          .router(
              [&](const auto &arg) -> std::optional<expression_t> {
                return arg["__broadcastTarget"];
              },
              dop, 128, RoutingPolicy::HASH_BASED, dev,
              aff_parallel())  // (trait=[Pelago.[].packed.X86_64.homBrdcst.hetSingle])
          .to_gpu()  // (trait=[Pelago.[].packed.NVPTX.homBrdcst.hetSingle])
          .unpack();
  auto rel51023 =
      getBuilder<Tplugin>()
          .scan<Tplugin>(
              "inputs/ssbm100/customer.csv",
              {"c_custkey", "c_nation", "c_region"},
              getCatalog())  // (table=[[SSB, ssbm_customer]], fields=[[0, 4,
                             // 5]],
                             // traits=[Pelago.[].packed.X86_64.homSingle.hetSingle])
          .unpack()  // (trait=[Pelago.[].unpckd.NVPTX.homBrdcst.hetSingle])
          .filter([&](const auto &arg) -> expression_t {
            return eq(arg["$2"], "AMERICA");
          })
          //          .bloomfilter_build([&](const auto &arg) { return
          //          arg["c_custkey"]; },
          //                             bloomSize, -3)
          .project([&](const auto &arg) -> std::vector<expression_t> {
            return {(arg["$0"]).as("PelagoProject#51023", "$0"),
                    (arg["$1"]).as("PelagoProject#51023", "c_nation")};
          })  // (c_custkey=[$0], c_nation=[$1],
              // trait=[Pelago.[].unpckd.NVPTX.homBrdcst.hetSingle])
          .pack()
          .membrdcst(dop, true, true)
          .router(
              [&](const auto &arg) -> std::optional<expression_t> {
                return arg["__broadcastTarget"];
              },
              dop, 1, RoutingPolicy::HASH_BASED, dev,
              aff_parallel())  // (trait=[Pelago.[].packed.X86_64.homBrdcst.hetSingle])
          .to_gpu()  // (trait=[Pelago.[].packed.NVPTX.homBrdcst.hetSingle])
          .unpack();
  auto rel51028 =
      getBuilder<Tplugin>()
          .scan<Tplugin>(
              "inputs/ssbm100/supplier.csv", {"s_suppkey", "s_region"},
              getCatalog())  // (table=[[SSB, ssbm_supplier]], fields=[[0, 5]],
                             // traits=[Pelago.[].packed.X86_64.homSingle.hetSingle])
          .membrdcst(dop, true, true)
          .router(
              [&](const auto &arg) -> std::optional<expression_t> {
                return arg["__broadcastTarget"];
              },
              dop, 128, RoutingPolicy::HASH_BASED, dev,
              aff_parallel())  // (trait=[Pelago.[].packed.X86_64.homBrdcst.hetSingle])
          .unpack()  // (trait=[Pelago.[].unpckd.NVPTX.homBrdcst.hetSingle])
          .filter([&](const auto &arg) -> expression_t {
            return eq(arg["s_region"], "AMERICA");
          })  // (condition=[=($1, 'AMERICA')],
              // trait=[Pelago.[].unpckd.NVPTX.homBrdcst.hetSingle],
              // isS=[false])
          .bloomfilter_build([&](const auto &arg) { return arg["s_suppkey"]; },
                             bloomSize, 2)
          .project([&](const auto &arg) -> std::vector<expression_t> {
            return {arg["s_suppkey"]};
          })  // (s_suppkey=[$0],
              // trait=[Pelago.[].unpckd.NVPTX.homBrdcst.hetSingle])
          .pack()
          .to_gpu()  // (trait=[Pelago.[].packed.NVPTX.homBrdcst.hetSingle])
          .unpack();
  auto rel =
      getBuilder<Tplugin>()
          .scan<Tplugin>(
              "inputs/ssbm100/lineorder.csv",
              {"lo_custkey", "lo_partkey", "lo_suppkey", "lo_orderdate",
               "lo_revenue", "lo_supplycost"},
              getCatalog())  // (table=[[SSB, ssbm_lineorder]], fields=[[2, 3,
                             // 4, 5, 12, 13]],
                             // traits=[Pelago.[].packed.X86_64.homSingle.hetSingle])
          .router(8, RoutingPolicy::LOCAL, DeviceType::CPU)
          .unpack()
          .bloomfilter_probe([&](const auto &arg) { return arg["lo_suppkey"]; },
                             bloomSize, 2)
          .pack()
          .router(16, RoutingPolicy::LOCAL, dev);

  if (memmv) rel = rel.memmove(8, dev);

  rel =
      rel.to_gpu()   // (trait=[Pelago.[].packed.NVPTX.homRandom.hetSingle])
          .unpack()  // (trait=[Pelago.[].unpckd.NVPTX.homRandom.hetSingle])
          .join(
              rel51023,
              [&](const auto &build_arg) -> expression_t {
                return build_arg["$0"];
              },
              [&](const auto &probe_arg) -> expression_t {
                return probe_arg["lo_custkey"];
              },
              21,
              1048576)  // (condition=[=($2, $0)], joinType=[inner],
                        // rowcnt=[6.144E8], maxrow=[3000000.0],
                        // maxEst=[3000000.0], h_bits=[28],
                        // build=[RecordType(INTEGER c_custkey, VARCHAR
                        // c_nation)], lcount=[2.571792865733552E10],
                        // rcount=[6.1443906048E10], buildcountrow=[6.144E8],
                        // probecountrow=[6.1443906048E10])
          .join(
              rel51018,
              [&](const auto &build_arg) -> expression_t {
                return build_arg["$0"];
              },
              [&](const auto &probe_arg) -> expression_t {
                return probe_arg["lo_partkey"];
              },
              21,
              1048576)  // (condition=[=($1, $0)], joinType=[inner],
                        // rowcnt=[5.7344E8], maxrow=[1400000.0],
                        // maxEst=[1400000.0], h_bits=[28],
                        // build=[RecordType(INTEGER p_partkey)],
                        // lcount=[1.1564658448644136E10],
                        // rcount=[1.22887812096E10], buildcountrow=[5.7344E8],
                        // probecountrow=[1.22887812096E10])
          .join(
              rel51028,
              [&](const auto &build_arg) -> expression_t {
                return build_arg["s_suppkey"];
              },
              [&](const auto &probe_arg) -> expression_t {
                return probe_arg["lo_suppkey"];
              },
              12,
              65536)  // (condition=[=($3, $0)], joinType=[inner],
                      // rowcnt=[4.096E7], maxrow=[200000.0],
                      // maxEst=[200000.0], h_bits=[28],
                      // build=[RecordType(INTEGER s_suppkey)],
                      // lcount=[7.179512438249687E8],
                      // rcount=[3.0721953024E11], buildcountrow=[4.096E7],
                      // probecountrow=[3.0721953024E11])
          .join(
              rel51013,
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
                     // rcount=[4.91551248384E9], buildcountrow=[2617344.0],
                     // probecountrow=[4.91551248384E9])
          .groupby(
              [&](const auto &arg) -> std::vector<expression_t> {
                return {arg["d_year"].as("PelagoAggregate#51041", "$0"),
                        arg["c_nation"].as("PelagoAggregate#51041", "$1")};
              },
              [&](const auto &arg) -> std::vector<GpuAggrMatExpr> {
                return {
                    GpuAggrMatExpr{(arg["lo_revenue"] - arg["lo_supplycost"])
                                       .as("PelagoAggregate#51041", "$2"),
                                   1, 0, SUM}};
              },
              10,
              131072)  // (group=[{0, 1}], profit=[SUM($2)],
                       // trait=[Pelago.[].unpckd.NVPTX.homRandom.hetSingle])
          .pack()      // (trait=[Pelago.[].packed.NVPTX.homRandom.hetSingle],
                       // intrait=[Pelago.[].unpckd.NVPTX.homRandom.hetSingle],
                       // inputRows=[4.91551248384E8], cost=[{563.3628 rows,
                       // 562.8000000000001 cpu, 0.0 io}])
          .to_cpu()    // (trait=[Pelago.[].packed.X86_64.homRandom.hetSingle])
          .router(
              DegreeOfParallelism{1}, 128, RoutingPolicy::RANDOM,
              DeviceType::CPU,
              aff_reduce())  // (trait=[Pelago.[].packed.X86_64.homSingle.hetSingle])
          .memmove(8, DeviceType::CPU)
          .unpack()  // (trait=[Pelago.[].unpckd.NVPTX.homSingle.hetSingle])
          .groupby(
              [&](const auto &arg) -> std::vector<expression_t> {
                return {arg["$0"].as("PelagoAggregate#51047", "d_year"),
                        arg["$1"].as("PelagoAggregate#51047", "c_nation")};
              },
              [&](const auto &arg) -> std::vector<GpuAggrMatExpr> {
                return {GpuAggrMatExpr{
                    (arg["$2"]).as("PelagoAggregate#51047", "profit"), 1, 0,
                    SUM}};
              },
              6,
              64)  // (group=[{0, 1}], profit=[SUM($2)],
                   // trait=[Pelago.[].unpckd.NVPTX.homSingle.hetSingle])
          .sort(
              [&](const auto &arg) -> std::vector<expression_t> {
                return {arg["d_year"], arg["c_nation"], arg["profit"]};
              },
              {direction::ASC, direction::ASC,
               direction::NONE})  // (sort0=[$0], sort1=[$1], dir0=[ASC],
                                  // dir1=[ASC], trait=[Pelago.[0,
                                  // 1].unpckd.X86_64.homSingle.hetSingle])
          .print([&](const auto &arg) -> std::vector<expression_t> {
            return {arg["d_year"], arg["c_nation"], arg["profit"]};
          })  // (trait=[ENUMERABLE.[2, 3
      // DESC].unpckd.X86_64.homSingle.hetSingle])
      ;
  return rel.prepare();
}

PreparedStatement Query::prepare41_b(bool memmv, size_t bloomSize) {
  auto rel =
      getBuilder<Tplugin>()
          .scan<Tplugin>("inputs/ssbm100/lineorder.csv",
                         {"lo_custkey", "lo_partkey", "lo_suppkey",
                          "lo_orderdate", "lo_revenue", "lo_supplycost"},
                         getCatalog())  // (table=[[SSB, ssbm_lineorder]],
                                        // fields=[[2, 3, 4, 5, 12, 13]],
          // traits=[Pelago.[].packed.X86_64.homSingle.hetSingle])
          .router(128, RoutingPolicy::LOCAL, DeviceType::CPU)
          .unpack()
          .bloomfilter_probe([&](const auto &arg) { return arg["lo_suppkey"]; },
                             bloomSize, 2)
          .reduce(
              [&](const auto &arg) -> std::vector<expression_t> {
                return {expression_t{1}.as("tmp", "cnt")};
              },
              {SUM})
          .router(DegreeOfParallelism{1}, 128, RoutingPolicy::RANDOM,
                  DeviceType::CPU)
          .reduce(
              [&](const auto &arg) -> std::vector<expression_t> {
                return {arg["cnt"]};
              },
              {SUM})
          .print([&](const auto &arg) -> std::vector<expression_t> {
            return {arg["cnt"]};
          });

  return rel.prepare();
}