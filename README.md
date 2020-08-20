# Rawkit

using an llvm built with

```
cmake ../llvm -T host=x64 -Ax64 -DCMAKE_INSTALL_PREFIX=E:\llvm -DLLVM_ENABLE_PROJECTS="clang;libc;libcxx;libcxxabi" -DLLVM_USE_CRT_RELEASE=MD -DLLVM_USE_CRT_DEBUG=MDd -DLLVM_ENABLE_RTTI=1 -DLLVM_ENABLE_TERMINFO=OFF -DLLVM_TARGETS_TO_BUILD=X86 && cmake --build . --target install --config RelWithDebInfo
```


## Building

__windows (VS2019)__:
```
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A x64 -T host=x64 -DCMAKE_INSTALL_PREFIX=install -DLLVM_DIR:PATH=E:/llvm/lib/cmake/llvm -DClang_DIR:PATH=E:/llvm/lib/cmake/clang
cmake --build . --config RelWithDebInfo --target install
```

## running

__windows__

```
cd build
watchexec -i projects -r -w .. "cmake --build . --config RelWithDebInfo --target install && install\bin\rawkit.exe ../projects/cncwiz/cncwiz.cpp"
```

## run tests

__windows__
```
cd build

watchexec -r -w .. "cmake --build . --config RelWithDebInfo && RelWithDebInfo\rawkit-test.exe"
```