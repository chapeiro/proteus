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

constexpr auto query = "ssb100_Q1_3";

#include "query.cpp.inc"

PreparedStatement Query::prepare13(bool memmv) {
  auto rel6631 =
      RelBuilder{getContext<Tplugin>()}
          .scan<Tplugin>(
              "inputs/ssbm100/date.csv",
              {"d_datekey", "d_year", "d_weeknuminyear"},
              getCatalog())  // (table=[[SSB, ssbm_date]], fields=[[0, 4, 11]],
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
            return (eq(arg["$2"], 6) & eq(arg["$1"], 1994));
          })  // (condition=[AND(=($2, 6), =($1, 1994))],
              // trait=[Pelago.[].unpckd.NVPTX.homBrdcst.hetSingle],
              // isS=[false])
          .project([&](const auto &arg) -> std::vector<expression_t> {
            return {(arg["$0"]).as("PelagoProject#6631", "$0")};
          })  // (d_datekey=[$0],
              // trait=[Pelago.[].unpckd.NVPTX.homBrdcst.hetSingle])
      ;
  auto rel =
      RelBuilder{getContext<Tplugin>()}
          .scan<Tplugin>(
              "inputs/ssbm100/lineorder.csv",
              {"lo_orderdate", "lo_quantity", "lo_extendedprice",
               "lo_discount"},
              getCatalog())  // (table=[[SSB, ssbm_lineorder]], fields=[[5, 8,
                             // 9, 11]],
                             // traits=[Pelago.[].packed.X86_64.homSingle.hetSingle])
          .router(
              dop, 8, RoutingPolicy::LOCAL, dev,
              aff_parallel())  // (trait=[Pelago.[].packed.X86_64.homRandom.hetSingle])
      ;

  if (memmv) rel = rel.memmove(8, dev == DeviceType::CPU);

  rel =
      rel.to_gpu()   // (trait=[Pelago.[].packed.NVPTX.homRandom.hetSingle])
          .unpack()  // (trait=[Pelago.[].unpckd.NVPTX.homRandom.hetSingle])
          .filter([&](const auto &arg) -> expression_t {
            return (ge(arg["$3"], 5) & le(arg["$3"], 7) & ge(arg["$1"], 26) &
                    le(arg["$1"], 35));
          })  // (condition=[AND(>=($3, 5), <=($3, 7), >=($1, 26), <=($1, 35))],
              // trait=[Pelago.[].unpckd.NVPTX.homRandom.hetSingle],
              // isS=[false])
          .join(
              rel6631,
              [&](const auto &build_arg) -> expression_t {
                return build_arg["$0"];
              },
              [&](const auto &probe_arg) -> expression_t {
                return probe_arg["lo_orderdate"];
              },
              5,
              16)  // (condition=[=($1, $0)], joinType=[inner],
                   // rowcnt=[58890.24], maxrow=[2556.0], maxEst=[2556.0],
                   // h_bits=[18], build=[RecordType(INTEGER d_datekey)],
                   // lcount=[646816.8670798924], rcount=[1.920122064E10],
                   // buildcountrow=[58890.24], probecountrow=[1.920122064E10])
          .project([&](const auto &arg) -> std::vector<expression_t> {
            return {(arg["lo_extendedprice"] * arg["lo_discount"])
                        .as("PelagoProject#6638", "$0")};
          })  // ($f0=[$2], trait=[Pelago.[].unpckd.NVPTX.homRandom.hetSingle])
          .reduce(
              [&](const auto &arg) -> std::vector<expression_t> {
                return {(arg["$0"]).as("PelagoAggregate#6639", "$0")};
              },
              {SUM})  // (group=[{}], revenue=[SUM($0)],
                      // trait=[Pelago.[].unpckd.NVPTX.homRandom.hetSingle])
          .to_cpu()   // (trait=[Pelago.[].packed.X86_64.homRandom.hetSingle])
          .router(
              DegreeOfParallelism{1}, 128, RoutingPolicy::RANDOM,
              DeviceType::CPU,
              aff_reduce())  // (trait=[Pelago.[].packed.X86_64.homSingle.hetSingle])
          .reduce(
              [&](const auto &arg) -> std::vector<expression_t> {
                return {(arg["$0"]).as("PelagoAggregate#6645", "$0")};
              },
              {SUM})  // (group=[{}], revenue=[SUM($0)],
                      // trait=[Pelago.[].unpckd.NVPTX.homSingle.hetSingle])
          .print(
              [&](const auto &arg,
                  std::string outrel) -> std::vector<expression_t> {
                return {arg["$0"].as(outrel, "revenue")};
              },
              std::string{query} +
                  (memmv
                       ? "mv"
                       : "nmv"))  // (trait=[ENUMERABLE.[].unpckd.X86_64.homSingle.hetSingle])
      ;
  return rel.prepare();
}
