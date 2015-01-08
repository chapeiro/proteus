/*
	RAW -- High-performance querying over raw, never-seen-before data.

							Copyright (c) 2014
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

#include "operators/reduce.hpp"

Reduce::Reduce(Monoid acc, expressions::Expression* outputExpr,
		expressions::Expression* pred, RawOperator* const child,
		RawContext* context) :
		UnaryRawOperator(child), acc(acc),
		outputExpr(outputExpr), pred(pred), context(context) {

	IRBuilder<>* Builder = context->getBuilder();
	LLVMContext& llvmContext = context->getLLVMContext();
	Function *f = Builder->GetInsertBlock()->getParent();

	Type* int1Type = Type::getInt1Ty(llvmContext);
	Type* int32Type = Type::getInt32Ty(llvmContext);
	Type* doubleType = Type::getDoubleTy(llvmContext);

	typeID outputType = outputExpr->getExpressionType()->getTypeID();
	switch (acc) {
	case SUM: {
		switch (outputType) {
		case INT: {
			mem_accumulating = context->CreateEntryBlockAlloca(f,
					string("reduce_acc"), int32Type);
			Value *val_zero = Builder->getInt32(0);
			Builder->CreateStore(val_zero, mem_accumulating);
			break;
		}
		case FLOAT: {
			Type* doubleType = Type::getDoubleTy(llvmContext);
			mem_accumulating = context->CreateEntryBlockAlloca(f,
					string("reduce_acc"), doubleType);
			Value *val_zero = ConstantFP::get(doubleType, 0.0);
			Builder->CreateStore(val_zero, mem_accumulating);
			break;
		}
		default: {
			string error_msg = string(
					"[Reduce: ] Sum/Multiply/Max operate on numerics");
			LOG(ERROR)<< error_msg;
			throw runtime_error(error_msg);
		}
		}
		break;
	}
	case MULTIPLY: {
		switch (outputType) {
		case INT: {
			mem_accumulating = context->CreateEntryBlockAlloca(f,
					string("reduce_acc"), int32Type);
			Value *val_zero = Builder->getInt32(1);
			Builder->CreateStore(val_zero, mem_accumulating);
			break;
		}
		case FLOAT: {
			mem_accumulating = context->CreateEntryBlockAlloca(f,
					string("reduce_acc"), doubleType);
			Value *val_zero = ConstantFP::get(doubleType, 1.0);
			Builder->CreateStore(val_zero, mem_accumulating);
			break;
		}
		default: {
			string error_msg = string(
					"[Reduce: ] Sum/Multiply/Max operate on numerics");
			LOG(ERROR)<< error_msg;
			throw runtime_error(error_msg);
		}
		}
		break;
	}
	case MAX: {
		switch (outputType) {
		case INT: {
			mem_accumulating = context->CreateEntryBlockAlloca(f,
					string("reduce_acc"), int32Type);
			Value *val_zero = Builder->getInt32(0);
			Builder->CreateStore(val_zero, mem_accumulating);
			break;
		}
		case FLOAT: {
			mem_accumulating = context->CreateEntryBlockAlloca(f,
					string("reduce_acc"), doubleType);
			Value *val_zero = ConstantFP::get(doubleType, 0.0);
			Builder->CreateStore(val_zero, mem_accumulating);
			break;
		}
		default: {
			string error_msg = string(
					"[Reduce: ] Sum/Multiply/Max operate on numerics");
			LOG(ERROR)<< error_msg;
			throw runtime_error(error_msg);
		}
		}
		break;
	}
	case OR:	{
		switch (outputType) {
		case BOOL: {
			mem_accumulating = context->CreateEntryBlockAlloca(f,
					string("reduce_acc"), int1Type);
			Value *val_zero = Builder->getInt1(0);
			Builder->CreateStore(val_zero, mem_accumulating);
			break;
		}
		default: {
			string error_msg = string("[Reduce: ] Or/And operate on booleans");
			LOG(ERROR)<< error_msg;
			throw runtime_error(error_msg);
		}

		}
		break;
	}
	case AND: {
		switch (outputType) {
		case BOOL: {
			mem_accumulating = context->CreateEntryBlockAlloca(f,
					string("reduce_acc"), int1Type);
			Value *val_zero = Builder->getInt1(1);
			Builder->CreateStore(val_zero, mem_accumulating);
			break;
		}
		default: {
			string error_msg = string("[Reduce: ] Or/And operate on booleans");
			LOG(ERROR)<< error_msg;
			throw runtime_error(error_msg);
		}

		}
		break;
	}
	case UNION:
	case BAGUNION: {
		string error_msg = string("[Reduce: ] Not implemented yet");
		LOG(ERROR)<< error_msg;
		throw runtime_error(error_msg);
	}
	case APPEND: {
		RawCatalog& catalog = RawCatalog::getInstance();
		map<int, Value*> *reduceSetHT = new map<int, Value*>();
		catalog.setReduceHT(reduceSetHT);
		break;
	}
	default: {
		string error_msg = string("[Reduce: ] Unknown accumulator");
		LOG(ERROR)<< error_msg;
		throw runtime_error(error_msg);
	}
	}
}

void Reduce::produce()	const { getChild()->produce(); }

void Reduce::consume(RawContext* const context, const OperatorState& childState) const {
	generate(context, childState);
}

void Reduce::generate(RawContext* const context, const OperatorState& childState) const
{
	IRBuilder<>* Builder = context->getBuilder();
	LLVMContext& llvmContext = context->getLLVMContext();
	Function *TheFunction = Builder->GetInsertBlock()->getParent();

	switch (acc) {
	case SUM:
		generateSum(context, childState);
		break;
	case MULTIPLY:
		generateMul(context, childState);
		break;
	case MAX:
		generateMax(context, childState);
		break;
	case OR:
		generateOr(context, childState);
		break;
	case AND:
		generateAnd(context, childState);
		break;
	case UNION:
//		generateUnion(context, childState);
//		break;
	case BAGUNION:
//		generateBagUnion(context, childState);
//		break;
	case APPEND:
//		generateAppend(context, childState);
//		break;
	default: {
		string error_msg = string("[Reduce: ] Unknown accumulator");
		LOG(ERROR)<< error_msg;
		throw runtime_error(error_msg);
	}
	}
}

void Reduce::generateSum(RawContext* const context, const OperatorState& childState) const	{
	IRBuilder<>* Builder = context->getBuilder();
	LLVMContext& llvmContext = context->getLLVMContext();
	Function *TheFunction = Builder->GetInsertBlock()->getParent();

	//Generate condition
	ExpressionGeneratorVisitor predExprGenerator = ExpressionGeneratorVisitor(context, childState);
	RawValue condition = pred->accept(predExprGenerator);
	/**
	 * Predicate Evaluation:
	 */
	BasicBlock* entryBlock = Builder->GetInsertBlock();
	BasicBlock *endBlock = BasicBlock::Create(llvmContext, "reduceCondEnd", TheFunction);
	BasicBlock *ifBlock;
	context->CreateIfBlock(context->getGlobalFunction(), "reduceIfCond",
					&ifBlock, endBlock);

	/**
	 * IF Block
	 */
	Builder->SetInsertPoint(ifBlock);
	ExpressionGeneratorVisitor outputExprGenerator = ExpressionGeneratorVisitor(context, childState);
	RawValue val_output = outputExpr->accept(outputExprGenerator);

	switch (outputExpr->getExpressionType()->getTypeID()) {
	case INT: {
		Builder->SetInsertPoint(entryBlock);
		Builder->CreateCondBr(condition.value, ifBlock, endBlock);
		Builder->SetInsertPoint(ifBlock);

		Value* val_accumulating = Builder->CreateLoad(mem_accumulating);
		Value* val_new = Builder->CreateAdd(val_accumulating, val_output.value);
		Builder->CreateStore(val_new, mem_accumulating);

		Builder->CreateBr(endBlock);

		//Prepare final result output
		Builder->SetInsertPoint(context->getEndingBlock());
#ifdef DEBUG
		std::vector<Value*> ArgsV;
		Function* debugInt = context->getFunction("printi");
		Value* finalResult = Builder->CreateLoad(mem_accumulating);
		ArgsV.push_back(finalResult);
		Builder->CreateCall(debugInt, ArgsV);
#endif
		//Back to 'normal' flow
		Builder->SetInsertPoint(ifBlock);
		break;
	}
	case FLOAT: {
		Type* doubleType = Type::getDoubleTy(llvmContext);
		Builder->SetInsertPoint(entryBlock);
		Builder->CreateCondBr(condition.value, ifBlock, endBlock);
		Builder->SetInsertPoint(ifBlock);

		Value* val_accumulating = Builder->CreateLoad(mem_accumulating);
		Value* val_new = Builder->CreateFAdd(val_accumulating,val_output.value);
		Builder->CreateStore(val_new,mem_accumulating);

		//Prepare final result output
		Builder->SetInsertPoint(context->getEndingBlock());
#ifdef DEBUG
		std::vector<Value*> ArgsV;
		Function* debugFloat = context->getFunction("printFloat");
		Value* finalResult = Builder->CreateLoad(mem_accumulating);
		ArgsV.push_back(finalResult);
		Builder->CreateCall(debugFloat, ArgsV);
#endif

		break;
	}
	default: {
		string error_msg = string(
				"[Reduce: ] Sum accumulator operates on numerics");
		LOG(ERROR)<< error_msg;
		throw runtime_error(error_msg);
	}
	}

	/**
	 * END Block
	 */
	Builder->SetInsertPoint(endBlock);
}

