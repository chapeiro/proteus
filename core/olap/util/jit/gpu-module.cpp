/*
    Proteus -- High-performance query processing on heterogeneous hardware.

                            Copyright (c) 2017
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
#include "util/jit/gpu-module.hpp"

#include <dlfcn.h>
#include <llvm/Analysis/BasicAliasAnalysis.h>
#include <llvm/CodeGen/TargetPassConfig.h>
#include <llvm/ExecutionEngine/JITEventListener.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetRegistry.h>
#pragma push_macro("NDEBUG")
#define NDEBUG
#include <llvm/Analysis/TargetTransformInfo.h>
#pragma pop_macro("NDEBUG")

#include "topology/affinity_manager.hpp"
#include "util/timing.hpp"

void initializeModule(CUmodule &cudaModule);

using namespace llvm;

#ifndef NVPTX_MAX_REGS
#define NVPTX_MAX_REGS (32)
#endif

constexpr uint64_t nvptx_max_regs = NVPTX_MAX_REGS;

LLVMTargetMachine *GpuModule::TheTargetMachine = nullptr;
legacy::PassManager GpuModule::Passes;
PassManagerBuilder GpuModule::Builder;

GpuModule::GpuModule(Context *context, std::string pipName)
    : JITModule(context, pipName) {
  uint32_t gpu_cnt = topology::getInstance().getGpuCount();
  cudaModule = (CUmodule *)malloc(gpu_cnt * sizeof(CUmodule));

  if (TheTargetMachine == nullptr) init();

  // Inform the module about the current configuration
  getModule()->setDataLayout(TheTargetMachine->createDataLayout());
  getModule()->setTargetTriple(TheTargetMachine->getTargetTriple().getTriple());

  // string ErrStr;
  // TheExecutionEngine =
  //     EngineBuilder(std::unique_ptr<Module>(getModule())).setErrorStr(&ErrStr).create();
  // if (TheExecutionEngine == nullptr) {
  //     fprintf(stderr, "Could not create ExecutionEngine: %s\n",
  //             ErrStr.c_str());
  //     exit(1);
  // }

  // // JITEventListener* vtuneProfiler =
  // JITEventListener::createIntelJITEventListener();
  // // if (vtuneProfiler == nullptr) {
  // //     fprintf(stderr, "Could not create VTune listener\n");
  // // } else {
  // //     TheExecutionEngine->RegisterJITEventListener(vtuneProfiler);
  // // }

  // JITEventListener* gdbDebugger =
  // JITEventListener::createGDBRegistrationListener(); if (gdbDebugger ==
  // nullptr) {
  //     fprintf(stderr, "Could not create GDB listener\n");
  // } else {
  //     TheExecutionEngine->RegisterJITEventListener(gdbDebugger);
  // }
}

void GpuModule::init() {
  // Get the triplet for GPU
  std::string TargetTriple("nvptx64-nvidia-cuda");

  string ErrStr;
  auto Target = TargetRegistry::lookupTarget(TargetTriple, ErrStr);

  // Print an error and exit if we couldn't find the requested target.
  // This generally occurs if we've forgotten to initialise the
  // TargetRegistry or we have a bogus target triple.
  if (!Target) {
    fprintf(stderr, "Could not create TargetTriple: %s\n", ErrStr.c_str());
    exit(1);
  }

  int dev;
  gpu_run(cudaGetDevice(&dev));
  cudaDeviceProp deviceProp;
  gpu_run(cudaGetDeviceProperties(&deviceProp, dev));
  auto GPU = "sm_" + std::to_string(deviceProp.major * 10 + deviceProp.minor);

  llvm::TargetOptions opt;
  opt.DisableIntegratedAS = 1;
  opt.MCOptions.ShowMCEncoding = 1;
  opt.MCOptions.MCUseDwarfDirectory = 1;
  // opt.MCOptions.AsmVerbose            = 1;
  opt.MCOptions.PreserveAsmComments = 1;

  std::cout << GPU << std::endl;

  auto RM = llvm::Optional<llvm::Reloc::Model>();
  TheTargetMachine = (llvm::LLVMTargetMachine *)Target->createTargetMachine(
      TargetTriple, GPU,
      "+ptx64",  // PTX 6.4 + Scoped Atomics
      //"+ptx60,+satom", //for V100
      opt, RM,
      llvm::Optional<llvm::CodeModel::Model>{},  // CodeModel::Model::Default,
      llvm::CodeGenOpt::Aggressive);

  // // Override function attributes based on CPUStr, FeaturesStr, and command
  // line
  // // flags.
  // setFunctionAttributes(CPUStr, FeaturesStr, *M);

  // TheTargetMachine->setDataLayout("e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-"
  //                        "i64:64:64-f32:32:32-f64:64:64-v16:16:16-v32:32:32-"
  //                        "v64:64:64-v128:128:128-n16:32:64");
  llvm::Triple ModuleTriple(TargetTriple);
  llvm::TargetLibraryInfoImpl TLII(ModuleTriple);

  Passes.add(new llvm::TargetLibraryInfoWrapperPass(TLII));

  // Add internal analysis passes from the target machine.
  Passes.add(createTargetTransformInfoWrapperPass(
      TheTargetMachine->getTargetIRAnalysis()));

  // FPasses.reset(new llvm::legacy::FunctionPassManager(getModule()));
  // FPasses->add(createTargetTransformInfoWrapperPass(TheTargetMachine->getTargetIRAnalysis()));

  llvm::Pass *TPC = TheTargetMachine->createPassConfig(Passes);
  Passes.add(TPC);

  // if (!NoVerify || VerifyEach)
  //   FPM.add(createVerifierPass()); // Verify that input is correct

  Builder.OptLevel = 3;
  Builder.SizeLevel = 0;

  Builder.Inliner = llvm::createFunctionInliningPass(3, 0, false);

  Builder.DisableUnrollLoops = false;
  Builder.LoopVectorize = true;

  // When #pragma vectorize is on for SLP, do the same as above
  Builder.SLPVectorize = true;

  TheTargetMachine->adjustPassManager(Builder);

  // if (Coroutines)
  //   addCoroutinePassesToExtensionPoints(Builder);

  // Builder.populateFunctionPassManager(*FPasses);
  Builder.populateModulePassManager(Passes);
}

void GpuModule::optimizeModule(llvm::Module *M) {
  time_block t("Optimization time: ");

  llvm::legacy::FunctionPassManager FPasses{M};
  FPasses.add(createTargetTransformInfoWrapperPass(
      TheTargetMachine->getTargetIRAnalysis()));

  Builder.populateFunctionPassManager(FPasses);

  FPasses.doInitialization();
  for (llvm::Function &F : *M) FPasses.run(F);
  FPasses.doFinalization();

  // Now that we have all of the passes ready, run them.
  Passes.run(*M);
}

// extern char _binary_device_funcs_cubin_end  [];
// extern char _binary_device_funcs_cubin_size   ; //size = (size_t)
// &_binary_device_funcs_cubin_size extern char
// _binary_device_funcs_cubin_start[];

extern const char *const _binary_buffer_manager_bc_start;
extern const char *const _binary_buffer_manager_bc_end;
// extern const size_t _binary_buffer_manager_bc_size;

constexpr size_t BUFFER_SIZE = 8192;
char error_log[BUFFER_SIZE];
char info_log[BUFFER_SIZE];

void GpuModule::compileAndLoad() {
#ifndef NCUDA
  LOG(INFO) << "[Prepare Function: ] Exit";  // and dump code so far";
  time_block t(pipName + " G: ");

#ifdef DEBUGCTX
  // getModule()->dump();

  if (print_generated_code) {
    std::error_code EC;
    llvm::raw_fd_ostream out(
        "generated_code/" + pipName + ".ll", EC,
        (llvm::sys::fs::OpenFlags)0);  // FIXME:
                                       // llvm::sys::fs::OpenFlags::F_NONE is
                                       // the correct one but it gives a
                                       // compilation error

    getModule()->print(out, nullptr, false, true);
  }
#endif
  llvm::SMDiagnostic error;
  auto start = (const char *)&_binary_buffer_manager_bc_start;
  auto end = (const char *)&_binary_buffer_manager_bc_end;
  size_t size = end - start;
  auto mb2 =
      llvm::MemoryBuffer::getMemBuffer(llvm::StringRef{start, size}, "", false);
  assert(mb2);

  auto mod =
      llvm::parseIR(mb2->getMemBufferRef(), error, getModule()->getContext());
  assert(mod);

  llvm::Linker::linkModules(*getModule(), std::move(mod));
  optimizeModule(getModule());

  // Dump to see final (optimized) form
#ifdef DEBUGCTX
  // getModule()->dump();

  if (print_generated_code) {
    std::error_code EC;
    llvm::raw_fd_ostream out(
        "generated_code/" + pipName + "_opt.ll", EC,
        (llvm::sys::fs::OpenFlags)0);  // FIXME:
                                       // llvm::sys::fs::OpenFlags::F_NONE is
                                       // the correct one but it gives a
                                       // compilation error

    getModule()->print(out, nullptr, false, true);
  }
#endif

  string ptx;
  {
    llvm::raw_string_ostream stream(ptx);
    llvm::buffer_ostream ostream(stream);

    llvm::legacy::PassManager PM;

    // Ask the target to add backend passes as necessary.
    TheTargetMachine->addPassesToEmitFile(
        PM, ostream,
#if LLVM_VERSION_MAJOR >= 7
        nullptr,
#endif
        llvm::TargetMachine::CGFT_AssemblyFile, false);

    PM.run(*(getModule()));
  }  // flushes stream and ostream
#ifdef DEBUGCTX
  if (print_generated_code) {
    std::ofstream optx("generated_code/" + pipName + ".ptx");
    optx << ptx;
  }
#endif

  {
    time_block t("TcuCompile: ");  // FIXME: Currently requires all GPUs to be
                                   // of the same compute capability, or to be
                                   // more precise, all of them to be compatible
                                   // with the CC of the current device
                                   //    void *cubin;
                                   //    size_t cubinSize;
                                   //
                                   //    CUlinkState linkState = nullptr;

    constexpr size_t opt_size = 8;
    CUjit_option options[opt_size];
    void *values[opt_size];

    options[0] = CU_JIT_TARGET_FROM_CUCONTEXT;
    values[0] = nullptr;
    options[1] = CU_JIT_ERROR_LOG_BUFFER;
    values[1] = (void *)error_log;
    options[2] = CU_JIT_ERROR_LOG_BUFFER_SIZE_BYTES;
    values[2] = (void *)BUFFER_SIZE;
    options[3] = CU_JIT_MAX_REGISTERS;
    values[3] = (void *)nvptx_max_regs;
    options[4] = CU_JIT_INFO_LOG_BUFFER;
    values[4] = (void *)info_log;
    options[5] = CU_JIT_INFO_LOG_BUFFER_SIZE_BYTES;
    values[5] = (void *)BUFFER_SIZE;
    options[6] = CU_JIT_LOG_VERBOSE;
    values[6] = (void *)1;
    options[7] = CU_JIT_OPTIMIZATION_LEVEL;
    values[7] = (void *)4;

    //    gpu_run(cuLinkCreate(opt_size, options, values, &linkState));
    //
    //    {
    //      auto x = cuLinkAddData(linkState, CU_JIT_INPUT_PTX, (void *)
    //      ptx.c_str(), ptx.size(), "jit",
    //                             0, nullptr, nullptr);
    //      LOG(INFO) << info_log;
    //      gpu_run(x);
    //    }
    //
    //    {
    //      auto x = cuLinkComplete(linkState, &cubin, &cubinSize);
    //      LOG(INFO) << info_log;
    //      gpu_run(x);
    //    }

    for (const auto &gpu : topology::getInstance().getGpus()) {
      time_block t("TloadModule: ");
      set_device_on_scope d(gpu);

      auto x = (cuModuleLoadDataEx(&cudaModule[gpu.id], ptx.c_str(), opt_size,
                                   options, values));

      if (info_log[0] != '\0') LOG(INFO) << info_log;
      if (x != CUDA_SUCCESS) {
        LOG(INFO) << error_log;
        gpu_run(x);
      }
      {
        time_block t("TinitModule: ");
        initializeModule(cudaModule[gpu.id]);
      }
    }

    //    gpu_run(cuLinkDestroy(linkState));
  }
  // func_name = F->getName().str();
#else
  assert(false);
#endif
}

void *GpuModule::getCompiledFunction(Function *f) const {
#ifndef NCUDA
  CUfunction func = nullptr;
  gpu_run(cuModuleGetFunction(
      &func, cudaModule[topology::getInstance().getActiveGpu().id],
      f->getName().str().c_str()));

  return (void *)func;
#else
  assert(false);
  return nullptr;
#endif
}
