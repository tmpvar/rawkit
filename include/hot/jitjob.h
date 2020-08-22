#pragma once

#include "jit.h"

#include <ghc/filesystem.hpp>

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
    clang::TextDiagnosticPrinter *diag_printer;
    llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> diag_id;
    clang::DiagnosticsEngine *diag_engine;

    std::unique_ptr<clang::driver::Compilation> compilation;
    llvm::opt::ArgStringList compilation_args;
    std::unique_ptr<clang::CodeGenAction> compiler_action;
    std::unique_ptr<Runnable> active_runnable;
    llvm::orc::JITSymbolBag symbols;

    ghc::filesystem::path program_source;
    ghc::filesystem::path program_dir;
    ghc::filesystem::path guest_include_dir;

    std::vector<JitJobFileEntry> watched_files;
    bool dirty;
  public:

    static JitJob *create(int argc, const char **argv);
    bool rebuild();
    void addExport(const char *name, void *addr);
    void addExport(const char *name, llvm::JITTargetAddress addr);
    void tick();
    void setup();
    void loop();
};