void Reduce::generateMul(RawContext* const context, const OperatorState& childState) const	{
	IRBuilder<>* Builder = context->getBuilder();
	LLVMContext& llvmContext = context->getLLVMContext();
	Function *TheFunction = Builder->GetInsertBlock()->getParent();

	//Generate condition
	ExpressionGeneratorVisitor predExprGenerator = ExpressionGeneratorVisitor(context, childState);
	RawValue condition = pred->accept(predExprGenerator);
	/**
	 * Predicate Evaluation:
	 */
	BasicBlock* entryBlock = Builder->GetInsertBlock();
	BasicBlock *endBlock = BasicBlock::Create(llvmContext, "reduceCondEnd", TheFunction);
	BasicBlock *ifBlock;
	context->CreateIfBlock(context->getGlobalFunction(), "reduceIfCond",
					&ifBlock, endBlock);

	/**
	 * IF Block
	 */
	Builder->SetInsertPoint(ifBlock);
	ExpressionGeneratorVisitor outputExprGenerator = ExpressionGeneratorVisitor(context, childState);
	RawValue val_output = outputExpr->accept(outputExprGenerator);

	switch (outputExpr->getExpressionType()->getTypeID()) {
	case INT: {
		Builder->SetInsertPoint(entryBlock);
		Builder->CreateCondBr(condition.value, ifBlock, endBlock);
		Builder->SetInsertPoint(ifBlock);

		Value* val_accumulating = Builder->CreateLoad(mem_accumulating);
		Value* val_new = Builder->CreateMul(val_accumulating, val_output.value);
		Builder->CreateStore(val_new, mem_accumulating);

		Builder->CreateBr(endBlock);

		//Prepare final result output
		Builder->SetInsertPoint(context->getEndingBlock());
#ifdef DEBUG
		std::vector<Value*> ArgsV;
		Function* debugInt = context->getFunction("printi");
		Value* finalResult = Builder->CreateLoad(mem_accumulating);
		ArgsV.push_back(finalResult);
		Builder->CreateCall(debugInt, ArgsV);
#endif
		//Back to 'normal' flow
		Builder->SetInsertPoint(ifBlock);
		break;
	}
	case FLOAT: {
		Type* doubleType = Type::getDoubleTy(llvmContext);
		Builder->SetInsertPoint(entryBlock);
		Builder->CreateCondBr(condition.value, ifBlock, endBlock);
		Builder->SetInsertPoint(ifBlock);

		Value* val_accumulating = Builder->CreateLoad(mem_accumulating);
		Value* val_new = Builder->CreateFMul(val_accumulating,val_output.value);
		Builder->CreateStore(val_new,mem_accumulating);

		//Prepare final result output
		Builder->SetInsertPoint(context->getEndingBlock());
#ifdef DEBUG
		std::vector<Value*> ArgsV;
		Function* debugFloat = context->getFunction("printFloat");
		Value* finalResult = Builder->CreateLoad(mem_accumulating);
		ArgsV.push_back(finalResult);
		Builder->CreateCall(debugFloat, ArgsV);
#endif

		break;
	}
	default: {
		string error_msg = string(
				"[Reduce: ] Sum accumulator operates on numerics");
		LOG(ERROR)<< error_msg;
		throw runtime_error(error_msg);
	}
	}

	/**
	 * END Block
	 */
	Builder->SetInsertPoint(endBlock);
}

