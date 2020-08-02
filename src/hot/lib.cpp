#include <hot/hot.h>

#include "llvm/Support/InitLLVM.h"

using namespace clang;
using namespace clang::driver;
using namespace llvm::opt;

//llvm::ExitOnError ExitOnErr;
void hot_init(int argc, char **argv) {
  llvm::InitLLVM initialize_llvm(argc, argv);

  llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  //ExitOnErr.setBanner("hot");
}