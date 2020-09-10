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

__MacOs__

install llvm@10
```
brew install llvm@10
```

install moltenvk sdk
```
wget https://sdk.lunarg.com/sdk/download/latest/mac/vulkan-sdk.dmg -O ~/Downloads/vulkan-sdk.dmg
hdiutil attach ~/Downloads/vulkan-sdk.dmg
mkdir ~/moltenvk
cp -rv /Volumes/vulkansdk-macos-* ~/moltenvk/
```

add moltenvk setup to `~/.bash_profile` or similar
```
# vulkan sdk
. ~/vulkansdk/setup-env.sh
```

update the current session:
```
. ~/.bash_profile
```

```
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=install -DLLVM_DIR=/usr/local/opt/llvm/Toolchains/LLVM10.0.0.xctoolchain/usr/lib/cmake/llvm/ -DClang_DIR=/usr/local/opt/llvm/Toolchains/LLVM10.0.0.xctoolchain/usr/lib/cmake/clang/
```
## running

__windows__

```
cd build
watchexec -i projects -i lib -r -w .. "cmake --build . --config RelWithDebInfo --target install && install\bin\rawkit.exe ../projects/cncwiz/cncwiz.cpp"
```

## run tests

__windows__
```
cd build

watchexec -r -w .. "cmake --build . --config RelWithDebInfo && lib\RelWithDebInfo\test-rawkit-libs.exe"
```