void Reduce::generateMax(RawContext* const context, const OperatorState& childState) const	{
	IRBuilder<>* Builder = context->getBuilder();
	LLVMContext& llvmContext = context->getLLVMContext();
	Function *TheFunction = Builder->GetInsertBlock()->getParent();

	//Generate condition
	ExpressionGeneratorVisitor predExprGenerator = ExpressionGeneratorVisitor(context, childState);
	RawValue condition = pred->accept(predExprGenerator);

	/**
	 * Predicate Evaluation:
	 */
	BasicBlock* entryBlock = Builder->GetInsertBlock();
	BasicBlock *endBlock = BasicBlock::Create(llvmContext, "reduceCondEnd", TheFunction);
	BasicBlock *ifBlock;
	context->CreateIfBlock(context->getGlobalFunction(), "reduceIfCond",
						&ifBlock, endBlock);

	/**
	 * IF Block
	 */
	Builder->SetInsertPoint(ifBlock);
	ExpressionGeneratorVisitor outputExprGenerator = ExpressionGeneratorVisitor(context, childState);
	RawValue val_output = outputExpr->accept(outputExprGenerator);

	switch (outputExpr->getExpressionType()->getTypeID()) {
	case INT: {
		Builder->SetInsertPoint(entryBlock);
		Builder->CreateCondBr(condition.value, ifBlock, endBlock);
		Builder->SetInsertPoint(ifBlock);

		/**
		 * if(curr > max) max = curr;
		 */
		BasicBlock* ifGtMaxBlock;
		context->CreateIfBlock(context->getGlobalFunction(), "reduceMaxCond",&ifGtMaxBlock,endBlock);
		Value* val_accumulating = Builder->CreateLoad(mem_accumulating);
		Value* maxCondition = Builder->CreateICmpSGT(val_output.value,val_accumulating);
		Builder->CreateCondBr(maxCondition,ifGtMaxBlock,endBlock);

		Builder->SetInsertPoint(ifGtMaxBlock);
		Builder->CreateStore(val_output.value,mem_accumulating);
		Builder->CreateBr(endBlock);

		//Prepare final result output
		Builder->SetInsertPoint(context->getEndingBlock());
#ifdef DEBUG
		std::vector<Value*> ArgsV;
		Function* debugInt = context->getFunction("printi");
		Value* finalResult = Builder->CreateLoad(mem_accumulating);
		ArgsV.push_back(finalResult);
		Builder->CreateCall(debugInt, ArgsV);
#endif
		//Back to 'normal' flow
		Builder->SetInsertPoint(ifGtMaxBlock);

		//Branch Instruction to reach endBlock will be flushed after end of switch
		break;
	}
	case FLOAT: {
		Type* doubleType = Type::getDoubleTy(llvmContext);
		Builder->SetInsertPoint(entryBlock);
		Builder->CreateCondBr(condition.value, ifBlock, endBlock);
		Builder->SetInsertPoint(ifBlock);

		/**
		 * if(curr > max) max = curr;
		 */
		BasicBlock* ifGtMaxBlock;
		context->CreateIfBlock(context->getGlobalFunction(), "reduceMaxCond",&ifGtMaxBlock, endBlock);
		Value* val_accumulating = Builder->CreateLoad(mem_accumulating);
		Value* maxCondition = Builder->CreateFCmpOGT(val_output.value,
				val_accumulating);
		Builder->CreateCondBr(maxCondition, ifGtMaxBlock, endBlock);

		Builder->SetInsertPoint(ifGtMaxBlock);
		Builder->CreateStore(val_output.value, mem_accumulating);
		Builder->CreateBr(endBlock);

		//Prepare final result output
		Builder->SetInsertPoint(context->getEndingBlock());
#ifdef DEBUG
		std::vector<Value*> ArgsV;
		Function* debugFloat = context->getFunction("printFloat");
		Value* finalResult = Builder->CreateLoad(mem_accumulating);
		ArgsV.push_back(finalResult);
		Builder->CreateCall(debugFloat, ArgsV);
#endif
		//Back to 'normal' flow
		break;
	}
	default: {
		string error_msg = string(
				"[Reduce: ] Sum accumulator operates on numerics");
		LOG(ERROR)<< error_msg;
		throw runtime_error(error_msg);
	}
	}

	/**
	 * END Block
	 */
	Builder->SetInsertPoint(endBlock);
}

