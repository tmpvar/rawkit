#include <rawkit-jit-internal.h>

// https://kevinaboos.wordpress.com/2013/07/29/clang-tutorial-part-iii-plugin-example/
// https://gist.github.com/dhbaird/918a92405657220aed166f636e732f6d
#include <clang/AST/Mangle.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Lex/Preprocessor.h>
// #include <clang/AST/RecursiveASTVisitor.h>
#include "llvm/Support/InitLLVM.h"

#include <whereami.c>

#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;

#include <vector>

using namespace clang;
using namespace clang::driver;
using namespace llvm::opt;

std::string GetExecutablePath(const char *Argv0, void *MainAddr) {
  return llvm::sys::fs::getMainExecutable(Argv0, MainAddr);
}

static ArgStringList FilterArgs(const ArgStringList& input) {
  ArgStringList args(input);
  ArgStringList::iterator it;
  it = std::find(
    args.begin(),
    args.end(),
    "-disable-free"
  );

  if (it != args.end()) {
    args.erase(it);
  }

  it = std::find(
    args.begin(),
    args.end(),
    "-cc1as"
  );

  if (it != args.end()) {
    args.erase(it);
  }

  return args;
}

static string computeExecutablePath() {
  string ret = "";
  int dirname_length = 0;
  int length = wai_getExecutablePath(NULL, 0, &dirname_length);
  if (length <= 0) {
    return ret;
  }
  char *path = (char*)malloc(length + 1);

  if (!path) {
    return ret;
  }

  wai_getExecutablePath(path, length, &dirname_length);
  path[length] = '\0';
  ret.assign(path);
  free(path);
  return ret;
}

JitJob::JitJob() {

  this->exe_arg = computeExecutablePath();
  this->exe_path = computeExecutablePath();


  // initialize llvm
  static llvm::InitLLVM *initialize_llvm = nullptr;
  if (!initialize_llvm) {
    int argc = 1;
    vector<const char *> args = { exe_path.c_str() };

    const char **argv = (const char **)args.data();

    initialize_llvm = new llvm::InitLLVM(argc, argv);
    llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
  }

  this->system_include.assign("-I" + (this->exe_path.parent_path() / "include").string());
  this->dirty = true;
}


