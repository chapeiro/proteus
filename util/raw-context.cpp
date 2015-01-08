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

#include "util/raw-context.hpp"

RawContext::RawContext(const string& moduleName) {
	llvmContext = new LLVMContext();
	LLVMContext& ctx = *llvmContext;
	TheBuilder = new IRBuilder<>(ctx);
	TheFPM = 0;
	TheExecutionEngine = 0;
	TheFunction = 0;
	codeEnd = NULL;
	availableFunctions = std::map<std::string, Function*>();

	InitializeNativeTarget();
	TheModule = new Module(moduleName, ctx);

	// Create the JIT.  This takes ownership of the module.
	std::string ErrStr;
	TheExecutionEngine = 
		EngineBuilder(TheModule).setErrorStr(&ErrStr).create();
	if (!TheExecutionEngine) {
		fprintf(stderr, "Could not create ExecutionEngine: %s\n",
				ErrStr.c_str());
		exit(1);
	}

	PassManager mpm;
	FunctionPassManager* OurFPM = new FunctionPassManager(TheModule);

	PassManagerBuilder pmb;
	pmb.OptLevel=0;
	pmb.populateModulePassManager(mpm);
	pmb.populateFunctionPassManager(*OurFPM);

	// Set up the optimizer pipeline.  Start with registering info about how the
	// target lays out data structures.
	OurFPM->add(new DataLayout(*TheExecutionEngine->getDataLayout()));
	//Provide basic AliasAnalysis support for GVN.
	OurFPM->add(createBasicAliasAnalysisPass());
	// Promote allocas to registers.
	OurFPM->add(createPromoteMemoryToRegisterPass());
	// Do simple "peephole" optimizations and bit-twiddling optzns.
	OurFPM->add(createInstructionCombiningPass());
	// Reassociate expressions.
	OurFPM->add(createReassociatePass());
	// Eliminate Common SubExpressions.
	OurFPM->add(createGVNPass());
	// Simplify the control flow graph (deleting unreachable blocks, etc).
	OurFPM->add(createCFGSimplificationPass());

	mpm.add(createFunctionInliningPass());
	mpm.add(createAlwaysInlinerPass());
	mpm.add(createBBVectorizePass());
	mpm.run(*TheModule);


	OurFPM->doInitialization();
	TheFPM = OurFPM;

	llvm::Type* int_type = Type::getInt32Ty(ctx);
	std::vector<Type*> Ints(1,int_type);
	FunctionType *FT = FunctionType::get(Type::getInt32Ty(ctx),Ints, false);
	registerFunctions(*this);
	Function *F = Function::Create(FT, Function::ExternalLinkage, 
		moduleName, TheModule);

	//Setting the 'global' function
	TheFunction = F;
	// Create a new basic block to start insertion into.
	BasicBlock *BB = BasicBlock::Create(ctx, "entry", F);
	TheBuilder->SetInsertPoint(BB);
}

void RawContext::prepareFunction(Function *F) {

	//FIXME Have a (tmp) return value for now at this point
	TheBuilder->CreateRet(TheBuilder->getInt32(114));

	LOG(INFO) << "[Prepare Function: ] Exit"; //and dump code so far";
#ifdef DEBUG
	//getModule()->dump();
#endif
	// Validate the generated code, checking for consistency.
	verifyFunction(*F);

	// Optimize the function.
	TheFPM->run(*F);

	// JIT the function, returning a function pointer.
	void *FPtr = TheExecutionEngine->getPointerToFunction(F);

	int (*FP)(int) = (int (*)(int))(intptr_t)FPtr;


	//	TheModule->dump();
	//Run function
	struct timespec t0, t1;
	clock_gettime(CLOCK_REALTIME, &t0);
	FP(11);
	//LOG(INFO) << "Mock return value of generated function " << FP(11);
	clock_gettime(CLOCK_REALTIME, &t1);
	printf("Execution took %f seconds\n",diff(t0, t1));

	TheFPM = 0;
	//Dump to see final form
	//F->dump();
}

void* RawContext::jit(Function* F) {
	// JIT the function, returning a function pointer.
	//void *FPtr = TheExecutionEngine->getPointerToFunction(F);
	return TheExecutionEngine->getPointerToFunction(F);
}

