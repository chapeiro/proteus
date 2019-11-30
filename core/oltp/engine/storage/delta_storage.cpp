/*
                  AEOLUS - In-Memory HTAP-Ready OLTP Engine

                             Copyright (c) 2019-2019
           Data Intensive Applications and Systems Laboratory (DIAS)
                   Ecole Polytechnique Federale de Lausanne

                              All Rights Reserved.

      Permission to use, copy, modify and distribute this software and its
    documentation is hereby granted, provided that both the copyright notice
  and this permission notice appear in all copies of the software, derivative
  works or modified versions, and any portions thereof, and that both notices
                      appear in supporting documentation.

  This code is distributed in the hope that it will be useful, but WITHOUT ANY
 WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 A PARTICULAR PURPOSE. THE AUTHORS AND ECOLE POLYTECHNIQUE FEDERALE DE LAUSANNE
DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE
                             USE OF THIS SOFTWARE.
*/

#include "storage/delta_storage.hpp"

#include <sys/mman.h>

#include "scheduler/worker.hpp"
#include "storage/table.hpp"

namespace storage {

// FIXME: NUMA socket for delta-partitions are hard-coded at the moment.

DeltaStore::DeltaStore(uint delta_id, uint64_t ver_list_capacity,
                       uint64_t ver_data_capacity, int num_partitions)
    : touched(false) {
  this->delta_id = delta_id;

  ver_list_capacity = ver_list_capacity * (1024 * 1024 * 1024);  // GB
  ver_list_capacity = ver_list_capacity / 2;
  ver_data_capacity = ver_data_capacity * (1024 * 1024 * 1024);  // GB
  for (int i = 0; i < num_partitions; i++) {
    const auto& numa_idx = storage::NUMAPartitionPolicy::getInstance()
                               .getPartitionInfo(i)
                               .numa_idx;

    void* mem_list = MemoryManager::alloc(ver_list_capacity, numa_idx,
                                          MADV_DONTFORK | MADV_HUGEPAGE);
    void* mem_data = MemoryManager::alloc(ver_data_capacity, numa_idx,
                                          MADV_DONTFORK | MADV_HUGEPAGE);
    assert(mem_list != NULL);
    assert(mem_data != NULL);

    assert(mem_list != nullptr);
    assert(mem_data != nullptr);

    void* obj_data =
        MemoryManager::alloc(sizeof(DeltaPartition), numa_idx, MADV_DONTFORK);

    partitions.emplace_back(new (obj_data) DeltaPartition(
        (char*)mem_list, mem_chunk(mem_list, ver_list_capacity, numa_idx),
        (char*)mem_data, mem_chunk(mem_data, ver_data_capacity, numa_idx), i));
  }

  if (DELTA_DEBUG) {
    std::cout << "\tDelta size: "
              << ((double)(ver_list_capacity + ver_data_capacity) /
                  (1024 * 1024 * 1024))
              << " GB * " << num_partitions << " Partitions" << std::endl;
    std::cout << "\tDelta size: "
              << ((double)(ver_list_capacity + ver_data_capacity) *
                  num_partitions / (1024 * 1024 * 1024))
              << " GB" << std::endl;
  }
  this->total_mem_reserved =
      (ver_list_capacity + ver_data_capacity) * num_partitions;

  this->readers.store(0);
  this->gc_reset_success.store(0);
  this->gc_requests.store(0);
  this->ops.store(0);
  this->gc_lock.store(0);
  this->tag = 1;
  this->max_active_epoch = 0;
  // this->min_active_epoch = std::numeric_limits<uint64_t>::max();
}

DeltaStore::~DeltaStore() { print_info(); }

void DeltaStore::print_info() {
  static int i = 0;
  std::cout << "[DeltaStore # " << i
            << "] Number of GC Requests: " << this->gc_requests.load()
            << std::endl;

  std::cout << "[DeltaStore # " << i << "] Number of Successful GC Resets: "
            << this->gc_reset_success.load() << std::endl;
  std::cout << "[DeltaStore # " << i
            << "] Number of Operations: " << this->ops.load() << std::endl;

  for (auto& p : partitions) {
    p->report();
  }
  i++;
  if (i >= partitions.size()) i = 0;
}

void* DeltaStore::insert_version(global_conf::IndexVal* idx_ptr, uint rec_size,
                                 ushort parition_id) {
  char* cnk = (char*)partitions[parition_id]->getVersionDataChunk(rec_size);
  global_conf::mv_version* val = (global_conf::mv_version*)cnk;
  val->t_min = idx_ptr->t_min;
  val->t_max = 0;  // idx_ptr->t_max;
  val->data = cnk + sizeof(global_conf::mv_version);

  if (idx_ptr->delta_ver_tag != tag) {
    // none/stale list
    idx_ptr->delta_ver_tag = tag;
    idx_ptr->delta_ver =
        (global_conf::mv_version_list*)partitions[parition_id]->getListChunk();
    idx_ptr->delta_ver->insert(val);
  } else {
    // valid list
    idx_ptr->delta_ver->insert(val);
  }

  if (!touched) touched = true;
  return val->data;
}

void DeltaStore::gc() {
  short e = 0;
  if (gc_lock.compare_exchange_strong(e, -1)) {
    // gc_requests++;

    uint64_t last_alive_txn =
        scheduler::WorkerPool::getInstance().get_min_active_txn();
    if (this->readers == 0 && should_gc() &&
        last_alive_txn > max_active_epoch) {
      for (auto& p : partitions) {
        p->reset();
      }
      tag++;
      gc_lock.store(0);
      touched = false;
      // gc_reset_success++;
    } else {
      // gc_lock.unlock();
      gc_lock.store(0);
    }
  }
}

}  // namespace storage