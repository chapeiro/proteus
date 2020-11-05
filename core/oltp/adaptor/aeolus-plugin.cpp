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

#include "aeolus-plugin.hpp"

#include <cmath>
#include <exception>
#include <olap/plan/catalog-parser.hpp>
#include <string>

#include "olap/util/parallel-context.hpp"
#include "oltp/storage/layout/column_store.hpp"
#include "oltp/storage/table.hpp"

using namespace llvm;

extern "C" {
storage::ColumnStore *getRelation(std::string fnamePrefix,
                                  std::string opt_suffix = "") {
  for (auto &tb : storage::Schema::getInstance().getTables()) {
    if (fnamePrefix.compare(tb->name) == 0 ||
        (fnamePrefix == (tb->name + opt_suffix))) {
      // assert(tb->storage_layout == storage::COLUMN_STORE);
      return (storage::ColumnStore *)tb;
    }
  }
  auto msg = std::string("Relation not found: ") + fnamePrefix;
  LOG(INFO) << msg;
  throw std::runtime_error(msg);
}

void **getDataPointerForFile_runtime(size_t i, const char *relName,
                                     const char *attrName, void *session,
                                     AeolusPlugin *pg) {
  return pg->getDataPointerForFile_runtime(i, relName, attrName, session);
}

void freeDataPointerForFile_runtime(void **inn, AeolusPlugin *pg) {
  pg->freeDataPointerForFile_runtime(inn);
}

void *getSession() { return nullptr; }
void releaseSession(void *session) {}

int64_t *getNumOfTuplesPerPartition_runtime(const char *relName, void *session,
                                            AeolusPlugin *pg) {
  return pg->getNumOfTuplesPerPartition_runtime(relName, session);
}

void freeNumOfTuplesPerPartition_runtime(int64_t *inn, AeolusPlugin *pg) {
  pg->freeNumOfTuplesPerPartition_runtime(inn);
}

Plugin *createBlockRemotePlugin(
    ParallelContext *context, std::string fnamePrefix, RecordType rec,
    const std::vector<RecordAttribute *> &whichFields) {
  return new AeolusRemotePlugin(context, fnamePrefix, rec, whichFields);
}

Plugin *createBlockLocalPlugin(
    ParallelContext *context, std::string fnamePrefix, RecordType rec,
    const std::vector<RecordAttribute *> &whichFields) {
  return new AeolusLocalPlugin(context, fnamePrefix, rec, whichFields);
}

Plugin *createBlockElasticPlugin(
    ParallelContext *context, std::string fnamePrefix, RecordType rec,
    const std::vector<RecordAttribute *> &whichFields) {
  return new AeolusElasticPlugin(context, fnamePrefix, rec, whichFields);
}

Plugin *createBlockElasticNiPlugin(
    ParallelContext *context, std::string fnamePrefix, RecordType rec,
    const std::vector<RecordAttribute *> &whichFields) {
  return new AeolusElasticNIPlugin(context, fnamePrefix, rec, whichFields);
}
}

void **AeolusPlugin::getDataPointerForFile_runtime(size_t i,
                                                   const char *relName,
                                                   const char *attrName,
                                                   void *session) {
  const auto &tbl = getRelation({relName}, "<" + pgType + ">");
  const auto &data_arenas =
      tbl->snapshot_get_data(i, wantedFields, olap_snapshot_only, elastic_scan);

  assert(data_arenas.size() > 0 && "ERROR: getDataPointerForFile");

  void **arr = (void **)malloc(sizeof(void *) * data_arenas.size());
  for (uint j = 0; j < data_arenas.size(); j++) {
    arr[j] = data_arenas[j].first.data;
    // LOG(INFO) << topology::getInstance().getCpuNumaNodeAddressed(arr[j])->id;
  }
  return arr;

  // std::vector<std::pair<mem_chunk, uint64_t>> Column::snapshot_get_data(
  //   size_t scan_idx, std::vector<RecordAttribute*>& wantedFields,
  //   bool olap_local, bool elastic_scan)

  // for (auto &c : tbl->getColumns()) {
  //   if (strcmp(c->name.c_str(), attrName) == 0) {
  //     const auto &data_arenas =
  //         c->snapshot_get_data(i, olap_snapshot_only, elastic_scan);
  //     void **arr = (void **)malloc(sizeof(void *) * data_arenas.size());
  //     for (uint i = 0; i < data_arenas.size(); i++) {
  //       arr[i] = data_arenas[i].first.data;
  //       LOG(INFO)
  //           << topology::getInstance().getCpuNumaNodeAddressed(arr[i])->id;
  //     }
  //     return arr;
  //   }
  // }
}

void AeolusPlugin::freeDataPointerForFile_runtime(void **inn) { free(inn); }