Function* const RawContext::getFunction(std::string funcName) const {
	std::map<std::string, Function*>::const_iterator it;
	it = availableFunctions.find(funcName);
	if (it == availableFunctions.end()) {
			throw runtime_error(string("Unknown function name: ") + funcName);
	}
	return it->second;
}

void RawContext::CodegenMemcpy(Value* dst, Value* src, int size) {
	LLVMContext& ctx = *llvmContext;
	// Cast src/dst to int8_t*.  If they already are, this will get optimized away
	//  DCHECK(PointerType::classof(dst->getType()));
	//  DCHECK(PointerType::classof(src->getType()));
	llvm::PointerType* ptr_type;

	Value* false_value_ = ConstantInt::get(ctx,
			APInt(1, false, true));
	Value* size_ = ConstantInt::get(ctx, APInt(32, size));
	Value* zero = ConstantInt::get(ctx, APInt(32, 0));

	dst = TheBuilder->CreateBitCast(dst, ptr_type);
	src = TheBuilder->CreateBitCast(src, ptr_type);

	// Get intrinsic function.
	Function* memcpy_fn = availableFunctions[string("memcpy")];
	if (memcpy_fn == NULL) {
		throw runtime_error(string("Could not load memcpy intrinsic"));
	}

	// The fourth argument is the alignment.  For non-zero values, the caller
	// must guarantee that the src and dst values are aligned to that byte boundary.
	// TODO: We should try to take advantage of this since our tuples are well aligned.
	Type* intType = Type::getInt32Ty(ctx);
	Value* args[] = { dst, src, size_, zero, false_value_  // is_volatile.
			};
	TheBuilder->CreateCall(memcpy_fn, args);
}

ConstantInt* RawContext::createInt32(int val) {
	LLVMContext& ctx = *llvmContext;
	return ConstantInt::get(ctx, APInt(32, val));
}

ConstantInt* RawContext::createInt64(int val) {
	LLVMContext& ctx = *llvmContext;
	return ConstantInt::get(ctx, APInt(64, val));
}

ConstantInt* RawContext::createTrue() {
	LLVMContext& ctx = *llvmContext;
	return ConstantInt::get(ctx, APInt(1, 1));
}

ConstantInt* RawContext::createFalse() {
	LLVMContext& ctx = *llvmContext;
	return ConstantInt::get(ctx, APInt(1, 0));
}

Value* RawContext::CastPtrToLlvmPtr(PointerType* type, const void* ptr) {
	LLVMContext& ctx = *llvmContext;
	Constant* const_int = ConstantInt::get(Type::getInt64Ty(ctx),(uint64_t) ptr);
	Value* llvmPtr = ConstantExpr::getIntToPtr(const_int, type);
	return llvmPtr;
}

Value* RawContext::getArrayElem(AllocaInst* mem_ptr, PointerType* type, Value* offset)	{
	LLVMContext& ctx = *llvmContext;

	Value* val_ptr = TheBuilder->CreateLoad(mem_ptr, "mem_ptr");
	Value* shiftedPtr = TheBuilder->CreateInBoundsGEP(val_ptr, offset);
	Value* val_shifted = TheBuilder->CreateLoad(shiftedPtr,"val_shifted");
	return val_shifted;
}

Value* RawContext::getStructElem(AllocaInst* mem_struct, int elemNo)	{
	vector<Value*> idxList = vector<Value*>();
	idxList.push_back(createInt32(0));
	idxList.push_back(createInt32(elemNo));
	//Shift in struct ptr
	Value* mem_struct_shifted = TheBuilder->CreateGEP(mem_struct, idxList);
	Value* val_struct_shifted =  TheBuilder->CreateLoad(mem_struct_shifted);
	return val_struct_shifted;
}

void RawContext::CreateForLoop(const string& cond, const string& body,
		const string& inc, const string& end, BasicBlock** cond_block,
		BasicBlock** body_block, BasicBlock** inc_block, BasicBlock** end_block,
		BasicBlock* insert_before) {
	Function* fn = TheFunction;
	LLVMContext& ctx = *llvmContext;
	*cond_block = BasicBlock::Create(ctx, string(cond), fn,	insert_before);
	*body_block = BasicBlock::Create(ctx, string(body), fn,	insert_before);
	*inc_block = BasicBlock::Create(ctx, string(inc), fn, insert_before);
	*end_block = BasicBlock::Create(ctx, string(end), fn, insert_before);
}

