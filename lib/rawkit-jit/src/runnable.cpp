#include <rawkit-jit-internal.h>

#include "llvm/Support/raw_ostream.h"

using namespace clang;
using namespace clang::driver;
using namespace llvm;
using namespace llvm::opt;
using namespace llvm::orc;


#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;

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
#pragma optimize( "", off )

// Adapted from llvm/clang/lib/Frontend/TextDiagnosticBuffer.cpp
class RawkitDiagnosticBuffer :  public DiagnosticConsumer {
  public:
    RawkitDiagnosticBuffer(std::vector<DiagnosticEntry> &entries)
      : entries(entries), DiagnosticConsumer()
    {

    }

    std::vector<DiagnosticEntry> &entries;

    void HandleDiagnostic(DiagnosticsEngine::Level Level, const Diagnostic &Info) {
      // Default implementation (Warnings/errors count).
      DiagnosticConsumer::HandleDiagnostic(Level, Info);

      SmallString<100> Buf;
      Info.FormatDiagnostic(Buf);

      SourceLocation location = Info.getLocation();
      const SourceManager &source_manager = Info.getSourceManager();
      DiagnosticEntry entry = {};

      if (location.isFileID()) {
        PresumedLoc PLoc = source_manager.getPresumedLoc(location);

        if (!PLoc.isInvalid()) {
          entry.filename = string(PLoc.getFilename());
          entry.line = PLoc.getLine();
          entry.column = PLoc.getColumn();
        }
      }

      entry.level = Level;
      entry.message = std::string(Buf.str());
      this->entries.push_back(entry);

      printf("%s:%u:%u", entry.filename.c_str(), entry.line, entry.column);
      switch(Level) {
        case clang::DiagnosticsEngine::Note: printf(" note:"); break;
        case clang::DiagnosticsEngine::Warning: printf(" warning:"); break;
        case clang::DiagnosticsEngine::Remark: printf(" remark:"); break;
        case clang::DiagnosticsEngine::Error: printf(" error:"); break;
        case clang::DiagnosticsEngine::Fatal: printf(" fatal:"); break;
        default:
          break;
      }

      printf(" %s\n", entry.message.c_str());
    }

    void FlushDiagnostics(DiagnosticsEngine &Diags) const {}
};

Runnable *Runnable::compile(
  std::unique_ptr<CompilerInvocation> invocation,
  const llvm::orc::JITSymbolBag &symbols,
  vector<DiagnosticEntry> &messages
) {
  Profiler compile_timer("Runnable::compile");
  if (!invocation) {
    printf("failed to create compiler invocation\n");
    return nullptr;
  }

  CompilerInstance compiler_instance;
  compiler_instance.setInvocation(std::move(invocation));
  compiler_instance.createDiagnostics(
    new RawkitDiagnosticBuffer(messages),
    true
  );


  // Create the compilers actual diagnostics engine.
  if (!compiler_instance.hasDiagnostics()) {
    printf("could not create diagnostics engine\n");
    return nullptr;
  }

  // Infer the builtin include path if unspecified.
  // if (
  //   compiler_instance.getHeaderSearchOpts().UseBuiltinIncludes &&
  //   compiler_instance.getHeaderSearchOpts().ResourceDir.empty()
  // ) {
  //   std::string res_path = CompilerInvocation::GetResourcesPath(
  //     this->exe_arg.c_str(),
  //     this->main_addr
  //   );
  //   compiler_instance.getHeaderSearchOpts().ResourceDir = res_path;
  // }

  vector<string> includes;
  compiler_instance.addDependencyCollector(std::make_shared<IncludeCollector>(
    compiler_instance.getInvocation().getDependencyOutputOpts(),
    includes
  ));

  Profiler emit_timer("EmitLLVMOnlyAction");
  // Create and execute the frontend to generate an LLVM bitcode module.
  auto action = new EmitLLVMOnlyAction();
  if (!compiler_instance.ExecuteAction(*action)) {
    printf("failed to execute action\n");
    return nullptr;
  }
  emit_timer.end();

  Runnable *run = Runnable::create(action, symbols);
  if (!run) {
    return nullptr;
  }

  run->includes = includes;
  delete action;
  return run;
}