int64_t *AeolusPlugin::getNumOfTuplesPerPartition_runtime(const char *relName,
                                                          void *session) {
  const auto &tbl = getRelation({relName}, "<" + pgType + ">");

  return tbl->snapshot_get_number_tuples(olap_snapshot_only, elastic_scan);

  // const auto &c = tbl->getColumns()[0];
  // // FIXME: this should actually get pointer, just the number of records.
  // const auto &data_arenas = c->snapshot_get_data();
  // int64_t *arr = (int64_t *)malloc(sizeof(int64_t *) * data_arenas.size());
}

void AeolusPlugin::freeNumOfTuplesPerPartition_runtime(int64_t *inn) {
  free(inn);
  // TODO: bit-mast reset logic.
}

AeolusPlugin::AeolusPlugin(ParallelContext *const context, string fnamePrefix,
                           RecordType rec,
                           const std::vector<RecordAttribute *> &whichFields,
                           string pgType)
    : BinaryBlockPlugin(context, fnamePrefix, rec, whichFields, false),
      pgType(pgType) {
  Nparts = getRelation(fnamePrefix, "<" + pgType + ">")
               ->getColumns()[0]
               ->snapshot_get_data()
               .size();

  olap_snapshot_only = false;
  elastic_scan = false;
}

AeolusElasticPlugin::AeolusElasticPlugin(
    ParallelContext *const context, std::string fnamePrefix, RecordType rec,
    const std::vector<RecordAttribute *> &whichFields)
    : AeolusPlugin(context, fnamePrefix, rec, whichFields, type) {
  // 2  means that a partition can be split in max 2 more.
  Nparts = std::pow(wantedFields.size(), 2);
  LOG(INFO) << "Elastic Plugin- Nparts: " << Nparts;

  elastic_scan = true;
  olap_snapshot_only = false;
}

AeolusElasticNIPlugin::AeolusElasticNIPlugin(
    ParallelContext *const context, std::string fnamePrefix, RecordType rec,
    const std::vector<RecordAttribute *> &whichFields)
    : AeolusPlugin(context, fnamePrefix, rec, whichFields, type) {
  // 2  means that a partition can be split in max 2 more.
  Nparts = std::pow(wantedFields.size(), 2);
  LOG(INFO) << "Elastic-ni Plugin- Nparts: " << Nparts;

  elastic_scan = true;
  olap_snapshot_only = false;
}

llvm::Value *createCall(std::string func,
                        std::initializer_list<llvm::Value *> args,
                        Context *context, llvm::Type *ret) {
  return context->gen_call(func, args, ret);
}

void createCall2(std::string func, std::initializer_list<llvm::Value *> args,
                 Context *context) {
  createCall(func, args, context, Type::getVoidTy(context->getLLVMContext()));
}

llvm::Value *AeolusPlugin::getSession(ParallelContext *context) const {
  return createCall("getSession", {}, context,
                    Type::getInt8PtrTy(context->getLLVMContext()));
}

void AeolusPlugin::releaseSession(ParallelContext *context,
                                  llvm::Value *session_ptr) const {
  createCall2("releaseSession", {session_ptr}, context);
}

llvm::Value *AeolusPlugin::getDataPointersForFile(
    ParallelContext *context, size_t i, llvm::Value *session_ptr) const {
  LLVMContext &llvmContext = context->getLLVMContext();

  Type *char8ptr = Type::getInt8PtrTy(llvmContext);

  Value *this_ptr = context->getBuilder()->CreateIntToPtr(
      context->createInt64((uintptr_t)this), char8ptr);

  Value *N_parts_ptr = createCall(
      "getDataPointerForFile_runtime",
      {context->createSizeT(i),
       context->CreateGlobalString(wantedFields[i]->getRelationName().c_str()),
       context->CreateGlobalString(wantedFields[i]->getAttrName().c_str()),
       session_ptr, this_ptr},
      context, PointerType::getUnqual(char8ptr));

  auto data_type = PointerType::getUnqual(
      ArrayType::get(RecordAttribute{*(wantedFields[i]), true}.getLLVMType(
                         context->getLLVMContext()),
                     Nparts));

  return context->getBuilder()->CreatePointerCast(N_parts_ptr, data_type);
  // return BinaryBlockPlugin::getDataPointersForFile(i);
}

void AeolusPlugin::freeDataPointersForFile(ParallelContext *context, size_t i,
                                           Value *v) const {
  LLVMContext &llvmContext = context->getLLVMContext();
  auto data_type = PointerType::getUnqual(Type::getInt8PtrTy(llvmContext));
  auto casted = context->getBuilder()->CreatePointerCast(v, data_type);
  Value *this_ptr = context->getBuilder()->CreateIntToPtr(
      context->createInt64((uintptr_t)this), Type::getInt8PtrTy(llvmContext));
  createCall2("freeDataPointerForFile_runtime", {casted, this_ptr}, context);
}