void RawContext::CreateIfElseBlocks(Function* fn, const string& if_label,
		const string& else_label, BasicBlock** if_block, BasicBlock** else_block,
		BasicBlock* insert_before) {
	LLVMContext& ctx = *llvmContext;
	*if_block = BasicBlock::Create(ctx, if_label, fn, insert_before);
	*else_block = BasicBlock::Create(ctx, else_label, fn, insert_before);
}

void RawContext::CreateIfBlock(Function* fn, const string& if_label,
		BasicBlock** if_block, BasicBlock* insert_before) {
	LLVMContext& ctx = *llvmContext;
	*if_block = BasicBlock::Create(ctx, if_label, fn, insert_before);
}

AllocaInst* RawContext::CreateEntryBlockAlloca(Function *TheFunction,
		const std::string &VarName, Type* varType) {
	IRBuilder<> TmpBuilder(&TheFunction->getEntryBlock(),
			TheFunction->getEntryBlock().begin());
	return TmpBuilder.CreateAlloca(varType, 0, VarName.c_str());
}

Value* RawContext::CreateGlobalString(char* str) {
	LLVMContext& ctx = *llvmContext;
	ArrayType* ArrayTy_0 = ArrayType::get(IntegerType::get(ctx, 8), strlen(str) + 1);

	GlobalVariable* gvar_array__str = new GlobalVariable(*TheModule,
	/*Type=*/ArrayTy_0,
	/*isConstant=*/true,
	/*Linkage=*/GlobalValue::PrivateLinkage,
	/*Initializer=*/0, // has initializer, specified below
			/*Name=*/".str");

	Constant *tmpHTname = ConstantDataArray::getString(ctx, str,true);
	PointerType* charPtrType = PointerType::get(IntegerType::get(ctx, 8), 0);
	AllocaInst* AllocaName = CreateEntryBlockAlloca(TheFunction, std::string("htName"), charPtrType);

	vector<Value*> idxList = vector<Value*>();
	idxList.push_back(createInt32(0));
	idxList.push_back(createInt32(0));
	Constant* shifted = ConstantExpr::getGetElementPtr(gvar_array__str,idxList);
	gvar_array__str->setInitializer(tmpHTname);


	LOG(INFO) << "[CreateGlobalString: ] " << str;
	TheBuilder->CreateStore(shifted, AllocaName);
	Value* globalStr = TheBuilder->CreateLoad(AllocaName);
	return globalStr;
}

PointerType* RawContext::getPointerType(Type* type) {
	return PointerType::get(type, 0);
}

StructType* RawContext::CreateCustomStruct(std::vector<Type*> innerTypes) {
	LLVMContext& ctx = *llvmContext;
	llvm::StructType* valueType = llvm::StructType::get(ctx,innerTypes);
	return valueType;
}

StructType* RawContext::CreateJSONPosStruct() {
	LLVMContext& ctx = *llvmContext;
	llvm::Type* int64_type = Type::getInt64Ty(ctx);
	std::vector<Type*> json_pos_types;
	json_pos_types.push_back(int64_type);
	json_pos_types.push_back(int64_type);
	return CreateCustomStruct(json_pos_types);
}

PointerType* RawContext::CreateJSMNStructPtr()	{
	LLVMContext& ctx = *llvmContext;
	Type* jsmnStructType = CreateJSMNStruct();
	PointerType* ptr_jsmnStructType = PointerType::get(jsmnStructType,0);
	return ptr_jsmnStructType;
}

StructType* RawContext::CreateJSMNStruct() {
	LLVMContext& ctx = *llvmContext;
	llvm::Type* int32_type = Type::getInt32Ty(ctx);
	std::vector<Type*> jsmn_pos_types;
	jsmn_pos_types.push_back(int32_type);
	jsmn_pos_types.push_back(int32_type);
	jsmn_pos_types.push_back(int32_type);
	jsmn_pos_types.push_back(int32_type);
	return CreateCustomStruct(jsmn_pos_types);
}

StructType* RawContext::CreateStringStruct() {
	LLVMContext& ctx = *llvmContext;
	llvm::Type* int32_type = Type::getInt32Ty(ctx);
	llvm::Type* char_type = Type::getInt8Ty(ctx);
	PointerType* ptr_char_type = PointerType::get(char_type,0);
	std::vector<Type*> string_obj_types;
	string_obj_types.push_back(ptr_char_type);
	string_obj_types.push_back(int32_type);

	return CreateCustomStruct(string_obj_types);
}

