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

#ifndef PREPARED_STATEMENT_HPP_
#define PREPARED_STATEMENT_HPP_

#include <memory>
#include <utility>
#include <vector>

#include "query-result.hpp"

class Pipeline;
class AffinitizationFactory;

class PreparedStatement {
 private:
  // Both fields are non-const to allow moving PreparedStatements
  std::vector<std::shared_ptr<Pipeline>> pipelines;
  std::string outputFile;

 protected:
  PreparedStatement(std::vector<std::unique_ptr<Pipeline>> pips,
                    std::string outputFile);

 public:
  QueryResult execute(bool deterministic_affinity = true);

  static PreparedStatement from(const std::string& planPath,
                                const std::string& label);
  static PreparedStatement from(const std::string& planPath,
                                const std::string& label,
                                const std::string& catalogJSON);
  static PreparedStatement from(
      const std::string& planPath, const std::string& label,
      std::unique_ptr<AffinitizationFactory> affFactory);

  friend class RelBuilder;
};

#endif /* PREPARED_STATEMENT_HPP_ */