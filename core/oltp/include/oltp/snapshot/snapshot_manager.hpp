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

#ifndef AEOLUS_SNAPSHOT_SNAPSHOT_MANAGER_HPP_
#define AEOLUS_SNAPSHOT_SNAPSHOT_MANAGER_HPP_

#include <cstdlib>

#include "oltp/common/common.hpp"
#include "oltp/snapshot/circular_master_arena.hpp"
#include "oltp/snapshot/cor_const_arena.hpp"
#include "oltp/snapshot/cow_arena.hpp"
#include "oltp/snapshot/lazy_master_arena.hpp"

namespace aeolus::snapshot {

template <typename T>
class SnapshotManager_impl {
 public:
  static void init() { T::init(); }
  static void deinit() { T::deinit(); }

  static auto create(size_t bytes) { return T::create(bytes); }

 private:
};

using SnapshotManager = SnapshotManager_impl<CircularMasterProvider>;

}  // namespace aeolus::snapshot

#endif /* AEOLUS_SNAPSHOT_SNAPSHOT_MANAGER_HPP_ */