//Remember to add these functions as extern in .hpp too!
extern "C" double putchari(int X) {
	putchar((char) X);
	return 0;
}

void printBoolean(bool in)	{
	if(in)	{
		printf("True\n");
	}	else	{
		printf("False\n");
	}
}

/// printd - printf that takes a double prints it as "%f\n", returning 0.
int printi(int X) {
	printf("[printi:] Generated code called %d\n", X);
	return 0;
}

int printFloat(double X) {
	printf("[printFloat:] Generated code called %f\n", X);
	return 0;
}

int printi64(size_t X) {
	printf("[printi64:] Generated code called %lu\n", X);
	//cout <<"[printi64:] Generated code called "<< X<< endl;
	return 0;
}

int printc(char* X) {
	printf("[printc:] Generated code -- char read: %c\n", X[0]);
	return 0;
}


int s(const char* X) {
	//printf("Generated code -- char read: %c\n", X[0]);
	return atoi(X);
}

void insertIntKeyToHT(char* HTname, int key, void* value, int type_size) {
	RawCatalog& catalog = RawCatalog::getInstance();
	//still, one unneeded indirection..... is there a quicker way?
	multimap<int, void*>* HT = catalog.getIntHashTable(string(HTname));

	void* valMaterialized = malloc(type_size);
	memcpy(valMaterialized, value, type_size);

	HT->insert(std::pair<int, void*>(key, valMaterialized));

	//	HT->insert(std::pair<int,void*>(key,value));
	LOG(INFO) << "[Insert: ] Integer key " << key << " inserted successfully";

	LOG(INFO) << "[INSERT: ] There are " << HT->count(key)
			<< " elements with key " << key << ":";

}

void** probeIntHT(char* HTname, int key, int typeIndex) {

	string name = string(HTname);
	RawCatalog& catalog = RawCatalog::getInstance();

	//same indirection here as above.
	multimap<int, void*>* HT = catalog.getIntHashTable(name);

	pair<multimap<int, void*>::iterator, std::multimap<int, void*>::iterator> results;
	results = HT->equal_range(key);

	void** bindings = 0;
	int count = HT->count(key);
	LOG(INFO) << "[PROBE INT:] There are " << HT->count(key)
			<< " elements with key " << key;
	if (count) {
		//+1 used to set last position to null and know when to terminate
		bindings = new void*[count + 1];
		bindings[count] = NULL;
	} else {
		bindings = new void*[1];
		bindings[0] = NULL;
		return bindings;
	}

	int curr = 0;
	for (multimap<int, void*>::iterator it = results.first;
			it != results.second; ++it) {
		bindings[curr] = it->second;
		curr++;
	}
	return bindings;
}

bool equalStrings(StringObject obj1, StringObject obj2)	{
	if(obj1.len != obj2.len)	{
		return false;
	}
	if(strncmp(obj1.start,obj2.start,obj1.len) != 0)	{
		return false;
	}
	return true;
}

int compareTokenString(const char* buf, int start, int end, const char* candidate)	{
	return (strncmp(buf + start, candidate, end - start) == 0 \
			&& strlen(candidate) == end - start);
}

int compareTokenString64(const char* buf, size_t start, size_t end, const char* candidate)	{
	return (strncmp(buf + start, candidate, end - start) == 0 \
			&& strlen(candidate) == end - start);
}

bool convertBoolean(const char* buf, int start, int end)	{
	if (compareTokenString(buf, start, end, "true") == 1
			|| compareTokenString(buf, start, end, "TRUE") == 1) {
		return true;
	} else if (compareTokenString(buf, start, end, "false") == 1
			|| compareTokenString(buf, start, end, "FALSE") == 1) {
		return false;
	} else {
		string error_msg = string("[convertBoolean: Error - unknown input]");
		LOG(ERROR)<< error_msg;
		throw runtime_error(error_msg);
	}
}

bool convertBoolean64(const char* buf, size_t start, size_t end)	{
	if (compareTokenString64(buf, start, end, "true") == 1
			|| compareTokenString64(buf, start, end, "TRUE") == 1) {
		return true;
	} else if (compareTokenString64(buf, start, end, "false") == 1
			|| compareTokenString64(buf, start, end, "FALSE") == 1) {
		return false;
	} else {
		string error_msg = string("[convertBoolean64: Error - unknown input]");
		LOG(ERROR)<< error_msg;
		throw runtime_error(error_msg);
	}
}