void Reduce::generateOr(RawContext* const context, const OperatorState& childState) const {
	IRBuilder<>* Builder = context->getBuilder();
	LLVMContext& llvmContext = context->getLLVMContext();
	Function *TheFunction = Builder->GetInsertBlock()->getParent();

	//Generate condition
	ExpressionGeneratorVisitor predExprGenerator = ExpressionGeneratorVisitor(
			context, childState);
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
	 * IF Block
	 */
	Builder->SetInsertPoint(ifBlock);
	ExpressionGeneratorVisitor outputExprGenerator = ExpressionGeneratorVisitor(
			context, childState);

	switch (outputExpr->getExpressionType()->getTypeID()) {
	case BOOL: {
		Builder->SetInsertPoint(entryBlock);
		Builder->CreateCondBr(condition.value, ifBlock, endBlock);
		Builder->SetInsertPoint(ifBlock);

		Value* val_accumulating = Builder->CreateLoad(mem_accumulating);

		RawValue val_output = outputExpr->accept(outputExprGenerator);
		Value* val_new = Builder->CreateOr(val_accumulating, val_output.value);
		Builder->CreateStore(val_new, mem_accumulating);

		Builder->CreateBr(endBlock);

		//Prepare final result output
		Builder->SetInsertPoint(context->getEndingBlock());
#ifdef DEBUG
		std::vector<Value*> ArgsV;
		Function* debugBoolean = context->getFunction("printBoolean");
		Value* finalResult = Builder->CreateLoad(mem_accumulating);
		ArgsV.push_back(finalResult);
		Builder->CreateCall(debugBoolean, ArgsV);
#endif
		break;
	}
	default: {
		string error_msg = string(
				"[Reduce: ] Or accumulator operates on numerics");
		LOG(ERROR)<< error_msg;
		throw runtime_error(error_msg);
	}
	}

	/**
	 * END Block
	 */
	Builder->SetInsertPoint(endBlock);
}