JitJob *JitJob::create(int argc, const char **argv) {
  JitJob *job = new JitJob();

  // paths / env and such
  job->triple_str.assign(llvm::sys::getProcessTriple());
  job->main_addr = (void*)(intptr_t)GetExecutablePath;
  job->path = GetExecutablePath(job->exe_arg.c_str(), job->main_addr);

  // job->guest_include_dir = fs::canonical(
  //   fs::path(job->path).remove_filename() / ".." / "include"
  // );

  // job->guest_include.assign(
  //   std::string("-I") + job->guest_include_dir.string()
  // );

  llvm::Triple triple(job->triple_str);

  // clang diagnostic engine
  job->diag_opts = new DiagnosticOptions();
  job->diag_printer = new TextDiagnosticPrinter(llvm::errs(), &*job->diag_opts);
  job->diag_id = new DiagnosticIDs();
  job->diag_engine = new DiagnosticsEngine(
    job->diag_id,
    job->diag_opts,
    job->diag_printer
  );

  // clang
  job->driver = new Driver(job->path, triple.str(), *job->diag_engine);
  job->driver->setTitle("hot");
  job->driver->setCheckInputsExist(false);

  SmallVector<const char *, 16> Args;

  Args.push_back(job->exe_arg.c_str());
  for (uint32_t i = 0; i < argc; i++) {
    Args.push_back(argv[i]);
  }

  if (fs::path(job->path).extension() == ".cpp") {
    Args.push_back("-x");
    Args.push_back("c++");
    Args.push_back("-std=c++17");
  }

  #if defined(_WIN32)
    Args.push_back("-fms-extensions");
    Args.push_back("-fms-compatibility");
    Args.push_back("-fdelayed-template-parsing");
    Args.push_back("-fms-compatibility-version=19.00");
    Args.push_back("-D_CRT_SECURE_NO_DEPRECATE");
    Args.push_back("-D_CRT_SECURE_NO_WARNINGS");
    Args.push_back("-D_CRT_NONSTDC_NO_DEPRECATE");
    Args.push_back("-D_CRT_NONSTDC_NO_WARNINGS");
    Args.push_back("-D_SCL_SECURE_NO_DEPRECATE");
    Args.push_back("-D_SCL_SECURE_NO_WARNINGS");
  #endif

  #ifdef __linux__
    Args.push_back("-I/home/tmpvar/llvm/lib/clang/10.0.1/include/");
  #endif

  #ifdef __APPLE__
    Args.push_back("-I/usr/local/opt/llvm/Toolchains/LLVM10.0.0.xctoolchain/usr/lib/clang/10.0.0/include");
  #endif

  Args.push_back("-fsyntax-only");
  Args.push_back("-fno-builtin");
  // TODO: this is not safe, but it is unclear how to get around missing `___stack_chk_guard` symbols on mac
  Args.push_back("-fno-stack-protector");
  Args.push_back("-fvisibility-inlines-hidden");
  Args.push_back("-fno-exceptions");
  Args.push_back("-fno-rtti");
  // Args.push_back(("-I" + fs::path(job->guest_include_dir / "rawkit" / "vulkan").string()).c_str());
  Args.push_back(job->guest_include.c_str());

  Args.push_back(job->system_include.c_str());

  /* TODO: only use these when debugging.. maybe
  Args.push_back("-I../deps/");
  Args.push_back("-I../deps/cimgui");
  Args.push_back("-I../include/hot/guest");
  Args.push_back(RAWKIT_GUEST_INCLUDE_DIR);
  Args.push_back(RAWKIT_CIMGUI_INCLUDE_DIR);
  */

  // Args.push_back("-Iinstall/include");
  Args.push_back("-DHOT_GUEST=1");
  Args.push_back("-DRAWKIT_GUEST=1");

  job->compilation.reset(job->driver->BuildCompilation(Args));
  if (!job->compilation) {
    return nullptr;
  }

  // FIXME: This is copied from ASTUnit.cpp; simplify and eliminate.

  // We expect to get back exactly one command job, if we didn't something
  // failed. Extract that job from the compilation.
  const driver::JobList &compilation_jobs = job->compilation->getJobs();
  if (compilation_jobs.size() != 1 || !isa<driver::Command>(*compilation_jobs.begin())) {
    SmallString<256> Msg;
    llvm::raw_svector_ostream OS(Msg);
    compilation_jobs.Print(OS, "; ", true);
    job->diag_engine->Report(diag::err_fe_expected_compiler_job) << OS.str();
    delete job;
    return nullptr;
  }

  const driver::Command &Cmd = cast<driver::Command>(*compilation_jobs.begin());
  if (llvm::StringRef(Cmd.getCreator().getName()) != "clang") {
    job->diag_engine->Report(diag::err_fe_expected_clang_command);
    delete job;
    return nullptr;
  }

  // Initialize a compiler invocation object from the clang (-cc1) arguments.

  job->compilation_args = FilterArgs(Cmd.getArguments());

  // program source
  fs::path file(argv[0]);
  // TODO: allow a directory to be specified as the "program" and look for common filenames
  //       sort of like node w/ index.js
  job->program_source = fs::canonical(fs::current_path() / argv[0]);
  job->program_dir = fs::path(job->program_source).remove_filename();
  cout << "running program: " << job->program_source.string() << " (" << file.filename().string() << ") " << endl;

  return job;
}


// Adapted from llvm/clang/unittests/Tooling/DependencyScannerTest.cpp
class IncludeCollector : public DependencyFileGenerator {
  vector<string> &includes;
  public:
  IncludeCollector(DependencyOutputOptions &Opts, vector<string> &includes)
    : DependencyFileGenerator(Opts)
    , includes(includes)
  {

  }

  void finishedMainFile(DiagnosticsEngine &Diags) override {
    auto new_deps = this->getDependencies();
    this->includes.insert(
      this->includes.end(),
      new_deps.begin(),
      new_deps.end()
    );
  }
};