size_t hashInt(int toHash)	{
	boost::hash<int> hasher;
	return hasher(toHash);
}

size_t hashDouble(double toHash) {
	boost::hash<double> hasher;
	return hasher(toHash);
}

//XXX Copy string? Or edit in place?
size_t hashStringC(char* toHash, size_t start, size_t end)	{
	char tmp = toHash[end+1];
	toHash[end+1] = '\0';
	boost::hash<string> hasher;
	size_t result = hasher(toHash);
	toHash[end+1] = tmp;
	return result;
}

size_t hashString(string toHash)	{
	boost::hash<string> hasher;
	size_t result = hasher(toHash);
	return result;
}

size_t hashBoolean(bool toHash) {
	boost::hash<bool> hasher;
	return hasher(toHash);
}

size_t hashStringObject(StringObject obj)	{
	char tmp = obj.start[obj.len+1];
	obj.start[obj.len+1] = '\0';
	boost::hash<string> hasher;
	size_t result = hasher(obj.start);
	obj.start[obj.len+1] = tmp;
	return result;
}


//size_t combineHashes(size_t hash1, size_t hash2) {
//	 size_t seed = 0;
//	 boost::hash_combine(seed, hash1);
//	 boost::hash_combine(seed, hash2);
//	 return seed;
//}
//
//template <class T>
//inline void hash_combine_no_order(std::size_t& seed, const T& v)
//{
//    boost::hash<T> hasher;
//    seed ^= hasher(v);
//}
//
//size_t combineHashesNoOrder(size_t hash1, size_t hash2) {
//	 size_t seed = 0;
//	 hash_combine_no_order(seed, hash1);
//	 hash_combine_no_order(seed, hash2);
//	 return seed;
//}

size_t combineHashes(size_t hash1, size_t hash2) {
	 boost::hash_combine(hash1, hash2);
	 return hash1;
}

template <class T>
inline void hash_combine_no_order(std::size_t& seed, const T& v)
{
    boost::hash<T> hasher;
    seed ^= hasher(v);
}

size_t combineHashesNoOrder(size_t hash1, size_t hash2) {
	 hash_combine_no_order(hash1, hash2);
	 return hash1;
}

//Provide support for some extern functions
void RawContext::registerFunction(const char* funcName, Function* func)	{
	availableFunctions[funcName] = func;
}

