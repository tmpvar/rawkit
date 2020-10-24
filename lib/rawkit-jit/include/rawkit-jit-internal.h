#pragma once

#include <ghc/filesystem.hpp>
#pragma warning(push, 0)
  #include <clang/Basic/DiagnosticOptions.h>
  #include <clang/CodeGen/CodeGenAction.h>
  #include <clang/Driver/Compilation.h>
  #include <clang/Driver/Driver.h>
  #include <clang/Driver/Tool.h>
  #include <clang/Frontend/CompilerInstance.h>
  #include <clang/Frontend/CompilerInvocation.h>
  #include <clang/Frontend/FrontendDiagnostic.h>
  #include <clang/Frontend/TextDiagnosticPrinter.h>
  #include <llvm/ADT/SmallString.h>
  #include <llvm/ExecutionEngine/ExecutionEngine.h>
  #include <llvm/ExecutionEngine/Orc/CompileUtils.h>
  #include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>
  #include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
  #include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
  #include <llvm/ExecutionEngine/SectionMemoryManager.h>
  #include <llvm/IR/DataLayout.h>
  #include <llvm/IR/Mangler.h>
  #include <llvm/IR/Module.h>
  #include <llvm/Support/FileSystem.h>
  #include <llvm/Support/Host.h>
  #include <llvm/Support/ManagedStatic.h>
  #include <llvm/Support/Path.h>
  #include <llvm/Support/TargetSelect.h>
  #include <llvm/Support/raw_ostream.h>
  #include <llvm/Target/TargetMachine.h>
  #include "llvm/ExecutionEngine/JITLink/JITLinkMemoryManager.h"
  #include <llvm/ExecutionEngine/Orc/LLJIT.h>
  #include "llvm/ExecutionEngine/Orc/ObjectLinkingLayer.h"
  #include <llvm/Demangle/Demangle.h>

  #include <clang/AST/Mangle.h>
  #include <clang/Frontend/FrontendActions.h>
  #include <clang/Lex/Preprocessor.h>
  // #include <clang/AST/RecursiveASTVisitor.h>
  #include "llvm/Support/InitLLVM.h"

#pragma warning(pop)

#include <stdio.h>
#include <iostream>
#include <chrono>
#include <vector>
#include <string>
using namespace std;

namespace llvm {
  namespace orc {
    using JITSymbolBag = DenseMap<const char *, JITTargetAddress>;
  } // end namespace orc
} // end namespace llvm


struct Profiler {
  std::chrono::time_point<std::chrono::high_resolution_clock> start;
  bool ended = false;
  std::string name;
  Profiler(std::string name) {
    this->start = std::chrono::high_resolution_clock::now();
    this->name = name;
  }
  ~Profiler() {
    this->end();
  }

  void end() {
    if (this->ended) {
      return;
    }

    this->ended = true;
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - this->start;
    std::cout << name << ": " << diff.count() << std::endl;
  }
};

typedef int(*rawkit_jit_guest_fn)(...);

class Runnable {
  private:
    // llvm

    std::unique_ptr<llvm::Module> mod;
    std::unique_ptr<llvm::orc::ThreadSafeModule> thread_safe_mod;
    std::unique_ptr<llvm::orc::LLJIT> jit;

    Runnable() {}

  public:
    std::vector<string> includes;
    rawkit_jit_guest_fn setup_fn;
    rawkit_jit_guest_fn loop_fn;
    static Runnable *create(clang::CodeGenAction *action, const llvm::orc::JITSymbolBag &symbols);
    static Runnable *compile(
      std::unique_ptr<clang::CompilerInvocation> invocation,
      const llvm::orc::JITSymbolBag &symbols
    );
    void setup();
    void loop();
};

// This function isn't referenced outside its translation unit, but it
// can't use the "static" keyword because its address is used for
// GetMainExecutable (since some platforms don't support taking the
// address of main, and some platforms can't implement GetMainExecutable
// without being given the address of a function in the main executable).
std::string GetExecutablePath(const char *Argv0, void *MainAddr);

struct JitJobFileEntry {
  ghc::filesystem::path file;
  ghc::filesystem::file_time_type mtime;
};

class JitJob {
  private:
    JitJob();

    std::string triple_str;
    std::string path;
    std::string guest_include;
    std::string system_include;

    ghc::filesystem::path exe_path;
    void *main_addr;
    clang::driver::Driver *driver;

    std::string exe_arg;
    llvm::IntrusiveRefCntPtr<clang::DiagnosticOptions> diag_opts;
    clang::DiagnosticsEngine *diag_engine;

    std::unique_ptr<clang::driver::Compilation> compilation;
    llvm::opt::ArgStringList compilation_args;
    llvm::orc::JITSymbolBag symbols;

    ghc::filesystem::path program_source;
    ghc::filesystem::path program_dir;
    ghc::filesystem::path guest_include_dir;

    std::vector<JitJobFileEntry> watched_files;
    bool dirty;
  public:

    std::unique_ptr<Runnable> active_runnable;
    static JitJob *create(int argc, const char **argv);
    bool rebuild();
    void addExport(const char *name, void *addr);
    void addExport(const char *name, llvm::JITTargetAddress addr);

    void tick();
    void setup();
    void loop();
};