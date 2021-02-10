#include <rawkit-jit-internal.h>
#include <whereami.c>

#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;

using namespace clang;
using namespace clang::driver;
using namespace llvm::opt;
#pragma optimize("", off)
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

  this->system_include.assign("-I" + (this->exe_path.parent_path().parent_path() / "include").string());
  this->clang_include.assign("-I" + (this->exe_path.parent_path().parent_path() / "include" / "clang").string());

  #ifdef CLANG_INCLUDE_DIR
    this->clang_system_include.assign("-I" + fs::absolute(fs::path(CLANG_INCLUDE_DIR)).string());
  #endif
  this->dirty = true;
}


JitJob *JitJob::create(int argc, const char **argv) {
  if (!argc || !argv || !argv[0]) {
    return nullptr;
  }

  // ensure the file exists, which should be argv[0]
  if (!fs::exists(argv[0])) {
    return nullptr;
  }

  JitJob *job = new JitJob();

  // paths / env and such
  job->triple_str.assign(llvm::sys::getProcessTriple());
  job->main_addr = (void*)(intptr_t)GetExecutablePath;
  job->path = GetExecutablePath(job->exe_arg.c_str(), job->main_addr);

  try {
    job->guest_include_dir = fs::canonical(
      fs::path(job->path).remove_filename() / ".." / "include"
    );
  }
  catch (fs::filesystem_error) {
    // Handle the case where the executable is in a subdirectory
    // example: rawkit/build/Debug/rawkit.exe
    try {
      job->guest_include_dir = fs::canonical(
        fs::path(job->path).remove_filename() / ".." / ".." / "include"
      );
    }
    catch (fs::filesystem_error) {
    }
  }

  job->guest_include.assign(
    std::string("-I") + job->guest_include_dir.string()
  );

  llvm::Triple triple(job->triple_str);

  // clang diagnostic engine
  job->diag_opts = new DiagnosticOptions();
  job->diag_engine = new DiagnosticsEngine(
    new DiagnosticIDs(),
    job->diag_opts,
    new TextDiagnosticPrinter(llvm::errs(), &*job->diag_opts)
  );

  // clang
  job->driver = new Driver(job->path, triple.str(), *job->diag_engine);
  job->driver->setTitle("rawkit-jit-job");
  job->driver->setCheckInputsExist(false);

  SmallVector<const char *, 16> Args;

  Args.push_back(job->exe_arg.c_str());
  for (int32_t i = 0; i < argc; i++) {
    Args.push_back(argv[i]);
  }

  if (fs::path(job->path).extension() == ".cpp") {
    Args.push_back("-x");
    Args.push_back("c++");
    Args.push_back("-std=c++17");
  }

  Args.push_back("-DRAWKIT_GUEST");
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
    /*
      <command line>:9:9 note: previous definition is here
      E:\c\rawkit\build\install\include\clang\xmmintrin.h:2070:9 warning: '_MM_HINT_T0' macro redefined
      C:\Program Files (x86)\Windows Kits\10\include\10.0.18362.0\um\winnt.h:3332:9 note: previous definition is here
      E:\c\rawkit\build\install\include\clang\xmmintrin.h:2072:9 warning: '_MM_HINT_T2' macro redefined
      C:\Program Files (x86)\Windows Kits\10\include\10.0.18362.0\um\winnt.h:3334:9 note: previous definition is here
    */
    // Args.push_back("-DWIN32_LEAN_AND_MEAN");
    Args.push_back("-DNOGDI");
  #endif

  #ifdef __linux__
    Args.push_back("-I/home/tmpvar/llvm/lib/clang/10.0.1/include/");

    // find the clang include path
    for (auto &p : fs::directory_iterator("/usr/lib/clang/")) {
      string strp = p.path().string();
      if (!p.is_directory()) {
        continue;
      }

      if (strp.find("11.") == string::npos) {
        continue;
      }
      printf("ADD: %s\n", strp.c_str());
      job->system_include = string("-I") + strp + "/include";
      break;
    }
  #endif

  #ifdef __APPLE__
    Args.push_back("-I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include/");
    Args.push_back("-I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks/Kernel.framework/Versions/A/Headers/");
    Args.push_back("-I/Library/Developer/CommandLineTools/usr/include/c++/v1/");
    Args.push_back("-Wno-nullability-completeness");
    Args.push_back("-Wno-expansion-to-defined");
    // TODO: this is not safe, but it is unclear how to get around missing `___stack_chk_guard` symbols on mac
    Args.push_back("-fno-stack-protector");
    // avoid error: 'TARGET_OS_IPHONE' is not defined error
    // see: https://www.gitmemory.com/issue/shirou/gopsutil/976/719870639
    Args.push_back("-Wno-undef-prefix");
  #endif

  Args.push_back("-fsyntax-only");
  Args.push_back("-fno-builtin");
  Args.push_back("-O3");
  Args.push_back("-march=native");

  /*
  Args.push_back("-fsyntax-only");
  Args.push_back("-fvisibility-inlines-hidden");
  Args.push_back("-fno-exceptions");
  Args.push_back("-fno-rtti");
  // TODO: use this and add the appropriate handlers
  Args.push_back("-fsanitize=integer-divide-by-zero");
  */

  // use clang includes first
  Args.push_back(job->clang_include.c_str());
  #ifdef CLANG_INCLUDE_DIR
    Args.push_back(job->clang_system_include.c_str());
  #endif

  Args.push_back(job->guest_include.c_str());

  #if defined(_WIN32)
    if (true || IsDebuggerPresent()) {
      // TODO: this assumes cwd is in the build directory
      Args.push_back("-Iinstall/include");
    }
  #endif

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
  Args.push_back("-D_DLL");

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
  job->program_file = job->program_source.string();
  return job;
}

bool JitJob::rebuild() {
  Profiler timeit("JitJob::rebuild");

  std::unique_ptr<CompilerInvocation> invocation(new CompilerInvocation);
  CompilerInvocation::CreateFromArgs(
    *invocation,
    this->compilation_args,
    *this->diag_engine
  );


  vector<DiagnosticEntry> local_messages;

  Runnable *run = Runnable::compile(std::move(invocation), symbols, local_messages);

  this->messages.assign(local_messages.begin(), local_messages.end());

  if (!run) {
    if (!this->active_runnable) {
      this->watched_files.clear();

      JitJobFileEntry entry;
      entry.file = this->program_source;
      entry.mtime = fs::last_write_time(this->program_source);
      watched_files.push_back(entry);
    }

    return false;
  }

  this->watched_files.clear();

  {
    JitJobFileEntry entry;
    entry.file = this->program_source;
    entry.mtime = fs::last_write_time(this->program_source);
    watched_files.push_back(entry);
  }

  for (auto &include : run->includes) {
    auto abs_path = fs::canonical(include);
    auto rel = fs::relative(abs_path, this->guest_include_dir);

    JitJobFileEntry entry;
    entry.file = abs_path;
    entry.mtime = fs::last_write_time(entry.file);
    this->watched_files.push_back(entry);
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

bool JitJob::tick() {
  if (!this->dirty) {
    for (auto &entry: this->watched_files) {
      try {
        fs::file_time_type mtime = fs::last_write_time(entry.file);
        if (mtime != entry.mtime) {
          this->dirty = true;
          entry.mtime = mtime;
          break;
        }
      } catch (fs::filesystem_error) {
      }
    }
  }

  if (!this->dirty) {
    return false;
  }

  this->dirty = false;
  return this->rebuild();
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
