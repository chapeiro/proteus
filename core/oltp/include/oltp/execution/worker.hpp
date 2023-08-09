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

#ifndef SCHEDULER_WORKER_POOL_HPP_
#define SCHEDULER_WORKER_POOL_HPP_

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <iostream>
#include <memory>
#include <platform/topology/topology.hpp>
#include <platform/util/erase-constructor-idioms.hpp>
#include <platform/util/percentile.hpp>
#include <queue>
#include <thread>
#include <unordered_map>

#include "oltp/common/common.hpp"
#include "oltp/common/constants.hpp"
#include "oltp/interface/bench.hpp"

namespace scheduler {

enum WORKER_STATE { READY, RUNNING, PAUSED, TERMINATED, PRERUN, POSTRUN };

class Worker {
  worker_id_t id;
  volatile bool terminate;
  volatile bool pause;
  volatile bool change_affinity;
  volatile WORKER_STATE state;
  volatile bool revert_affinity;

  partition_id_t partition_id;

  const topology::core *exec_core;
  topology::core *affinity_core{};

  bool is_hotplugged;
  volatile int64_t num_iters;

  txn::TxnQueue *txnQueue{};

  // STATS
  size_t num_txns;
  size_t num_commits;
  size_t num_aborts;
  // size_t txn_start_tsc;

  std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>
      txn_start_time;

  std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>
      txn_end_time;

 public:
  Worker(worker_id_t id, const topology::core *exec_core,
         int64_t num_iters = -1, partition_id_t partition_id = 0)
      : id(id),
        num_iters(num_iters),
        terminate(false),
        exec_core(exec_core),
        pause(false),
        change_affinity(false),
        revert_affinity(false),
        state(READY),
        partition_id(partition_id),
        is_hotplugged(false),
        num_txns(0),
        num_commits(0),
        num_aborts(0) {
    pause = false;
  }

 private:
  void run();
  friend class WorkerPool;
};

class WorkerPool : proteus::utils::remove_copy_move {
 public:
  // Singleton
  static WorkerPool &getInstance() {
    static WorkerPool instance;
    return instance;
  }

  void init(bench::Benchmark *txn_bench = nullptr, worker_id_t num_workers = 1,
            partition_id_t num_partitions = 1, uint worker_sched_mode = 0,
            int num_iter_per_worker = -1, bool is_elastic_workload = false);
  void shutdown(bool print_stats = false);

  void start_workers();
  void add_worker(const topology::core *exec_location, int partition_id = -1);
  void remove_worker(const topology::core *exec_location);
  void migrate_worker(bool return_back = false);

  const std::vector<worker_id_t> &scale_down(uint num_cores = 1);
  void scale_back();

  void print_worker_stats(bool global_only = true);
  void print_worker_stats_diff();
  std::pair<double, double> get_worker_stats_diff(bool print = false);

  inline worker_id_t size() const { return workers.size(); }
  void pause();
  void resume();

 private:
  void create_worker(const topology::core &exec_core,
                     bool physical_thread = true);

 private:
  WorkerPool() = default;
  ~WorkerPool() = default;

  txn::TxnQueue *txnQueue{};
  bench::Benchmark *_txn_bench{};

  worker_id_t worker_counter{};
  std::atomic<bool> terminate{};
  std::mutex worker_pool_lk;

  std::atomic<size_t> post_barrier{};
  std::atomic<size_t> pre_barrier{};
  std::condition_variable pre_cv;
  std::mutex pre_m;

  std::unordered_map<uint, std::pair<std::thread *, Worker *>> workers;
  std::vector<worker_id_t> elastic_set;

  uint num_iter_per_worker{};
  uint worker_sched_mode{};
  partition_id_t num_partitions{};
  bool elastic_workload{};

  // Stats

  std::vector<std::chrono::time_point<std::chrono::system_clock,
                                      std::chrono::nanoseconds>>
      prev_time_tps;

  std::vector<double> prev_sum_tps;

  friend class Worker;
};

}  // namespace scheduler

#endif /* SCHEDULER_WORKER_POOL_HPP_ */