void registerFunctions(RawContext& context)	{
	LLVMContext& ctx = context.getLLVMContext();
	Module* const TheModule = context.getModule();

	llvm::Type* int1_bool_type = Type::getInt1Ty(ctx);
	llvm::Type* int8_type = Type::getInt8Ty(ctx);
	llvm::Type* int_type = Type::getInt32Ty(ctx);
	llvm::Type* int64_type = Type::getInt64Ty(ctx);
	llvm::Type* void_type = Type::getVoidTy(ctx);
	llvm::Type* double_type = Type::getDoubleTy(ctx);
	llvm::PointerType* void_ptr_type = PointerType::get(int8_type, 0);
	llvm::PointerType* char_ptr_type = PointerType::get(int8_type, 0);


	std::vector<Type*> Ints8Ptr(1,Type::getInt8PtrTy(ctx));
	std::vector<Type*> Ints8(1,int8_type);
	std::vector<Type*> Ints1(1,int1_bool_type);
	std::vector<Type*> Ints(1,int_type);
	std::vector<Type*> Ints64(1,int64_type);
	std::vector<Type*> Floats(1,double_type);

	std::vector<Type*> ArgsCmpTokens;
	ArgsCmpTokens.insert(ArgsCmpTokens.begin(),char_ptr_type);
	ArgsCmpTokens.insert(ArgsCmpTokens.begin(),int_type);
	ArgsCmpTokens.insert(ArgsCmpTokens.begin(),int_type);
	ArgsCmpTokens.insert(ArgsCmpTokens.begin(),char_ptr_type);

	std::vector<Type*> ArgsConvBoolean;
	ArgsConvBoolean.insert(ArgsConvBoolean.begin(),int_type);
	ArgsConvBoolean.insert(ArgsConvBoolean.begin(),int_type);
	ArgsConvBoolean.insert(ArgsConvBoolean.begin(),char_ptr_type);

	std::vector<Type*> ArgsConvBoolean64;
	ArgsConvBoolean64.insert(ArgsConvBoolean64.begin(),int64_type);
	ArgsConvBoolean64.insert(ArgsConvBoolean64.begin(),int64_type);
	ArgsConvBoolean64.insert(ArgsConvBoolean64.begin(),char_ptr_type);

	std::vector<Type*> ArgsAtois;
	ArgsAtois.insert(ArgsAtois.begin(),int_type);
	ArgsAtois.insert(ArgsAtois.begin(),char_ptr_type);

	std::vector<Type*> ArgsStringCmp;
	StructType* strObjType = context.CreateStringStruct();
	ArgsStringCmp.insert(ArgsStringCmp.begin(),strObjType);
	ArgsStringCmp.insert(ArgsStringCmp.begin(),strObjType);

	//Args of functions computing hash
	std::vector<Type*> ArgsHashInt;
	ArgsHashInt.insert(ArgsHashInt.begin(),int_type);

	std::vector<Type*> ArgsHashDouble;
	ArgsHashDouble.insert(ArgsHashDouble.begin(),double_type);

	std::vector<Type*> ArgsHashStringC;
	ArgsHashStringC.insert(ArgsHashStringC.begin(),int64_type);
	ArgsHashStringC.insert(ArgsHashStringC.begin(),int64_type);
	ArgsHashStringC.insert(ArgsHashStringC.begin(),char_ptr_type);

	std::vector<Type*> ArgsHashStringObj;
	ArgsHashStringObj.insert(ArgsHashStringObj.begin(),strObjType);

	std::vector<Type*> ArgsHashBoolean;
	ArgsHashBoolean.insert(ArgsHashBoolean.begin(),int1_bool_type);

	std::vector<Type*> ArgsHashCombine;
	ArgsHashCombine.insert(ArgsHashCombine.begin(),int64_type);
	ArgsHashCombine.insert(ArgsHashCombine.begin(),int64_type);


	FunctionType *FTint = FunctionType::get(Type::getInt32Ty(ctx), Ints, false);
	FunctionType *FTint64 = FunctionType::get(Type::getInt32Ty(ctx), Ints64, false);
	FunctionType *FTcharPtr = FunctionType::get(Type::getInt32Ty(ctx), Ints8Ptr, false);
	FunctionType *FTatois = FunctionType::get(int_type, ArgsAtois, false);
	FunctionType *FTatof = FunctionType::get(double_type, Ints8Ptr, false);
	FunctionType *FTprintFloat_ = FunctionType::get(int_type, Floats, false);
	FunctionType *FTcompareTokenString_ = FunctionType::get(int_type, ArgsCmpTokens, false);
	FunctionType *FTconvertBoolean_ = FunctionType::get(int1_bool_type, ArgsConvBoolean, false);
	FunctionType *FTconvertBoolean64_ = FunctionType::get(int1_bool_type, ArgsConvBoolean64, false);
	FunctionType *FTprintBoolean_ = FunctionType::get(void_type, Ints1, false);
	FunctionType *FTcompareStrings = FunctionType::get(int1_bool_type, ArgsStringCmp, false);
	FunctionType *FThashInt = FunctionType::get(int64_type, ArgsHashInt, false);
	FunctionType *FThashDouble = FunctionType::get(int64_type, ArgsHashDouble, false);
	FunctionType *FThashStringC = FunctionType::get(int64_type, ArgsHashStringC, false);
	FunctionType *FThashStringObj = FunctionType::get(int64_type, ArgsHashStringObj, false);
	FunctionType *FThashBoolean = FunctionType::get(int64_type, ArgsHashBoolean, false);
	FunctionType *FThashCombine = FunctionType::get(int64_type, ArgsHashCombine, false);


	Function *printi_ 		= Function::Create(FTint, Function::ExternalLinkage,"printi", TheModule);
	Function *printi64_ 	= Function::Create(FTint64, Function::ExternalLinkage,"printi64", TheModule);
	Function *printc_ 		= Function::Create(FTcharPtr, Function::ExternalLinkage,"printc", TheModule);
	Function *printFloat_ 	= Function::Create(FTprintFloat_, Function::ExternalLinkage, "printFloat", TheModule);
	Function *printBoolean_ = Function::Create(FTprintBoolean_, Function::ExternalLinkage, "printBoolean", TheModule);

	Function *atoi_ 	= Function::Create(FTcharPtr, Function::ExternalLinkage,"atoi", TheModule);
	Function *atois_ 	= Function::Create(FTatois, Function::ExternalLinkage,"atois", TheModule);
	atois_->addFnAttr(llvm::Attribute::AlwaysInline);
	Function *atof_ 	= Function::Create(FTatof, Function::ExternalLinkage,"atof", TheModule);

	Function *compareTokenString_	= Function::Create(FTcompareTokenString_,
			Function::ExternalLinkage, "compareTokenString", TheModule);
	compareTokenString_->addFnAttr(llvm::Attribute::AlwaysInline);
	Function *stringEquality 		= Function::Create(FTcompareStrings,
			Function::ExternalLinkage, "equalStrings", TheModule);
	stringEquality->addFnAttr(llvm::Attribute::AlwaysInline);

	Function *convertBoolean_	= Function::Create(FTconvertBoolean_,
				Function::ExternalLinkage, "convertBoolean", TheModule);
	convertBoolean_->addFnAttr(llvm::Attribute::AlwaysInline);
	Function *convertBoolean64_ = Function::Create(FTconvertBoolean64_,
					Function::ExternalLinkage, "convertBoolean64", TheModule);
	convertBoolean64_->addFnAttr(llvm::Attribute::AlwaysInline);

	Function *hashInt_ = Function::Create(FThashInt, Function::ExternalLinkage,
			"hashInt", TheModule);
	Function *hashDouble_ = Function::Create(FThashDouble,
			Function::ExternalLinkage, "hashDouble", TheModule);
	Function *hashStringC_ = Function::Create(FThashStringC,
			Function::ExternalLinkage, "hashString", TheModule);
	Function *hashStringObj_ = Function::Create(FThashStringObj,
			Function::ExternalLinkage, "hashStringObject", TheModule);
	Function *hashBoolean_ = Function::Create(FThashBoolean,
			Function::ExternalLinkage, "hashBoolean", TheModule);
	Function *hashCombine_ = Function::Create(FThashCombine,
			Function::ExternalLinkage, "combineHashes", TheModule);
	Function *hashCombineNoOrder_ = Function::Create(FThashCombine,
			Function::ExternalLinkage, "combineHashesNoOrder", TheModule);

	//Memcpy - not used yet
	Type* types[] = { void_ptr_type, void_ptr_type, Type::getInt32Ty(ctx) };
	Function* memcpy_ = Intrinsic::getDeclaration(TheModule, Intrinsic::memcpy, types);
	if (memcpy_ == NULL) {
		throw runtime_error(string("Could not find memcpy intrinsic"));
	}

	//HASHTABLES
	//Last type is needed to capture file size. Tentative
	Type* ht_int_types[] = { char_ptr_type, int_type, void_ptr_type, int_type };
	FunctionType *FTintHT = FunctionType::get(void_type, ht_int_types, false);
	Function* insertIntKeyToHT_ = Function::Create(FTintHT, Function::ExternalLinkage, "insertIntKeyToHT", TheModule);

	Type* ht_int_probe_types[] = { char_ptr_type, int_type, int_type };
	PointerType* void_ptr_ptr_type = context.getPointerType(void_ptr_type);
	FunctionType *FTint_probeHT = FunctionType::get(void_ptr_ptr_type, ht_int_probe_types, false);
	Function* probeIntHT_ = Function::Create(FTint_probeHT,	Function::ExternalLinkage, "probeIntHT", TheModule);
	probeIntHT_->addFnAttr(llvm::Attribute::AlwaysInline);

	context.registerFunction("printi", printi_);
	context.registerFunction("printi64", printi64_);
	context.registerFunction("printFloat", printFloat_);
	context.registerFunction("printBoolean", printBoolean_);
	context.registerFunction("printc", printc_);
	context.registerFunction("atoi", atoi_);
	context.registerFunction("atois", atois_);
	context.registerFunction("atof", atof_);
	context.registerFunction("memcpy", memcpy_);
	context.registerFunction("insertInt", insertIntKeyToHT_);
	context.registerFunction("probeInt", probeIntHT_);
	context.registerFunction("compareTokenString", compareTokenString_);
	context.registerFunction("convertBoolean", convertBoolean_);
	context.registerFunction("convertBoolean64", convertBoolean64_);
	context.registerFunction("equalStrings", stringEquality);
	context.registerFunction("hashInt", hashInt_);
	context.registerFunction("hashDouble", hashDouble_);
	context.registerFunction("hashStringC", hashStringC_);
	context.registerFunction("hashStringObject", hashStringObj_);
	context.registerFunction("hashBoolean", hashBoolean_);
	context.registerFunction("combineHashes", hashCombine_);
	context.registerFunction("combineHashesNoOrder", hashCombineNoOrder_);
}

