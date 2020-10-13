#include <rawkit-jit-internal.h>

using namespace clang;
using namespace clang::driver;
using namespace llvm::opt;


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

Runnable *Runnable::compile(std::unique_ptr<CompilerInvocation> invocation, const llvm::orc::JITSymbolBag &symbols) {
  if (!invocation) {
    printf("failed to create compiler invocation\n");
    return nullptr;
  }

  CompilerInstance compiler_instance;
  compiler_instance.setInvocation(std::move(invocation));

  // Create the compilers actual diagnostics engine.
  compiler_instance.createDiagnostics();

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


  // Create and execute the frontend to generate an LLVM bitcode module.
  auto action = new EmitLLVMOnlyAction();
  if (!compiler_instance.ExecuteAction(*action)) {
    printf("failed to execute action\n");
    return nullptr;
  }

  Runnable *run = Runnable::create(action, symbols);
  run->includes = includes;
  delete action;
  return run;
}


Runnable *Runnable::create(clang::CodeGenAction *action, const llvm::orc::JITSymbolBag &symbols) {
  Runnable *run = new Runnable();
  llvm::ExitOnError EOE;
  EOE.setBanner("jit.h");
  auto jit_builder = llvm::orc::LLJITBuilder();
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
      mainJD = &session.createJITDylib("<main>");
    }

    // Resolve symbols that are statically linked in the current process.
    mainJD->addGenerator(
      cantFail(
        llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
          dataLayout.getGlobalPrefix()
        )
      )
    );

    // // Resolve symbols from shared libraries.
    // for (auto libPath : sharedLibPaths) {
    //   auto mb = llvm::MemoryBuffer::getFile(libPath);
    //   if (!mb) {
    //     printf("Fail to create MemoryBuffer for: %s\n", libPath.c_str());
    //     continue;
    //   }
    //   auto &JD = session.createJITDylib(libPath);
    //   auto loaded = llvm::orc::DynamicLibrarySearchGenerator::Load(
    //     libPath.data(),
    //     dataLayout.getGlobalPrefix()
    //   );
    //   if (!loaded) {
    //     printf("Could not load %s:\n  %s\n", libPath.c_str(), loaded.takeError().c_str());
    //     continue;
    //   }
    //   JD.addGenerator(std::move(*loaded));
    //   cantFail(objectLayer->add(JD, std::move(mb.get())));
    // }

    return objectLayer;
  };

  jit_builder.setJITTargetMachineBuilder(std::move(*JTMB));
  jit_builder.setObjectLinkingLayerCreator(objectLinkingLayerCreator);

  if (jit_builder.prepareForConstruction()) {
    printf("ERROR: failed to prepare for construction\n");
    return nullptr;
  }
  auto jit_result = jit_builder.create();

  if (!jit_result) {
    printf("could not create LLJIT\n");
    return nullptr;
  }

  run->jit.reset(std::move(jit_result.get().release()));

  for (auto &it : symbols) {
    std::string mangled_name;
    llvm::raw_string_ostream mangled_name_stream(mangled_name);

    llvm::Mangler::getNameWithPrefix(
      mangled_name_stream,
      it.first,
      run->jit->getDataLayout()
    );

    auto err = run->jit->defineAbsolute(
      mangled_name_stream.str().c_str(),
      llvm::JITEvaluatedSymbol(
        it.second,
        llvm::JITSymbolFlags(llvm::JITSymbolFlags::FlagNames::Exported)
      )
    );

    if (err) {
      printf("WARNING: could not defineAbsolute(%s)\n", it.first);
      consumeError(std::move(err));
    }
  }

  if (!mod_ptr) {
    return nullptr;
  }

  string mangledSetup = "";
  string mangledLoop = "";

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


  {
    run->setup_fn = nullptr;
    auto mangledResult = run->jit->lookupLinkerMangled(mangledSetup);
    if (mangledResult) {
      run->setup_fn = (rawkit_jit_guest_fn)mangledResult.get().getAddress();
    } else {
      auto mangledLookupResult = run->jit->lookup(mangledSetup);
      if (mangledLookupResult) {
        run->setup_fn = (rawkit_jit_guest_fn)mangledLookupResult.get().getAddress();
      } else {
        auto normalResult = run->jit->lookup("setup");
        if (normalResult) {
          run->setup_fn = (rawkit_jit_guest_fn)normalResult.get().getAddress();
        } else {
          auto fnResult = run->jit->lookup("setup()");
          if (fnResult) {
            run->setup_fn = (rawkit_jit_guest_fn)fnResult.get().getAddress();
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
      auto mangledLookupResult = run->jit->lookup(mangledLoop);
      if (mangledLookupResult) {
        run->loop_fn = (rawkit_jit_guest_fn)mangledLookupResult.get().getAddress();
      } else {

        auto normalResult = run->jit->lookup("loop");
        if (normalResult) {
          run->loop_fn = (rawkit_jit_guest_fn)normalResult.get().getAddress();
        } else {
          auto fnResult = run->jit->lookup("loop()");
          if (fnResult) {
            run->loop_fn = (rawkit_jit_guest_fn)fnResult.get().getAddress();
          }
        }
      }
    }

    if (run->loop_fn == nullptr) {
      printf("could not find loop()\n");
      return nullptr;
    }
  }

  if (run->jit->runConstructors()) {
    printf("ERROR: could not run constructors\n");
    return nullptr;
  }
  return run;
}

void Runnable::setup() {
  this->setup_fn();
}

void Runnable::loop() {
  this->loop_fn();
}
