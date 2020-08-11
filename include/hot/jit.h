#pragma once

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
#include <llvm/ExecutionEngine/Orc/LLJIT.h>

#include <llvm/Demangle/Demangle.h>

#include <stdio.h>
#include <iostream>
using namespace std;

namespace llvm {
  namespace orc {
    using JITSymbolBag = DenseMap<const char *, JITTargetAddress>;
  } // end namespace orc
} // end namespace llvm


typedef int(*mainfn)(...);

class Runnable {
private:
  // llvm

  std::unique_ptr<llvm::Module> mod;
  std::unique_ptr<llvm::orc::ThreadSafeModule> thread_safe_mod;
  std::unique_ptr<llvm::orc::LLJIT> jit;
  mainfn setup_fn, loop_fn;

  Runnable() {}

  public:


    static Runnable *create(clang::CodeGenAction *action, const llvm::orc::JITSymbolBag &symbols) {
      Runnable *run = new Runnable();
     
      // TODO:
      auto jit_builder = llvm::orc::LLJITBuilder();
      jit_builder.prepareForConstruction();
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

        run->jit->defineAbsolute(
          mangled_name_stream.str().c_str(),
          llvm::JITEvaluatedSymbol(
            it.second,
            llvm::JITSymbolFlags(llvm::JITSymbolFlags::FlagNames::Exported)
          )
        );
      }


      auto mod_ptr = action->takeModule();

      if (!mod_ptr) {
        return nullptr;
      }

      string mangledSetup = "";
      string mangledLoop = "";

      int i = 0;
      for (auto& f : *mod_ptr)
      {
          string demangled = llvm::demangle(f.getName().str());
          if (demangled.find("__cdecl setup(", 0) != string::npos) {
              mangledSetup.assign(f.getName().str());
              continue;
          }

          if (demangled.find("__cdecl loop(", 0) != string::npos) {
              mangledLoop.assign(f.getName().str());
              continue;
          }
      }


      llvm::orc::ThreadSafeModule M(
        std::move(mod_ptr),
        std::unique_ptr<llvm::LLVMContext>(action->takeLLVMContext())
      );

      run->jit->addIRModule(std::move(M));

      {
        auto result = mangledSetup.length()
        ? run->jit->lookupLinkerMangled(mangledSetup)
        : run->jit->lookup("setup");

        if (!result) {
          printf("could not find setup()\n");
          return nullptr;
        }

        run->setup_fn = (mainfn)result.get().getAddress();
      }

      {
        auto result = mangledLoop.length()
        ? run->jit->lookupLinkerMangled(mangledLoop)
        : run->jit->lookup("loop");

        if (!result) {
          printf("could not find loop()\n");
          return nullptr;
        }

        run->loop_fn = (mainfn)result.get().getAddress();
      }

      run->jit->runConstructors();
      return run;
    }

    void setup() {
      this->setup_fn();
    }

    void loop() {
      this->loop_fn();
    }
};