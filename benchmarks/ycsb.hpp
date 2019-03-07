/*
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

#ifndef YCSB_HPP_
#define YCSB_HPP_

// extern "C" {
//#include "stdlib.h"
//}

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <thread>
#include "benchmarks/bench.hpp"
#include "scheduler/topology.hpp"
#include "storage/table.hpp"
#include "transactions/transaction_manager.hpp"
//#include <thread

namespace bench {

/*

        Benchmark: Yahoo! Cloud Serving Benchmark

        Description:

        Tunable Parameters:

*/

class YCSB : public Benchmark {
 private:
  const int num_fields;
  const int num_records;
  const double theta;
  const int num_iterations_per_worker;
  const int num_ops_per_txn;
  const double write_threshold;
  int num_max_workers;
  int num_active_workers;

  storage::Schema schema;
  storage::Table *ycsb_tbl;

  std::atomic<bool> initialized;  // so that nobody calls load twice
                                  // std::atomic<uint64_t> key_gen;

 public:
  /*  // Singleton
    static inline YCSB &getInstance() {
      static YCSB instance("YCSB");
      return instance;
    }
    YCSB(YCSB const &) = delete;            // Don't Implement
    void operator=(YCSB const &) = delete;  // Don't implement */

  void load_data(int num_threads = 1) {
    std::cout << "[YCSB] Loading data.." << std::endl;
    std::vector<std::tuple<std::string, storage::data_type, size_t> > columns;
    for (int i = 0; i < num_fields; i++) {
      columns.emplace_back(std::tuple<std::string, storage::data_type, size_t>(
          "col_" + std::to_string(i + 1), storage::INTEGER, sizeof(uint64_t)));
    }
    ycsb_tbl = schema.create_table("ycsb_tbl", storage::COLUMN_STORE, columns,
                                   num_records);

    /* Load data into tables*/
    txn::TransactionManager *txnManager =
        &txn::TransactionManager::getInstance();

    for (int i = 0; i < num_records; i++) {
      std::vector<uint64_t> tmp(num_fields, i);
      // ycsb_tbl->insertRecord(&tmp);
      struct txn::TXN insert_txn = gen_insert_txn(i, &tmp);
      txnManager->execute_txn(&insert_txn);
      if (i % 1000000 == 0)
        std::cout << "[YCSB] inserted records: " << i << std::endl;
      // key_gen++;
      // free the txn ops pointers
    };
    std::cout << "[YCSB] inserted records: " << num_records << std::endl;
    // Set init flag to true
    initialized = true;

    /*for (int i = 0; i < num_records; i++) {
      std::vector<uint64_t> tmp(num_fields, i);
      // ycsb_tbl->insertRecord(&tmp);
      struct txn::TXN insert_txn = gen_upd_txn(i, &tmp);
      txnManager->execute_txn(&insert_txn);
      if (i % 1000000 == 0)
        std::cout << "[YCSB] updated records: " << i << std::endl;
      // key_gen++;
      // free the txn ops pointers
    };*/
  };

  struct txn::TXN gen_insert_txn(uint64_t key, void *rec) {
    struct txn::TXN txn;

    txn.ops = new txn::TXN_OP[1];
    assert(txn.ops != NULL);

    txn.ops[0].op_type = txn::OPTYPE_INSERT;
    txn.ops[0].data_table = ycsb_tbl;
    txn.ops[0].key = key;
    txn.ops[0].rec = rec;
    txn.n_ops = 1;
    return txn;
  }
  struct txn::TXN gen_upd_txn(uint64_t key, void *rec) {
    struct txn::TXN txn;

    txn.ops = new txn::TXN_OP[1];
    assert(txn.ops != NULL);

    txn.ops[0].op_type = txn::OPTYPE_UPDATE;
    txn.ops[0].data_table = ycsb_tbl;
    txn.ops[0].key = key;
    txn.ops[0].rec = rec;
    txn.n_ops = 1;
    return txn;
  }