inline int atoi1(const char *buf) {
	return  (buf[0] - '0');
}

inline int atoi2(const char *buf) {
	return  ((buf[0] - '0') * 10) + \
			(buf[1] - '0');
}

inline int atoi3(const char *buf) {
	return  ((buf[0] - '0') * 100) + \
			((buf[1] - '0') * 10) + \
			(buf[2] - '0');
}

inline int atoi4(const char *buf) {
	return  ((buf[0] - '0') * 1000) + \
			((buf[1] - '0') * 100) + \
			((buf[2] - '0') * 10) + \
			(buf[3] - '0');
}

inline int atoi5(const char *buf) {
	return  ((buf[0] - '0') * 10000) + \
			((buf[1] - '0') * 1000) + \
			((buf[2] - '0') * 100) + \
			((buf[3] - '0') * 10) + \
			(buf[4] - '0');
}

inline int atoi6(const char *buf) {
	return  ((buf[0] - '0') * 100000) + \
			((buf[1] - '0') * 10000) + \
			((buf[2] - '0') * 1000) + \
			((buf[3] - '0') * 100) + \
			((buf[4] - '0') * 10) + \
			(buf[5] - '0');
}

inline int atoi7(const char *buf) {
	return  ((buf[0] - '0') * 1000000) + \
			((buf[1] - '0') * 100000) + \
			((buf[2] - '0') * 10000) + \
			((buf[3] - '0') * 1000) + \
			((buf[4] - '0') * 100) + \
			((buf[5] - '0') * 10) + \
			(buf[6] - '0');
}