bool JitJob::rebuild() {
  this->dirty = false;
  std::unique_ptr<CompilerInvocation> invocation(new CompilerInvocation);
  CompilerInvocation::CreateFromArgs(
    *invocation,
    this->compilation_args,
    *this->diag_engine
  );

  if (!invocation) {
    printf("failed to create compiler invocation\n");
    return false;
  }

  const driver::JobList &compilation_jobs = this->compilation->getJobs();
  if (invocation->getHeaderSearchOpts().Verbose) {
    llvm::errs() << "clang invocation:\n";
    compilation_jobs.Print(llvm::errs(), "\n", true);
    llvm::errs() << "\n";
  }

  CompilerInstance compiler_instance;
  compiler_instance.setInvocation(std::move(invocation));

  // Create the compilers actual diagnostics engine.
  compiler_instance.createDiagnostics();

  if (!compiler_instance.hasDiagnostics()) {
    printf("could not create diagnostics engine\n");
    return false;
  }

  // Infer the builtin include path if unspecified.
  if (
    compiler_instance.getHeaderSearchOpts().UseBuiltinIncludes &&
    compiler_instance.getHeaderSearchOpts().ResourceDir.empty()
  ) {
    std::string res_path = CompilerInvocation::GetResourcesPath(
      this->exe_arg.c_str(),
      this->main_addr
    );
    compiler_instance.getHeaderSearchOpts().ResourceDir = res_path;
  }

  vector<string> includes;
  compiler_instance.addDependencyCollector(std::make_shared<IncludeCollector>(
    compiler_instance.getInvocation().getDependencyOutputOpts(),
    includes
  ));


  // Create and execute the frontend to generate an LLVM bitcode module.
  auto action = new EmitLLVMOnlyAction();
  if (!compiler_instance.ExecuteAction(*action)) {
    printf("failed to execute action\n");
    return false;
  }

  this->watched_files.clear();

  {
    JitJobFileEntry entry;
    entry.file = this->program_source;
    entry.mtime = fs::last_write_time(this->program_source);
    watched_files.push_back(entry);
  }

  for (auto &include : includes) {
    auto abs_path = fs::canonical(include);
    auto rel = fs::relative(abs_path, this->guest_include_dir);

    // Filter down the results to files that exist outside of the rawkit install dir
    if (rel.string().find("..") != 0) {
      JitJobFileEntry entry;
      entry.file = abs_path;
      entry.mtime = fs::last_write_time(entry.file);
      this->watched_files.push_back(entry);
    }
  }


  // cout << "watching: " << endl;
  // for (auto &watched : this->watched_files) {
  //   cout << "  " << watched.file << endl;
  // }


  //ASTContext& ast_context = compiler_instance.getASTContext();
  //clang::MangleContext *mangle_context = ast_context.createMangleContext();
  //mangle_context->mangleCXXName()
  Runnable *run = Runnable::create(action, this->symbols);
  delete action;

  if (!run) {
    return false;
  }

  this->active_runnable.reset(run);
  return true;
}

void JitJob::addExport(const char *name, void *addr) {
  this->symbols.insert({
    std::move(name),
    llvm::pointerToJITTargetAddress(addr)
  });
}

void JitJob::addExport(const char *name, llvm::JITTargetAddress addr) {
  this->symbols.insert({
    std::move(name),
    addr
  });
}

void JitJob::tick() {
  if (!this->dirty) {
    for (auto &entry: this->watched_files) {
      try {
        fs::file_time_type mtime = fs::last_write_time(entry.file);
        if (mtime != entry.mtime) {
          this->dirty = true;
          break;
        }
      } catch (fs::filesystem_error &e) {
      }
    }
  }

  if (this->dirty && this->rebuild()) {
    this->setup();
  }

}

void JitJob::setup() {
  if (this->active_runnable) {
    this->active_runnable->setup();
  }
}

void JitJob::loop() {
  if (this->active_runnable) {
    this->active_runnable->loop();
  }
}