void Reduce::generateAnd(RawContext* const context, const OperatorState& childState) const {
	IRBuilder<>* Builder = context->getBuilder();
	LLVMContext& llvmContext = context->getLLVMContext();
	Function *TheFunction = Builder->GetInsertBlock()->getParent();

	//Generate condition
	ExpressionGeneratorVisitor predExprGenerator = ExpressionGeneratorVisitor(
			context, childState);
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
	 * IF Block
	 */
	Builder->SetInsertPoint(ifBlock);
	ExpressionGeneratorVisitor outputExprGenerator = ExpressionGeneratorVisitor(
			context, childState);

	switch (outputExpr->getExpressionType()->getTypeID()) {
	case BOOL: {
		Builder->SetInsertPoint(entryBlock);
		Builder->CreateCondBr(condition.value, ifBlock, endBlock);
		Builder->SetInsertPoint(ifBlock);

		Value* val_accumulating = Builder->CreateLoad(mem_accumulating);

		RawValue val_output = outputExpr->accept(outputExprGenerator);
		Value* val_new = Builder->CreateAnd(val_accumulating, val_output.value);
		Builder->CreateStore(val_new, mem_accumulating);

		Builder->CreateBr(endBlock);

		//Prepare final result output
		Builder->SetInsertPoint(context->getEndingBlock());
#ifdef DEBUG
		std::vector<Value*> ArgsV;
		Function* debugBoolean = context->getFunction("printBoolean");
		Value* finalResult = Builder->CreateLoad(mem_accumulating);
		ArgsV.push_back(finalResult);
		Builder->CreateCall(debugBoolean, ArgsV);
#endif
		break;
	}
	default: {
		string error_msg = string(
				"[Reduce: ] Or accumulator operates on numerics");
		LOG(ERROR)<< error_msg;
		throw runtime_error(error_msg);
	}
	}

	/**
	 * END Block
	 */
	Builder->SetInsertPoint(endBlock);
}

//Flush out whatever you received
//FIXME Need 'output plugin'
void Reduce::generateUnion(RawContext* const context, const OperatorState& childState) const	{

}

//Flush out whatever you received
//FIXME Need 'output plugin'
void Reduce::generateBagUnion(RawContext* const context, const OperatorState& childState) const	{

}

//Materializes collection (in HT?)
//Use the catalog for the materialization
void Reduce::generateAppend(RawContext* const context, const OperatorState& childState) const	{

}