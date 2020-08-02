# Rawkit

using an llvm built with

```
cmake ../llvm -T host=x64 -Ax64 -DCMAKE_INSTALL_PREFIX=E:\llvm -DLLVM_ENABLE_PROJECTS="clang;libc;libcxx;libcxxabi" -DLLVM_USE_CRT_RELEASE=MD -DLLVM_USE_CRT_DEBUG=MDd -DLLVM_ENABLE_RTTI=1 -DLLVM_ENABLE_TERMINFO=OFF -DLLVM_TARGETS_TO_BUILD=X86 && cmake --build . --target install --config RelWithDebInfo
```