inline int atoi8(const char *buf) {
	return  ((buf[0] - '0') * 10000000) + \
			((buf[1] - '0') * 1000000) + \
			((buf[2] - '0') * 100000) + \
			((buf[3] - '0') * 10000) + \
			((buf[4] - '0') * 1000) + \
			((buf[5] - '0') * 100) + \
			((buf[6] - '0') * 10) + \
			(buf[7] - '0');
}

inline int atoi9(const char *buf) {
	return  ((buf[0] - '0') * 100000000) + \
			((buf[1] - '0') * 10000000) + \
			((buf[2] - '0') * 1000000) + \
			((buf[3] - '0') * 100000) + \
			((buf[4] - '0') * 10000) + \
			((buf[5] - '0') * 1000) + \
			((buf[6] - '0') * 100) + \
			((buf[7] - '0') * 10) + \
			(buf[8] - '0');
}

inline int atoi10(const char *buf) {
	return  ((buf[0] - '0') * 1000000000) + \
			((buf[1] - '0') * 100000000) + \
			((buf[2] - '0') * 10000000) + \
			((buf[3] - '0') * 1000000) + \
			((buf[4] - '0') * 100000) + \
			((buf[5] - '0') * 10000) + \
			((buf[6] - '0') * 1000) + \
			((buf[7] - '0') * 100) + \
			((buf[8] - '0') * 10) + \
			(buf[9] - '0');
}

int atois(const char *buf, int len) {
	switch (len) {
	case 1:
		return atoi1(buf);
	case 2:
		return atoi2(buf);
	case 3:
		return atoi3(buf);
	case 4:
		return atoi4(buf);
	case 5:
		return atoi5(buf);
	case 6:
		return atoi6(buf);
	case 7:
		return atoi7(buf);
	case 8:
		return atoi8(buf);
	case 9:
		return atoi9(buf);
	case 10:
		return atoi10(buf);
	default:
		LOG(ERROR) << "[ATOIS: ] Invalid Size " << len;
		throw runtime_error(string("[ATOIS: ] Invalid Size "));
	}
}
