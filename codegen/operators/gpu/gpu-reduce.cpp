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

#include "operators/gpu/gpu-reduce.hpp"
#include "operators/gpu/gmonoids.hpp"

namespace opt {

GpuReduce::GpuReduce(vector<Monoid>             accs,
            vector<expressions::Expression*>    outputExprs,
            expressions::Expression*            pred, 
            RawOperator* const                  child,
            GpuRawContext*                      context)
        : Reduce(accs, outputExprs, pred, child, context, false) {
    for (const auto &expr: outputExprs){
        if (!expr->getExpressionType()->isPrimitive()){
            string error_msg("[GpuReduce: ] Currently only supports primitive types");
            LOG(ERROR) << error_msg;
            throw runtime_error(error_msg);
        }
    }
}

void GpuReduce::produce() {
    flushResults = flushResults && !getParent(); //TODO: is this the best way to do it ?
    generate_flush();

    ((GpuRawContext *) context)->popNewPipeline();

    auto flush_pip = ((GpuRawContext *) context)->removeLatestPipeline();
    flush_fun = flush_pip->getKernel();

    ((GpuRawContext *) context)->pushNewPipeline(flush_pip);

    assert(mem_accumulators.empty());
    if (mem_accumulators.empty()){
        vector<Monoid>::const_iterator itAcc;
        vector<expressions::Expression*>::const_iterator itExpr;
        itAcc = accs.begin();
        itExpr = outputExprs.begin();

        int aggsNo = accs.size();
        /* Prepare accumulator FOREACH outputExpr */
        for (; itAcc != accs.end(); itAcc++, itExpr++) {
            Monoid acc = *itAcc;
            expressions::Expression *outputExpr = *itExpr;
            bool flushDelim = (aggsNo > 1) && (itAcc != accs.end() - 1);
            bool is_first   = (itAcc == accs.begin()  );
            bool is_last    = (itAcc == accs.end() - 1);
            size_t mem_accumulator = resetAccumulator(outputExpr, acc, flushDelim, is_first, is_last);
            mem_accumulators.push_back(mem_accumulator);
        }
    }

    getChild()->produce();
}

void GpuReduce::consume(RawContext* const context, const OperatorState& childState) {
    consume((GpuRawContext *) context, childState);
}

void GpuReduce::consume(GpuRawContext* const context, const OperatorState& childState) {
    IRBuilder<>* Builder = context->getBuilder();
    LLVMContext& llvmContext = context->getLLVMContext();
    Function *TheFunction = Builder->GetInsertBlock()->getParent();

    int aggsNo = accs.size();

    //Generate condition
    ExpressionGeneratorVisitor predExprGenerator{context, childState};
    RawValue condition = pred->accept(predExprGenerator);
    /**
     * Predicate Evaluation:
     */
    BasicBlock* entryBlock = Builder->GetInsertBlock();
    BasicBlock *endBlock = BasicBlock::Create(llvmContext, "reduceCondEnd",
            TheFunction);
    BasicBlock *ifBlock;
    context->CreateIfBlock(context->getGlobalFunction(), "reduceIfCond",
            &ifBlock, endBlock);

    /**
     * IF(pred) Block
     */
    RawValue val_output;
    Builder->SetInsertPoint(entryBlock);

    Builder->CreateCondBr(condition.value, ifBlock, endBlock);

    Builder->SetInsertPoint(ifBlock);

    vector<Monoid                   >::const_iterator itAcc ;
    vector<expressions::Expression *>::const_iterator itExpr;
    vector<size_t                   >::const_iterator itMem ;
    /* Time to Compute Aggs */
    itAcc  = accs.begin();
    itExpr = outputExprs.begin();
    itMem  = mem_accumulators.begin();

    for (; itAcc != accs.end(); itAcc++, itExpr++, itMem++) {
        Monoid                      acc                 = *itAcc    ;
        expressions::Expression   * outputExpr          = *itExpr   ;
        Value                     * mem_accumulating    = NULL      ;

        switch (acc) {
        case SUM:
        case MULTIPLY:
        case MAX:
        case OR:
        case AND:{
            BasicBlock *cBB = Builder->GetInsertBlock();
            Builder->SetInsertPoint(context->getCurrentEntryBlock());

            mem_accumulating = context->getStateVar(*itMem);

            Constant * acc_init = getIdentityElementIfSimple(
                acc,
                outputExpr->getExpressionType(),
                context
            );
            Value * acc_mem  = context->CreateEntryBlockAlloca("acc", acc_init->getType());
            Builder->CreateStore(acc_init, acc_mem);

            Builder->SetInsertPoint(context->getEndingBlock());
            generate(acc, outputExpr, context, childState, acc_mem, mem_accumulating);

            Builder->SetInsertPoint(cBB);

            ExpressionGeneratorVisitor outputExprGenerator{context, childState};

            // Load accumulator -> acc_value
            RawValue acc_value;
            acc_value.value  = Builder->CreateLoad(acc_mem);
            acc_value.isNull = context->createFalse();

            // new_value = acc_value op outputExpr
            expressions::Expression * val = new expressions::RawValueExpression(outputExpr->getExpressionType(), acc_value);
            expressions::Expression * upd = toExpression(acc, val, outputExpr);
            assert(upd && "Monoid is not convertible to expression!");
            RawValue new_val = upd->accept(outputExprGenerator);

            // store new_val to accumulator
            Builder->CreateStore(new_val.value, acc_mem);
            break;
        }
        case BAGUNION:
        case APPEND:
            //      generateAppend(context, childState);
            //      break;
        case UNION:
        default: {
            string error_msg = string(
                    "[Reduce: ] Unknown / Still Unsupported accumulator");
            LOG(ERROR)<< error_msg;
            throw runtime_error(error_msg);
        }
        }
    }

    Builder->CreateBr(endBlock);

    /**
     * END Block
     */
    Builder->SetInsertPoint(endBlock);
}





// void GpuReduce::consume(RawContext* const context, const OperatorState& childState) {
//     Reduce::consume(context, childState);
//     generate(context, childState);
// }

// void GpuReduce::generate(RawContext* const context, const OperatorState& childState) const{
//     vector<Monoid                   >::const_iterator itAcc    ;
//     vector<expressions::Expression *>::const_iterator itExpr   ;
//     vector<size_t                   >::const_iterator itMem    ;
//     vector<int                      >::const_iterator itID;
//     /* Time to Compute Aggs */
//     itAcc       = accs.begin();
//     itExpr      = outputExprs.begin();
//     itMem       = mem_accumulators.begin();
//     itID        = out_ids.begin();

//     IRBuilder<>* Builder        = context->getBuilder();
    
//     for (; itAcc != accs.end(); itAcc++, itExpr++, itMem++, ++itID) {
//         Monoid                      acc                     = *itAcc ;
//         expressions::Expression *   outputExpr              = *itExpr;
//         Value                   *   mem_accumulating        = NULL   ;

//         BasicBlock* insBlock = Builder->GetInsertBlock();
        
//         BasicBlock* entryBlock = context->getCurrentEntryBlock();
//         Builder->SetInsertPoint(entryBlock);

//         Value * global_acc_ptr = ((const GpuRawContext *) context)->getStateVar(*itID);

//         Builder->SetInsertPoint(insBlock);

//         switch (acc) {
//         case MAX:
//         case SUM:
//         case OR:
//         case AND:
//             mem_accumulating = context->getStateVar(*itMem);
//             generate(acc, outputExpr, (GpuRawContext * const) context, childState, mem_accumulating, global_acc_ptr);
//             break;
//         case MULTIPLY:
//         case BAGUNION:
//         case APPEND:
//         case UNION:
//         default: {
//             string error_msg = string(
//                     "[GpuReduce: ] Unknown / Still Unsupported accumulator");
//             LOG(ERROR)<< error_msg;
//             throw runtime_error(error_msg);
//         }
//         }
//     }

//     ((GpuRawContext *) context)->registerOpen (this, [this](RawPipeline * pip){this->open (pip);});
//     ((GpuRawContext *) context)->registerClose(this, [this](RawPipeline * pip){this->close(pip);});
// }

void GpuReduce::generate(const Monoid &m, expressions::Expression* outputExpr,
        GpuRawContext* const context, const OperatorState& state,
        Value * mem_accumulating, Value *global_accumulator_ptr) const {

    IRBuilder<>* Builder        = context->getBuilder();
    LLVMContext& llvmContext    = context->getLLVMContext();
    Function *TheFunction       = Builder->GetInsertBlock()->getParent();

    gpu::Monoid * gm = gpu::Monoid::get(m);



    global_accumulator_ptr->setName("reduce_" + std::to_string(*gm) + "_ptr");

    BasicBlock* entryBlock = Builder->GetInsertBlock();
    
    BasicBlock* endBlock = context->getEndingBlock();
    Builder->SetInsertPoint(endBlock);

    Value* val_accumulating = Builder->CreateLoad(mem_accumulating);

    // Warp aggregate
    Value * aggr = gm->createWarpAggregateTo0(context, val_accumulating);

    // Store warp-aggregated final result (available to all threads of each warp)
    Builder->CreateStore(aggr, mem_accumulating);

    // Write to global accumulator only from a single thread per warp

    BasicBlock * laneendBlock   = BasicBlock::Create(llvmContext, "reduceWriteEnd", TheFunction);
    BasicBlock * laneifBlock    = context->CreateIfBlock(TheFunction,
                                                            "reduceWriteIf",
                                                            laneendBlock);

    Value * laneid              = context->laneId();
    Builder->CreateCondBr(Builder->CreateICmpEQ(laneid, ConstantInt::get(laneid->getType(), 0), "is_pivot"), laneifBlock, laneendBlock);

    Builder->SetInsertPoint(laneifBlock);

    gm->createAtomicUpdate(context, global_accumulator_ptr, aggr, llvm::AtomicOrdering::Monotonic);

    Builder->CreateBr(laneendBlock);
    context->setEndingBlock(laneendBlock);

    Builder->SetInsertPoint(entryBlock);
}

void GpuReduce::open(RawPipeline * pip) const{
    for (size_t i = 0 ; i < out_ids.size() ; ++i){
        Type * llvm_type = ((const PrimitiveType *) outputExprs[i]->getExpressionType())->getLLVMType(context->getLLVMContext());

        size_t size_in_bytes = (llvm_type->getPrimitiveSizeInBits() + 7)/8;

        void * acc;
        gpu_run(cudaMalloc(&acc,    size_in_bytes));

        gpu_run(cudaMemset( acc, 0, size_in_bytes)); //FIXME: reset every type of (data, monoid)

        pip->setStateVar(out_ids[i], acc);
    }
}

void GpuReduce::close(RawPipeline * pip) const{
    // for (size_t i = 0 ; i < out_ids.size() ; ++i){
    //     gpu_run(cudaFree(pip->getStateVar<uint32_t *>(context, out_ids[i])));
    // }
    for (size_t i = 0 ; i < out_ids.size() ; ++i){
        uint32_t r;
        gpu_run(cudaMemcpy(&r, pip->getStateVar<uint32_t *>(out_ids[i]), sizeof(uint32_t), cudaMemcpyDefault));
        std::cout << r << std::endl;
    }
}



}


