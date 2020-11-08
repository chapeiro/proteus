/*
    Proteus -- High-performance query processing on heterogeneous hardware.

                            Copyright (c) 2020
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

#include <query-shaping/input-prefix-query-shaper.hpp>

namespace proteus {
InputPrefixQueryShaper::InputPrefixQueryShaper(std::string base_path)
    : base_path(std::move(base_path)) {}

std::string InputPrefixQueryShaper::getRelName(const std::string& base) {
  return base_path + base + ".csv";
}

int InputPrefixQueryShaper::sf() {
  return (base_path.find("1000") != std::string::npos) ? 1000 : 100;
}

[[nodiscard]] pg InputPrefixQueryShaper::getPlugin() const {
  return pg{"distributed-block"};
}

[[nodiscard]] DeviceType InputPrefixQueryShaper::getDevice() {
  return DeviceType::CPU;
}
}  // namespace proteus