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

install XCode and then the command line tools
```
xcode-select --install
```

install llvm@10
```
brew install llvm@10
```

install moltenvk sdk
```
wget https://sdk.lunarg.com/sdk/download/latest/mac/vulkan-sdk.dmg -O ~/Downloads/vulkan-sdk.dmg
hdiutil attach ~/Downloads/vulkan-sdk.dmg
mkdir ~/vulkansdk
cp -rv /Volumes/vulkansdk-macos-* ~/vulkansdk/
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

__Ubuntu 20.04 Focal__

```
# install cmake > 3.17
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main'
sudo apt purge --auto-remove cmake
sudo apt update
sudo apt install cmake ninja-build

# install clang 10
sudo apt-add-repository 'deb http://apt.llvm.org/focal/ llvm-toolchain-focal main'
sudo apt install clang-10 libclang-10-dev

# install glfw dependencies
sudo apt install libxrandr-dev libxrandr libxinerama-dev libxcursor-dev libz-dev

# ATI drivers
sudo add-apt-repository ppa:oibaf/graphics-drivers
sudo apt update
sudo apt upgrade
sudo apt install libvulkan1 mesa-vulkan-drivers vulkan-utils

# build rawkit
mkdir build
cmake .. -DCMAKE_INSTALL_PREFIX=install -DLLVM_DIR:PATH=/usr/lib/llvm-10/lib/cmake/llvm/ -DClang_DIR:PATH=/usr/lib/llvm-10/lib/cmake/clang/
cmake --build . --target install
```

__Note__: I'm currently getting a segfault on the first line of main() and the backtrace is pretty useless. I'm going to try building a debug version of llvm/clang using the `release/10.x` branch

```
git clone git://github.com/llvm/llvm-project
mkdir -p llvm-project/build
cd llvm-project/build
git checkout release/10.x
cmake ../llvm -DCMAKE_INSTALL_PREFIX=~/llvm -DLLVM_ENABLE_PROJECTS="clang;libc;libcxx;libcxxabi" -G Ninja
cmake --build . --config Debug --target install
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