Runnable *Runnable::create(clang::CodeGenAction *action, const llvm::orc::JITSymbolBag &symbols) {
  Runnable *run = new Runnable();
  llvm::ExitOnError EOE;
  EOE.setBanner("jit.h");

  auto mod_ptr = action->takeModule();

  auto JTMB = llvm::orc::JITTargetMachineBuilder::detectHost();
  if (!JTMB) {
    printf("ERROR: unable to detect host\n");
    return nullptr;
  }

  bool isCoffFormat = JTMB->getTargetTriple().isOSBinFormatCOFF();

  // Callback to create the object layer with symbol resolution to current
  // process and dynamically linked libraries.
  auto objectLinkingLayerCreator = [&](
    llvm::orc::ExecutionSession &session,
    const llvm::Triple &TT
  ) {

    auto objectLayer = std::make_unique<llvm::orc::RTDyldObjectLinkingLayer>(
        session,
        []() {
          return std::make_unique<llvm::SectionMemoryManager>();
        }
    );

    objectLayer->setOverrideObjectFlagsWithResponsibilityFlags(true);

    // Fix the "unable to materialize symbol" __real@123124 and related errors.
    // see: http://llvm.org/doxygen/classllvm_1_1orc_1_1ObjectLinkingLayer.html#aa30bc825696d7254aef0fe76015d10ff
    // see: https://stackoverflow.com/a/60609825/132424
    if (isCoffFormat) {
      objectLayer->setAutoClaimResponsibilityForObjectSymbols(true);
    }

    auto dataLayout = mod_ptr->getDataLayout();
    llvm::orc::JITDylib *mainJD = session.getJITDylibByName("<main>");
    if (!mainJD) {
      mainJD = &cantFail(session.createJITDylib("<main>"));
    }

    #if defined(_WIN32)
      fs::path windows_dir(getenv("WINDIR"));

      vector<fs::path> dlls = {
        windows_dir / "system32" / "msvcrt.dll",
        windows_dir / "system32" / "msvcp140.dll",
        windows_dir / "system32" / "msvcp140_1.dll",
        windows_dir / "system32" / "msvcp140_2.dll",
        windows_dir / "system32" / "concrt140.dll",
        windows_dir / "system32" / "vcruntime140.dll",
        windows_dir / "system32" / "vcruntime140_1.dll",
        windows_dir / "system32" / "ucrtbase.dll",
      };

      for (auto dll : dlls) {
        string dll_path = dll.string();
        auto mb = llvm::MemoryBuffer::getFile(dll_path.c_str());

        if (!mb) {
          printf("Fail to create MemoryBuffer for: %s\n", dll_path.c_str());
          continue;
        }

        auto& JD = session.createBareJITDylib(dll_path);
        auto loaded = DynamicLibrarySearchGenerator::Load(dll_path.c_str(), dataLayout.getGlobalPrefix());

        if (!loaded) {
          printf("ERROR: could not load dynamic library %s\n", dll_path.c_str());
          continue;
        }

        JD.addGenerator(std::move(*loaded));
        cantFail(objectLayer->add(std::move(JD), std::move(mb.get())));
      }
    #endif

    return objectLayer;
  };

  auto jit = LLJITBuilder()
      .setJITTargetMachineBuilder(std::move(*JTMB))
      .setObjectLinkingLayerCreator(objectLinkingLayerCreator)
      .create();

  if (!jit) {
    printf("could not create LLJIT\n");
    return nullptr;
  }

  run->jit.reset(std::move(jit.get().release()));

  Profiler symbol_timer("Runnable : export symbols");

  // Add the defined symbols to this jit session
  {
    SymbolMap symbol_map;

    for (auto &it : symbols) {
      symbol_map[run->jit->mangleAndIntern(it.first)] = JITEvaluatedSymbol(
        it.second,
        JITSymbolFlags::Callable
      );
    }

    run->jit->define(absoluteSymbols(symbol_map));
  }

  symbol_timer.end();

  if (!mod_ptr) {
    return nullptr;
  }

  string mangledSetup = "";
  string mangledLoop = "";

  Profiler setup_main_timer("Runnable : search for setup/main");
  for (auto& f : *mod_ptr) {

    string mangled = f.getName().str();
    string demangled = llvm::demangle(mangled);
    // printf("  %s (%s)\n", demangled.c_str(), mangled.c_str());
    if (demangled.find("__cdecl setup(", 0) != string::npos) {
      mangledSetup.assign(mangled);
      continue;
    }

    if (demangled == "setup") {
      mangledSetup.assign(mangled);
      continue;
    }

    if (demangled == "setup()") {
      mangledSetup.assign(mangled);
      continue;
    }

    if (demangled.find("__cdecl loop(", 0) != string::npos) {
      mangledLoop.assign(mangled);
      continue;
    }

    if (demangled == "loop") {
      mangledLoop.assign(mangled);
      continue;
    }

    if (demangled == "loop()") {
      mangledLoop.assign(mangled);
      continue;
    }
  }

  llvm::orc::ThreadSafeModule M(
    std::move(mod_ptr),
    std::unique_ptr<llvm::LLVMContext>(action->takeLLVMContext())
  );

  if (run->jit->addIRModule(std::move(M))) {
    printf("ERROR: unable to add IR module\n");
    return nullptr;
  }

  // Resolve symbols that are statically linked in the current process.
  JITDylib& mainJD = run->jit->getMainJITDylib();
  mainJD.addGenerator(
    cantFail(
      llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
        run->jit->getDataLayout().getGlobalPrefix()
      )
    )
  );


  {
    run->setup_fn = nullptr;

    auto mangledResult = run->jit->lookupLinkerMangled(mangledSetup);
    if (mangledResult) {
      run->setup_fn = (rawkit_jit_guest_fn)mangledResult.get().getAddress();
    } else {
      consumeError(mangledResult.takeError());
      auto mangledLookupResult = run->jit->lookup(mangledSetup);
      if (mangledLookupResult) {
        run->setup_fn = (rawkit_jit_guest_fn)mangledLookupResult.get().getAddress();
      } else {
        consumeError(mangledLookupResult.takeError());
        auto normalResult = run->jit->lookup("setup");
        if (normalResult) {
          run->setup_fn = (rawkit_jit_guest_fn)normalResult.get().getAddress();
        } else {
          consumeError(normalResult.takeError());
          auto fnResult = run->jit->lookup("setup()");
          if (fnResult) {
            run->setup_fn = (rawkit_jit_guest_fn)fnResult.get().getAddress();
          } else {
            consumeError(fnResult.takeError());
          }
        }
      }
    }

    if (run->setup_fn == nullptr) {
      printf("could not find setup()\n");
      return nullptr;
    }
  }

  {
    auto mangledResult = run->jit->lookupLinkerMangled(mangledLoop);
    if (mangledResult) {
      run->loop_fn = (rawkit_jit_guest_fn)mangledResult.get().getAddress();
    } else {
      consumeError(mangledResult.takeError());
      auto mangledLookupResult = run->jit->lookup(mangledLoop);
      if (mangledLookupResult) {
        run->loop_fn = (rawkit_jit_guest_fn)mangledLookupResult.get().getAddress();
      } else {
        consumeError(mangledLookupResult.takeError());
        auto normalResult = run->jit->lookup("loop");
        if (normalResult) {
          run->loop_fn = (rawkit_jit_guest_fn)normalResult.get().getAddress();
        } else {
          consumeError(normalResult.takeError());
          auto fnResult = run->jit->lookup("loop()");
          if (fnResult) {
            run->loop_fn = (rawkit_jit_guest_fn)fnResult.get().getAddress();
          } else {
            consumeError(fnResult.takeError());
          }
        }
      }
    }

    if (run->loop_fn == nullptr) {
      printf("could not find loop()\n");
      return nullptr;
    }
  }
  setup_main_timer.end();

  //if (run->jit->runConstructors()) {
  //  printf("ERROR: could not run constructors\n");
  //  return nullptr;
  //}
  return run;
}

void Runnable::setup() {
  this->setup_fn();
}

void Runnable::loop() {
  this->loop_fn();
}
