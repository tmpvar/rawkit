#include <hot/jitjob.h>

// https://kevinaboos.wordpress.com/2013/07/29/clang-tutorial-part-iii-plugin-example/ 
// https://gist.github.com/dhbaird/918a92405657220aed166f636e732f6d
#include <clang/AST/Mangle.h>

#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;
// #include <filesystem>
// namespace fs = std::filesystem;
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


JitJob *JitJob::create(int argc, const char **argv) {
  JitJob *job = new JitJob();
  job->exe_arg.assign(argv[0]);
  // paths / env and such
  job->triple_str.assign(llvm::sys::getProcessTriple());
  job->main_addr = (void*)(intptr_t)GetExecutablePath;
  job->path = GetExecutablePath(job->exe_arg.c_str(), job->main_addr);
  job->guest_include.assign(
    std::string("-I") +
    (fs::path(job->path).remove_filename() / "include").string()
  );

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

  SmallVector<const char *, 16> Args(argv, argv + argc);

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
  #endif

  Args.push_back("-fsyntax-only");
  Args.push_back(job->guest_include.c_str());
  Args.push_back("-I/usr/local/opt/llvm/Toolchains/LLVM10.0.0.xctoolchain/usr/lib/clang/10.0.0/include");

  Args.push_back("-I../deps/");
  Args.push_back("-I../deps/cimgui");
  Args.push_back("-I../include/hot/guest");
  Args.push_back(RAWKIT_GUEST_INCLUDE_DIR);
  Args.push_back(RAWKIT_CIMGUI_INCLUDE_DIR);
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
  fs::path file(argv[1]);
  job->program_source = fs::current_path() / file;

  return job;
}

bool JitJob::rebuild() {
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

  // Create and execute the frontend to generate an LLVM bitcode module.
  auto action = new EmitLLVMOnlyAction();
  if (!compiler_instance.ExecuteAction(*action)) {
    printf("failed to execute action\n");
    return false;
  }

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
  try {
    fs::file_time_type mtime = fs::last_write_time(this->program_source);
    if (mtime != this->program_source_mtime) {
      this->program_source_mtime = mtime;
      if (this->rebuild()) {
        this->setup();
      }
    }
  } catch (fs::filesystem_error e) {
    // Handle the case where the file has been removed temporarily while saving
    return;
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



