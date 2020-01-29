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

#ifndef HARMONIA_QUERIES_HPP_
#define HARMONIA_QUERIES_HPP_

#include <functional>
#include <operators/relbuilder-factory.hpp>
#include <string>

#include "aeolus-plugin.hpp"
#include "plan/prepared-statement.hpp"
#include "routing/affinitizers.hpp"
#include "routing/degree-of-parallelism.hpp"

template <typename Tplugin>
inline static auto getBuilder(const std::string &query) {
  static RelBuilderFactory ctx{query + typeid(Tplugin).name()};
  return ctx.getBuilder();
}

inline static auto &getCatalog() { return CatalogParser::getInstance(); }

typedef std::function<RelBuilder(std::string, std::vector<std::string>)> scan_t;

typedef std::function<std::unique_ptr<Affinitizer>()> aff_t;

template <int64_t id>
struct Q {
 private:
  static constexpr int64_t Qid = id;

  template <typename Tplugin = AeolusRemotePlugin>
  static PreparedStatement c1t() {
    throw std::runtime_error("unimplemented");
  }

  template <typename Tplugin = AeolusRemotePlugin, typename Tp, typename Tr>
  inline static PreparedStatement cpar(DegreeOfParallelism dop, Tp aff_parallel,
                                       Tr aff_reduce, DeviceType dev);

  template <typename Tplugin>
  static auto scan(std::string relName, std::vector<std::string> relAttrs) {
    return getBuilder<Tplugin>("ch_Q" + std::to_string(Qid))
        .template scan<Tplugin>(relName, relAttrs,
                                CatalogParser::getInstance());
  }

 public:
  template <typename Tplugin = AeolusRemotePlugin, typename Tp, typename Tr>
  inline static PreparedStatement prepare(DegreeOfParallelism dop,
                                          Tp aff_parallel, Tr aff_reduce,
                                          DeviceType dev = DeviceType::CPU) {
    if (dop == DegreeOfParallelism{1} && dev == DeviceType::CPU) {
      return c1t<Tplugin>();
    }
    return cpar<Tplugin>(dop, aff_parallel, aff_reduce, dev);
  }
};

#endif /* HARMONIA_QUERIES_HPP_ */
