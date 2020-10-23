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

#ifndef PROTEUS_CLUSTER_CONTROL_HPP
#define PROTEUS_CLUSTER_CONTROL_HPP

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <thread>

#include "clustercontrolplane.grpc.pb.h"
#include "distributed-runtime/cluster-manager.hpp"
#include "util/async_containers.hpp"

/*
 * CusterManager.hpp
 *
 * class ClusterControl
 *    implementation-interface for cluster manager, an intermediary for
 *    cluster manager and actual communication primitive implementation,
 *    which in proteus' case, is gRPC server.
 *
 *    startServer: starts a gRPC server. if not-primary, then it selects
 *    an available port, starts a listening server and then registers
 *    itself to primary node with it's listenening address for control commands.
 *
 *
 *  NodeControlServiceImpl
 *     Implementation of gRPC methods as defined in the
 *     `clustercontrolplane.proto`, including serialization/deserialization
 *     of messages, as provided by the gRPC library.
 *
 */

namespace proteus::distributed {

class NodeControlServiceImpl final
    : public proteus::distributed::NodeControlPlane::Service {
  grpc::Status registerExecutor(
      grpc::ServerContext* context,
      const proteus::distributed::NodeInfo* request,
      proteus::distributed::NodeRegistrationReply* reply) override;
  grpc::Status prepareStatement(
      grpc::ServerContext* context,
      const proteus::distributed::QueryPlan* request,
      proteus::distributed::genericReply* reply) override;
  grpc::Status executeStatement(
      grpc::ServerContext* context,
      const proteus::distributed::QueryPlan* request,
      proteus::distributed::genericReply* reply) override;
  grpc::Status sendCommand(
      grpc::ServerContext* context,
      const proteus::distributed::NodeCommand* request,
      proteus::distributed::NodeStatusUpdate* reply) override;

 private:
  std::unique_ptr<proteus::distributed::NodeControlPlane::Stub> stub_;
};

class ClusterControl {
 public:
  static inline ClusterControl& getInstance() {
    static ClusterControl instance;
    return instance;
  }

  ClusterControl(ClusterControl const&) = delete;
  void operator=(ClusterControl const&) = delete;
  void operator=(ClusterControl const&&) = delete;

  void startServer(bool is_primary_node, const std::string& primary_node_addr,
                   int primary_control_port);
  void shutdownServer();

  Query getQuery();
  void broadcastQuery(Query query);
  int registerExecutor(const proteus::distributed::NodeInfo*);

  size_t getNumExecutors() {
    std::unique_lock<std::mutex> safety_lock(this->registration_lock);
    return this->executors.size();
  }

 private:
  void listenerThread();
  void registerSelfToPrimary();

  ClusterControl()
      : network_control_port(-1),
        is_primary(false),
        executor_id_ctr(0),
        self_executor_id(0) {}

 private:
  bool is_primary;
  int network_control_port;
  std::string server_address;
  std::string primary_node_address;
  std::thread listener_thread;
  int self_executor_id;

  NodeControlServiceImpl controlService;
  std::unique_ptr<grpc::Server> server;

  std::mutex registration_lock;
  std::vector<proteus::distributed::NodeInfo> executors;
  std::map<int, proteus::distributed::NodeInfo*> exec_id_map;
  std::map<std::string, proteus::distributed::NodeInfo*> exec_address_map;
  uint32_t executor_id_ctr;

  // If Primary, connection stub for every executor.
  std::map<int, std::unique_ptr<proteus::distributed::NodeControlPlane::Stub>>
      client_conn;

  // If secondary, connection stub to primary.
  std::unique_ptr<proteus::distributed::NodeControlPlane::Stub> primary_conn;

  AsyncQueueMPSC<Query> query_queue;
  // std::map<std::string, Query> query_map;

  friend class NodeControlServiceImpl;
};

}  // namespace proteus::distributed

#endif  // PROTEUS_CLUSTER_CONTROL_HPP
