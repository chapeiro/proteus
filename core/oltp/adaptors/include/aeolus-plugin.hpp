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

#ifndef AEOLUS_PLUGIN_HPP_
#define AEOLUS_PLUGIN_HPP_

#include <utility>

#include "olap/plugins/binary-block-plugin.hpp"

class AeolusPlugin : public BinaryBlockPlugin {
 public:
  AeolusPlugin(ParallelContext *const context, std::string fnamePrefix,
               RecordType rec,
               const std::vector<RecordAttribute *> &whichFields,
               std::string pgType);

 protected:
  llvm::Value *getSession(ParallelContext *context) const override;

  llvm::Value *getDataPointersForFile(ParallelContext *context, size_t i,
                                      llvm::Value *session_ptr) const override;
  void freeDataPointersForFile(ParallelContext *context, size_t i,
                               llvm::Value *v) const override;
  std::pair<llvm::Value *, llvm::Value *> getPartitionSizes(
      ParallelContext *context, llvm::Value *session_ptr) const override;
  void freePartitionSizes(ParallelContext *context,
                          llvm::Value *v) const override;

  void releaseSession(ParallelContext *context, llvm::Value *) const override;

 public:
  virtual void **getDataPointerForFile_runtime(size_t i, const char *relName,
                                               const char *attrName,
                                               void *session);

  virtual void freeDataPointerForFile_runtime(void **inn);

  virtual int64_t *getNumOfTuplesPerPartition_runtime(const char *relName,
                                                      void *session);

  virtual void freeNumOfTuplesPerPartition_runtime(int64_t *inn);

  void updateValueEager(ParallelContext *context, ProteusValue rid,
                        ProteusValue value, const ExpressionType *type,
                        const std::string &fileName) override;

  bool olap_snapshot_only;
  bool elastic_scan;

 private:
  void updateValueEagerInternal(ParallelContext *context, ProteusValue rid,
                                ProteusValue value, const ExpressionType *type,
                                const std::string &relName, uint8_t index);

  std::string pgType;
};

class AeolusLocalPlugin : public AeolusPlugin {
 public:
  static constexpr auto type = "block-local";
  AeolusLocalPlugin(ParallelContext *context, std::string fnamePrefix,
                    RecordType rec,
                    const std::vector<RecordAttribute *> &whichFields)
      : AeolusPlugin(context, std::move(fnamePrefix), rec, whichFields, type) {
    olap_snapshot_only = true;
    elastic_scan = false;
  }
};

class AeolusRemotePlugin : public AeolusPlugin {
 public:
  static constexpr auto type = "block-remote";
  AeolusRemotePlugin(ParallelContext *context, std::string fnamePrefix,
                     RecordType rec,
                     const std::vector<RecordAttribute *> &whichFields)
      : AeolusPlugin(context, std::move(fnamePrefix), rec, whichFields, type) {}
};

class AeolusElasticPlugin : public AeolusPlugin {
 public:
  static constexpr auto type = "block-elastic";
  AeolusElasticPlugin(ParallelContext *context, std::string fnamePrefix,
                      RecordType rec,
                      const std::vector<RecordAttribute *> &whichFields);
};

class AeolusElasticNIPlugin : public AeolusPlugin {
 public:
  static constexpr auto type = "block-elastic-ni";
  AeolusElasticNIPlugin(ParallelContext *context, std::string fnamePrefix,
                        RecordType rec,
                        const std::vector<RecordAttribute *> &whichFields);
};

extern "C" {

Plugin *createBlockRemotePlugin(
    ParallelContext *context, std::string fnamePrefix, RecordType rec,
    const std::vector<RecordAttribute *> &whichFields);
Plugin *createBlockLocalPlugin(
    ParallelContext *context, std::string fnamePrefix, RecordType rec,
    const std::vector<RecordAttribute *> &whichFields);
Plugin *createBlockElasticPlugin(
    ParallelContext *context, std::string fnamePrefix, RecordType rec,
    const std::vector<RecordAttribute *> &whichFields);
Plugin *createBlockElasticNiPlugin(
    ParallelContext *context, std::string fnamePrefix, RecordType rec,
    const std::vector<RecordAttribute *> &whichFields);
}

#endif /* AEOLUS_PLUGIN_HPP_ */