std::pair<llvm::Value *, llvm::Value *> AeolusPlugin::getPartitionSizes(
    ParallelContext *context, llvm::Value *session_ptr) const {
  IRBuilder<> *Builder = context->getBuilder();

  IntegerType *sizeType = context->createSizeType();

  Value *this_ptr = context->getBuilder()->CreateIntToPtr(
      context->createInt64((uintptr_t)this),
      Type::getInt8PtrTy(context->getLLVMContext()));

  Value *N_parts_ptr = Builder->CreatePointerCast(
      context->gen_call(::getNumOfTuplesPerPartition_runtime,
                        {context->CreateGlobalString(fnamePrefix.c_str()),
                         session_ptr, this_ptr}),
      PointerType::getUnqual(ArrayType::get(sizeType, Nparts)));

  Value *max_pack_size = ConstantInt::get(sizeType, 0);
  for (size_t i = 0; i < Nparts; ++i) {
    auto v = Builder->CreateLoad(Builder->CreateInBoundsGEP(
        N_parts_ptr, {context->createSizeT(0), context->createSizeT(i)}));
    auto cond = Builder->CreateICmpUGT(max_pack_size, v);
    max_pack_size = Builder->CreateSelect(cond, max_pack_size, v);
  }

  return {N_parts_ptr, max_pack_size};
  // return BinaryBlockPlugin::getPartitionSizes();
}

void AeolusPlugin::freePartitionSizes(ParallelContext *context,
                                      Value *v) const {
  Value *this_ptr = context->getBuilder()->CreateIntToPtr(
      context->createInt64((uintptr_t)this),
      Type::getInt8PtrTy(context->getLLVMContext()));
  context->gen_call(::freeNumOfTuplesPerPartition_runtime, {v, this_ptr});
}

struct session {
  xid_t xid;
  master_version_t master_ver;
  delta_id_t delta_ver;
  partition_id_t partition_id;
};

extern session TheSession;
session TheSession;

void update_query(storage::Table *tbl, int64_t vid, column_id_t col_update_idx,
                  void *rec) {
  auto xid = TheSession.xid;
  auto master_ver = TheSession.master_ver;
  auto delta_ver = TheSession.delta_ver;
  auto partition_id = TheSession.partition_id;
  LOG(INFO) << xid << " " << master_ver << " " << delta_ver << " "
            << partition_id;
  LOG(INFO) << col_update_idx;

  auto *hash_ptr = (global_conf::IndexVal *)tbl->p_index->find(vid);
  if (!hash_ptr->write_lck.try_lock()) {
    throw proteus::abort();
  }
  hash_ptr->latch.acquire();

  tbl->updateRecord(xid, hash_ptr, rec, delta_ver, &col_update_idx, 1,
                    master_ver);
  hash_ptr->t_min = xid;
  hash_ptr->write_lck.unlock();
  hash_ptr->latch.release();
  LOG(INFO) << "UPDATE QUERY SUCCESS";
}

void AeolusPlugin::updateValueEagerInternal(
    ParallelContext *context, ProteusValue rid, ProteusValue value,
    const ExpressionType *type, const std::string &relName, uint8_t index) {
  switch (type->getTypeID()) {
    case RECORD: {
      auto attrs = ((const RecordType *)type)->getArgs();

      size_t i = 0;
      for (const auto &attr : attrs) {
        auto ii = CatalogParser::getInstance().getInputInfo(relName);
        LOG(INFO) << ii;
        auto indx = dynamic_cast<const RecordType &>(
                        dynamic_cast<BagType *>(ii->exprType)->getNestedType())
                        .getIndex(attr);
        LOG(INFO) << indx;

        ProteusValue partialFlush{
            context->getBuilder()->CreateExtractValue(value.value, i),
            context->createFalse()};
        updateValueEagerInternal(context, rid, partialFlush,
                                 attr->getOriginalType(), relName, indx);
        ++i;
      }

      return;
    }
    case STRING:
    case SET:
    case COMPOSITE:
    case BLOCK:
    case LIST:
    case BAG: {
      LOG(ERROR) << "Unsupported datatype: " << *type;
      throw runtime_error("Unsupported datatype");
    }
    default: {
      // TODO: consider using overloaded functions per type
      auto tbl = getRelation({relName}, "<" + pgType + ">");
      // TODO: iterate over components of record and set attribute indexes
      // correctly
      auto mem = context->toMem(value);
      context->gen_call(
          update_query,
          {context->CastPtrToLlvmPtr(
               llvm::Type::getInt8PtrTy(context->getLLVMContext()), tbl),
           rid.value, context->createInt8(index), mem.mem});
      return;
    }
  }
}

void AeolusPlugin::updateValueEager(ParallelContext *context, ProteusValue rid,
                                    ProteusValue value,
                                    const ExpressionType *type,
                                    const std::string &relName) {
  assert(type->getTypeID() == RECORD);
  updateValueEagerInternal(context, rid, value, type, relName, -1);
}
