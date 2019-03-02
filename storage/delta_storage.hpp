/*                             Copyright (c) 2019-2019
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

#ifndef DELTA_STORAGE_HPP_
#define DELTA_STORAGE_HPP_

#include <iostream>
#include "glo.hpp"
//#include "indexes/hash_index.hpp"
#include "storage/memory_manager.hpp"

namespace storage {

/*
    TODO:
      - Memory management sucks
      - Delta size is not resizeable
      - same locl for all 3 types of data-struc, parition and have fine-grained
          locking
      - templated so that it can have multiple types of indexing
*/

/* Currently DeltaStore is not resizeable*/
class DeltaStore {
 public:
  DeltaStore(size_t rec_size, uint64_t initial_num_objs) {
    initial_num_objs *= 50;

    size_t mem_req = (rec_size * initial_num_objs) +
                     (rec_size * sizeof(global_conf::mv_version)) +
                     (rec_size * sizeof(global_conf::mv_version_list));

    int numa_id = 0;
    void* mem = MemoryManager::alloc(mem_req, numa_id);

    // warm-up mem
    int* pt = (int*)mem;
    int warmup_size = mem_req / sizeof(int);
    for (int i = 0; i < warmup_size; i++) pt[i] = 0;

    // init object vars
    // this->data_ptr.emplace_back(new mem_chunk(mem, mem_req, numa_id));
    data_ptr = new mem_chunk(mem, mem_req, numa_id);
    this->rec_size = rec_size;
    this->cursor = (char*)mem;
    this->total_rec_capacity = initial_num_objs;
    this->used_recs_capacity = 0;

    vid_version_map.reserve(initial_num_objs);
  }

  // TODO: clear out all the memory
  ~DeltaStore();

  void insert_version(uint64_t vid, void* rec, uint64_t tmin, uint64_t tmax) {
    assert(used_recs_capacity < total_rec_capacity);
    used_recs_capacity++;
    global_conf::mv_version* val = (global_conf::mv_version*)getVersionChunk();
    val->t_min = tmin;
    val->t_max = tmax;
    val->data = getDataChunk();
    memcpy(val->data, rec, rec_size);

    // template <typename K> bool find(const K &key, mapped_type &val)
    global_conf::mv_version_list* vlst;
    vid_version_map.find(vid, vlst);
    vlst->insert(val);
    vid_version_map.insert(vid, vlst);
  }

  void* insert_version(uint64_t vid, uint64_t tmin, uint64_t tmax) {
    assert(used_recs_capacity < total_rec_capacity);
    used_recs_capacity++;
    global_conf::mv_version* val = (global_conf::mv_version*)getVersionChunk();
    val->t_min = tmin;
    val->t_max = tmax;
    val->data = getDataChunk();

    // template <typename K> bool find(const K &key, mapped_type &val)
    global_conf::mv_version_list* vlst = nullptr;
    if (vid_version_map.find(vid, vlst))
      vlst->insert(val);
    else {
      vlst = (global_conf::mv_version_list*)getListChunk();
      vlst->insert(val);
      // vid_version_map.insert_or_assign(vid, vlst);
      vid_version_map.insert_or_assign(vid, vlst);
    }

    return val->data;
  }

  bool getVersionList(uint64_t vid, global_conf::mv_version_list*& vlst) {
    if (vid_version_map.find(vid, vlst))
      return true;
    else
      return false;
  }

  global_conf::mv_version_list* getVersionList(uint64_t vid) {
    global_conf::mv_version_list* vlst = nullptr;
    vid_version_map.find(vid, vlst);
    return vlst;
  }

  double getUtilPercentage() {
    return ((double)used_recs_capacity.load() / (double)total_rec_capacity) *
           100;
  };
  void reset() {
    std::unique_lock<std::mutex> lock(this->m);
    vid_version_map.clear();
    cursor = (char*)data_ptr->data;
    used_recs_capacity = 0;
  }

 private:
  inline void* getVersionChunk() {
    void* tmp = nullptr;
    {
      std::unique_lock<std::mutex> lock(this->m);
      tmp = (void*)cursor;
      cursor += sizeof(global_conf::mv_version);
    }
    return tmp;
  }

  inline void* getDataChunk() {
    void* tmp = nullptr;
    {
      std::unique_lock<std::mutex> lock(this->m);
      tmp = (void*)cursor;
      cursor += rec_size;
    }
    return tmp;
  }
  inline void* getListChunk() {
    void* tmp = nullptr;
    {
      std::unique_lock<std::mutex> lock(this->m);
      tmp = (void*)cursor;
      cursor += sizeof(global_conf::mv_version_list);
    }
    return tmp;
  }

  std::mutex m;
  char* cursor;
  size_t rec_size;
  mem_chunk* data_ptr;
  uint64_t total_rec_capacity;
  std::atomic<uint64_t> used_recs_capacity;

  indexes::HashIndex<uint64_t, global_conf::mv_version_list*> vid_version_map;

  /* VID -> List Mapping*/
};
};  // namespace storage

#endif /* DELTA_STORAGE_HPP_ */