  /* FIXME: Possible memory leak because we dont clear the TXN memory*/
  void *gen_txn(int wid) {
    /* TODO: too many pointer indirections, it should be something static so
     * that we reduce random memory access while generating queries */
    struct txn::TXN *txn = (struct txn::TXN *)malloc(sizeof(struct txn::TXN));

    assert(txn != NULL);

    txn->ops = new txn::TXN_OP[num_ops_per_txn];
    //(struct txn::TXN_OP **)malloc(sizeof(txn::TXN_OP) * num_ops_per_txn);
    assert(txn->ops != NULL);
    txn::OP_TYPE op;
    wid = wid % num_active_workers;
    // std::cout << "wid :" << wid
    //          << "| c= " << ((int)(write_threshold * (double)num_workers))
    //          << std::endl;

    if (wid >= (write_threshold * (double)num_active_workers)) {
      op = txn::OPTYPE_LOOKUP;
      // std::cout << "L ";
    } else {
      op = txn::OPTYPE_UPDATE;
      // std::cout << "U ";
    }

    // op = txn::OPTYPE_UPDATE;
    // op = txn::OPTYPE_LOOKUP;

    bool is_duplicate = false;
    for (int i = 0; i < num_ops_per_txn; i++) {
      txn->ops[i].op_type = op;
      txn->ops[i].data_table = ycsb_tbl;
      txn->ops[i].rec = nullptr;

      do {
        // make op
        zipf_val(wid, &txn->ops[i]);
        is_duplicate = false;

        for (int j = 0; j < i; j++) {
          if (txn->ops[i].key == txn->ops[j].key) {
            is_duplicate = true;
            break;
          }
        }
      } while (is_duplicate == true);
      // txn->n_ops++; // THIS IS FUCKING BUG HERE
    }
    txn->n_ops = num_ops_per_txn;  // THIS IS FUCKING BUG HERE
    return txn;
  }

  void exec_txn(void *stmts) { return; }

  // TODO: clean-up
  ~YCSB() { std::cout << "destructor of YCSB" << std::endl; }

  // private:
  YCSB(std::string name = "YCSB", int num_fields = 2, int num_records = 1000000,
       double theta = 0.5, int num_iterations_per_worker = 1000000,
       int num_ops_per_txn = 2, double write_threshold = 0.5,
       int num_active_workers = -1, int num_max_workers = -1)
      : Benchmark(name),
        num_fields(num_fields),
        num_records(num_records),
        theta(theta),
        num_iterations_per_worker(num_iterations_per_worker),
        num_ops_per_txn(num_ops_per_txn),
        write_threshold(write_threshold),
        schema(name),
        num_max_workers(num_max_workers),
        num_active_workers(num_active_workers) {
    initialized = false;
    // num_workers = scheduler::Topology::getInstance().get_num_worker_cores();
    if (num_max_workers == -1)
      num_max_workers = std::thread::hardware_concurrency();
    if (num_active_workers == -1)
      num_active_workers = std::thread::hardware_concurrency();
    // key_gen = 0;
    init();

    std::cout << "W_THRESH: " << write_threshold << std::endl;

    for (int i = 0; i < num_active_workers; i++) {
      std::cout << "WID: " << i;
      if (i >= (write_threshold * (double)num_active_workers)) {
        std::cout << " L ";
      } else {
        std::cout << " U ";
      }
      std::cout << std::endl;
    }
  };

  struct drand48_data *rand_buffer;
  double g_zetan;
  double g_zeta2;
  double g_eta;
  double g_alpha_half_pow;

  void init() {
    printf("Initializing zipf\n");
    rand_buffer = (struct drand48_data *)calloc(num_max_workers,
                                                sizeof(struct drand48_data));

    for (int i = 0; i < num_max_workers; i++) {
      srand48_r(i + 1, &rand_buffer[i]);
    }

    uint64_t n = num_records - 1;
    g_zetan = zeta(n, theta);
    g_zeta2 = zeta(2, theta);

    g_eta = (1 - pow(2.0 / n, 1 - theta)) / (1 - g_zeta2 / g_zetan);
    g_alpha_half_pow = 1 + pow(0.5, theta);
    printf("n = %lu\n", n);
    printf("theta = %.2f\n", theta);
  }

  void zipf_val(int wid, struct txn::TXN_OP *op) {
    uint64_t n = num_records - 1;

    // elasticity hack when we will increase num_server on runtime
    // wid = wid % num_workers;

    double alpha = 1 / (1 - theta);

    double u;
    drand48_r(&rand_buffer[wid], &u);
    double uz = u * g_zetan;

    if (uz < 1) {
      op->key = 0;
    } else if (uz < g_alpha_half_pow) {
      op->key = 1;
    } else {
      op->key = (uint64_t)(n * pow(g_eta * u - g_eta + 1, alpha));
    }

    // get the server id for the key
    int tserver = op->key % num_max_workers;
    // get the key count for the key
    uint64_t key_cnt = op->key / num_max_workers;

    uint64_t recs_per_server = num_records / num_max_workers;
    op->key = tserver * recs_per_server + key_cnt;

    assert(op->key < num_records);
  }

  double zeta(uint64_t n, double theta) {
    double sum = 0;
    for (uint64_t i = 1; i <= n; i++) sum += std::pow(1.0 / i, theta);
    return sum;
  }
};

}  // namespace bench

#endif /* YCSB_HPP_ */
