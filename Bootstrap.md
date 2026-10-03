Stable Bootstrap

Stable contains a Stable-written compiler track and reproducible bootstrap infrastructure.
Production compiler
The normal production flow is:
Stable source
    |
    v
C++17 Stable compiler
    |
    v
Native LLVM IR
    |
    v
Clang/LLVM
    |
    v
native executable
Native Stable-written compiler
The repository contains:
selfhost/lexer.st
selfhost/parser.st
selfhost/compiler.st
The compiler is written in Stable and contains the bootstrap frontend/compiler machinery needed for the supported self-hosting path.
Build the reference compiler:
cmake -S . -B build -DSTABLE_BUILD_TESTS=ON
cmake --build build -j2
Build the Stable-written compiler through the native LLVM backend:
./build/stablec selfhost/compiler.st --backend=llvm -o build/stage1-native-llvm
Recursive bootstrap
The authoritative native bootstrap path is:
stage 0
  |
  v
Stable-written compiler
  |
  v
stage 1
  |
  v
Stable-written compiler
  |
  v
stage 2
  |
  v
Stable-written compiler
  |
  v
stage 3
The verification scripts compare compiler outputs and probe artifacts.
The goal is a stable fixed point where repeated compilation does not change the compiler output.
Byte-identical fixed point
The native bootstrap verifier checks that the relevant stage-2 and stage-3 outputs are byte-identical.
This is stronger than merely checking that both binaries happen to run.
Legacy HIR path
Stable also contains a legacy HIR/SSA backend.
Enable it with:
cmake -S . -B build \
    -DSTABLE_ENABLE_LEGACY_HIR=ON \
    -DSTABLE_BUILD_TESTS=ON
The legacy path remains useful for:
compatibility testing
bootstrap/reference comparison
HIR inspection
optimizer development
The production backend remains native LLVM.
Verification scripts
Important scripts include:
tools/verify_selfhost.sh
tools/verify_selfhost_bootstrap.sh
tools/verify_selfhost_native_llvm.sh
tools/verify_selfhost_typed_frontend.sh
tools/verify_native_selfhost.sh
tools/verify_llvm_compat.sh
tools/verify_native_readfile_failure.sh
The exact set used depends on whether legacy HIR compatibility is enabled. 
