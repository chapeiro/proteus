/*
    RAW -- High-performance querying over raw, never-seen-before data.

                            Copyright (c) 2017
        Data Intensive Applications and Systems Labaratory (DIAS)
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

#include "operators/exchange.hpp"
#include "expressions/expressions-generator.hpp"
#include <cstring>

void Exchange::produce() {
    generate_catch();
    
    context->popNewPipeline();

    catch_pip = context->removeLatestPipeline();

    //push new pipeline for the throw part
    context->pushNewCpuPipeline();

    ((GpuRawContext *) context)->registerOpen (this, [this](RawPipeline * pip){this->open (pip);});
    ((GpuRawContext *) context)->registerClose(this, [this](RawPipeline * pip){this->close(pip);});

    getChild()->produce();
}

void Exchange::generate_catch(){
    LLVMContext & llvmContext   = context->getLLVMContext();
    // IRBuilder<> * Builder       = context->getBuilder    ();
    // BasicBlock  * insBB         = Builder->GetInsertBlock();
    // Function    * F             = insBB->getParent();

    // Builder->SetInsertPoint(context->getCurrentEntryBlock());
    Plugin* pg = RawCatalog::getInstance().getPlugin(wantedFields[0]->getRelationName());

    Type  * charPtrType         = Type::getInt8PtrTy(llvmContext);
    Type  * int64Type           = Type::getInt64Ty  (llvmContext);
    Type  * ptr_t               = PointerType::get(charPtrType, 0);

    const PrimitiveType * ptoid = dynamic_cast<const PrimitiveType *>(pg->getOIDType());

    Type  * oidType             = ptoid->getLLVMType(llvmContext);

    // Value * subState   = ((GpuRawContext *) context)->getSubStateVar();
    // Value * subStatePtr = context->CreateEntryBlockAlloca(F, "subStatePtr", subState->getType());

    std::vector<Type *> param_typelist;
    for (size_t i = 0 ; i < wantedFields.size() ; ++i){
        param_typelist.push_back(wantedFields[i]->getOriginalType()->getLLVMType(llvmContext));
    }
    param_typelist.push_back(oidType); //cnt
    param_typelist.push_back(oidType); //oid

    // param_typelist.push_back(subStatePtr->getType());

    params_type = StructType::get(llvmContext, param_typelist);

    size_t buf_size = context->getModule()->getDataLayout().getTypeSizeInBits(params_type);
    for (int i = 0 ; i < numOfParents ; ++i){
        for (int j = 0 ; j < slack ; ++j) {
            free_pool[i].push(malloc(buf_size));
        }
    // std::cout << free_pool[i].size() << std::endl;
    }

    // context->SetInsertPoint(insBB);

    RecordAttribute tupleCnt        = RecordAttribute(wantedFields[0]->getRelationName(), "activeCnt", pg->getOIDType()); //FIXME: OID type for blocks ?
    RecordAttribute tupleIdentifier = RecordAttribute(wantedFields[0]->getRelationName(),  activeLoop, pg->getOIDType()); 

    // Generate catch code
    int p = context->appendParameter(PointerType::get(params_type, 0), true, true);
    context->setGlobalFunction();

    IRBuilder<> * Builder       = context->getBuilder    ();
    BasicBlock  * entryBB       = Builder->GetInsertBlock();
    Function    * F             = entryBB->getParent();

    context->setCurrentEntryBlock(entryBB);

    BasicBlock *mainBB = BasicBlock::Create(llvmContext, "main", F);

    BasicBlock *endBB = BasicBlock::Create(llvmContext, "end", F);
    context->setEndingBlock(endBB);

    Builder->SetInsertPoint(entryBB);

    Value * params = Builder->CreateLoad(context->getArgument(p));
    
    map<RecordAttribute, RawValueMemory> variableBindings;

    for (size_t i = 0 ; i < wantedFields.size() ; ++i){
        RawValueMemory mem_valWrapper;

        mem_valWrapper.mem    = context->CreateEntryBlockAlloca(F, wantedFields[i]->getAttrName() + "_ptr", wantedFields[i]->getOriginalType()->getLLVMType(llvmContext));
        mem_valWrapper.isNull = context->createFalse(); //FIMXE: should we alse transfer this information ?

        Value * param = Builder->CreateExtractValue(params, i);

        Builder->CreateStore(param, mem_valWrapper.mem);

        variableBindings[*(wantedFields[i])] = mem_valWrapper;
    }

    RawValueMemory mem_cntWrapper;
    mem_cntWrapper.mem    = context->CreateEntryBlockAlloca(F, "activeCnt", oidType);
    mem_cntWrapper.isNull = context->createFalse(); //FIMXE: should we alse transfer this information ?

    Value * cnt = Builder->CreateExtractValue(params, wantedFields.size()    );
    Builder->CreateStore(cnt, mem_cntWrapper.mem);

    variableBindings[tupleCnt] = mem_cntWrapper;



    RawValueMemory mem_oidWrapper;
    mem_oidWrapper.mem    = context->CreateEntryBlockAlloca(F, activeLoop, oidType);
    mem_oidWrapper.isNull = context->createFalse(); //FIMXE: should we alse transfer this information ?

    Value * oid = Builder->CreateExtractValue(params, wantedFields.size() + 1);
    Builder->CreateStore(oid, mem_oidWrapper.mem);

    variableBindings[tupleIdentifier] = mem_oidWrapper;

    Builder->SetInsertPoint(mainBB);

    OperatorState state{*this, variableBindings};
    getParent()->consume(context, state);

    Builder->CreateBr(endBB);

    Builder->SetInsertPoint(context->getCurrentEntryBlock());
    // Insert an explicit fall through from the current (entry) block to the CondBB.
    Builder->CreateBr(mainBB);


    Builder->SetInsertPoint(context->getEndingBlock());
    // Builder->CreateRetVoid();
}

void * Exchange::acquireBuffer(int target, bool polling){
    nvtxRangePushA("acq_buff");
    std::unique_lock<std::mutex> lock(free_pool_mutex[target]);

    if (free_pool[target].empty()){
        if (polling) {
            lock.unlock();
            nvtxRangePop();
            return NULL;
        }
        nvtxRangePushA("cv_buff");
        // std::cout << "Blocking" << std::endl;
        free_pool_cv[target].wait(lock, [this, target](){return !free_pool[target].empty();});
        nvtxRangePop();
    }

    nvtxRangePushA("stack_buff");
    void * buff = free_pool[target].top();
    free_pool[target].pop();
    nvtxRangePop();

    lock.unlock();
    nvtxRangePushA("got_acq_buff");
    return buff;
}

void   Exchange::releaseBuffer(int target, void * buff){
    nvtxRangePop();
    std::unique_lock<std::mutex> lock(ready_pool_mutex[target]);
    ready_pool[target].emplace(buff);
    ready_pool_cv[target].notify_all();
    lock.unlock();
    nvtxRangePop();
}

void   Exchange::freeBuffer(int target, void * buff){
    nvtxRangePushA("waiting_to_release");
    std::unique_lock<std::mutex> lock(free_pool_mutex[target]);
    nvtxRangePop();
    free_pool[target].emplace(buff);
    free_pool_cv[target].notify_all();
    lock.unlock();
}

bool   Exchange::get_ready(int target, void * &buff){
    // while (ready_pool[target].empty() && remaining_producers > 0);

    std::unique_lock<std::mutex> lock(ready_pool_mutex[target]);

    if (ready_pool[target].empty()){
        ready_pool_cv[target].wait(lock, [this, target](){return !ready_pool[target].empty() || (ready_pool[target].empty() && remaining_producers <= 0);});
    }
    
    if (ready_pool[target].empty()){
        assert(remaining_producers == 0);
        lock.unlock();
        return false;
    }

    buff = ready_pool[target].front();
    ready_pool[target].pop();

    lock.unlock();
    return true;
}

void   Exchange::fire(int target, RawPipelineGen * pipGen){
    nvtxRangePushA((pipGen->getName() + ":" + std::to_string(target)).c_str());

    // size_t packets = 0;
    // time_block t("Xchange pipeline (target=" + std::to_string(target) + "): ");

    set_exec_location_on_scope d(target_processors[target]);
    RawPipeline * pip = pipGen->getPipeline(target);
    std::this_thread::yield();

    nvtxRangePushA((pipGen->getName() + ":" + std::to_string(target) + "open").c_str());
    pip->open();
    nvtxRangePop();
    
    {
        time_block t("Texchange consume (target=" + std::to_string(target) + "): ");
        do {
            void * p;
            if (!get_ready(target, p)) break;
            // ++packets;
            nvtxRangePushA((pipGen->getName() + ":" + std::to_string(target) + "cons").c_str());

            // if (numOfParents > 1){
            //     int node;
            //     int r = move_pages(0, 1, (void **) p, NULL, &node, MPOL_MF_MOVE);
            //     assert((target & 1) == node);
            //     assert((target & 1) == (sched_getcpu() & 1));
            //     if (r != 0 || node < 0) {
            //         std::cout << *((void **)p) << " " << target << " " << node << " " << r << " " << strerror(-node); std::cout << std::endl; 
            //     }
            // }
            pip->consume(0, p);
            nvtxRangePop();

            freeBuffer(target, p); //FIXME: move this inside the generated code
            
            // std::this_thread::yield();
        } while (true);
    }

    nvtxRangePushA((pipGen->getName() + ":" + std::to_string(target) + "close").c_str());
    pip->close();
    nvtxRangePop();
    
    // std::cout << "Xchange pipeline packets (target=" << target << "): " << packets << std::endl;
    
    nvtxRangePop();
}

extern "C"{
    void * acquireBuffer(int target, Exchange * xch){
        return xch->acquireBuffer(target, false);
    }

    void * try_acquireBuffer(int target, Exchange * xch){
        return xch->acquireBuffer(target, true);
    }

    void   releaseBuffer(int target, Exchange * xch, void * buff){
        return xch->releaseBuffer(target, buff);
    }

    void   freeBuffer   (int target, Exchange * xch, void * buff){
        return xch->freeBuffer(target, buff);
    }
}

void Exchange::consume(RawContext* const context, const OperatorState& childState) {
    // Generate throw code
    LLVMContext & llvmContext   = context->getLLVMContext();
    IRBuilder<> * Builder       = context->getBuilder    ();
    BasicBlock  * insBB         = Builder->GetInsertBlock();
    Function    * F             = insBB->getParent();

    Type  * charPtrType         = Type::getInt8PtrTy(llvmContext);

    const map<RecordAttribute, RawValueMemory>& activeVars = childState.getBindings();

    Value * params      = UndefValue::get(params_type);

    Plugin* pg = RawCatalog::getInstance().getPlugin(wantedFields[0]->getRelationName());

    for (size_t i = 0 ; i < wantedFields.size() ; ++i){
        auto it = activeVars.find(*(wantedFields[i]));
        assert(it != activeVars.end());
        RawValueMemory mem_valWrapper = it->second;

        params = Builder->CreateInsertValue(params, Builder->CreateLoad(mem_valWrapper.mem), i);
    }

    BasicBlock *tryBB = BasicBlock::Create(llvmContext, "tryAcq", F);
    Builder->CreateBr(tryBB);
    Builder->SetInsertPoint(tryBB);

    Value * target;
    bool retry = true;
    if (hashExpr){
        ExpressionGeneratorVisitor exprGenerator{context, childState};
        target            = hashExpr->accept(exprGenerator).value;
        retry             = false;
    } else if (numa_local){//GPU local
        auto it = activeVars.find(*(wantedFields[0]));
        assert(it != activeVars.end());
        Function * getdev = context->getFunction("get_ptr_device_or_rand_for_host");

        Value * ptr       = Builder->CreateLoad(it->second.mem);
        ptr               = Builder->CreateBitCast(ptr, charPtrType);
        target            = Builder->CreateCall(getdev, vector<Value *>{ptr});
        retry             = false; //FIXME: Should we retry ?
    } else if (rand_local_cpu){
        auto it = activeVars.find(*(wantedFields[0]));
        assert(it != activeVars.end());
        Function * getdev = context->getFunction("get_rand_core_local_to_ptr");

        Value * ptr       = Builder->CreateLoad(it->second.mem);
        ptr               = Builder->CreateBitCast(ptr, charPtrType);
        target            = Builder->CreateCall(getdev, vector<Value *>{ptr});
        retry             = true;
    } else {
        Function * crand  = context->getFunction("rand");
        target            = Builder->CreateCall(crand, vector<Value *>{});
        retry             = true;
    }

    Value * numOfParentsV = ConstantInt::get((IntegerType *) target->getType(), ((uint64_t) numOfParents));

    target = Builder->CreateURem(target, numOfParentsV);
    target->setName("target");

    RecordAttribute tupleCnt        = RecordAttribute(wantedFields[0]->getRelationName(), "activeCnt", pg->getOIDType()); //FIXME: OID type for blocks ?
    RecordAttribute tupleIdentifier = RecordAttribute(wantedFields[0]->getRelationName(),  activeLoop, pg->getOIDType()); 
    
    auto it = activeVars.find(tupleCnt);
    assert(it != activeVars.end());

    RawValueMemory mem_cntWrapper = it->second;

    params = Builder->CreateInsertValue(params, Builder->CreateLoad(mem_cntWrapper.mem), wantedFields.size()    );

    it = activeVars.find(tupleIdentifier);
    assert(it != activeVars.end());

    RawValueMemory mem_oidWrapper = it->second;
    params = Builder->CreateInsertValue(params, Builder->CreateLoad(mem_oidWrapper.mem), wantedFields.size() + 1);

    Value * exchangePtr = ConstantInt::get(llvmContext, APInt(64, ((uint64_t) this)));
    Value * exchange    = Builder->CreateIntToPtr(exchangePtr, charPtrType);

    vector<Value *> kernel_args{target, exchange};
    
    Function * acquireBuffer;
    if (retry) acquireBuffer = context->getFunction("try_acquireBuffer");
    else       acquireBuffer = context->getFunction("acquireBuffer"    );

    Value * param_ptr   = Builder->CreateCall(acquireBuffer, kernel_args);

    Value * null_ptr    = ConstantPointerNull::get(((PointerType *) param_ptr->getType()));
    Value * is_null     = Builder->CreateICmpEQ(param_ptr, null_ptr);

    BasicBlock *contBB = BasicBlock::Create(llvmContext, "cont", F);
    Builder->CreateCondBr(is_null, tryBB, contBB);

    Builder->SetInsertPoint(contBB);

    param_ptr           = Builder->CreateBitCast(param_ptr, PointerType::get(params->getType(), 0));

    Builder->CreateStore(params, param_ptr);

    Function * releaseBuffer = context->getFunction("releaseBuffer");
    kernel_args.push_back(Builder->CreateBitCast(param_ptr, charPtrType));

    Builder->CreateCall(releaseBuffer, kernel_args);
}



void Exchange::open (RawPipeline * pip){
    // time_block t("Tinit_exchange: ");

    std::lock_guard<std::mutex> guard(init_mutex);
    
    if (firers.empty()){
        remaining_producers = producers;
        for (int i = 0 ; i < numOfParents ; ++i){
            firers.emplace_back(&Exchange::fire, this, i, catch_pip);
        }
    }
}

void Exchange::close(RawPipeline * pip){
    // time_block t("Tterm_exchange: ");

    int rem = --remaining_producers;
    assert(rem >= 0);

    for (int i = 0 ; i < numOfParents ; ++i) ready_pool_cv[i].notify_all();

    if (rem == 0) {
        nvtxRangePushA("Exchange_waiting_to_close");
        for (auto &t: firers) t.join();
        firers.clear();
        nvtxRangePop();
    }
}
