# **Stable 1.0.0 Release Packages**

The v1.0.0 release provides prebuilt `stablec` compiler packages for **Linux x86_64** and **Windows x86_64**.

## Package contents

- `bin/stablec` or `bin/stablec.exe` — the Stable compiler
- `lib/stable_runtime.c` — runtime support source
- `examples/hello.st` — a minimal example
- `README.txt` — platform-specific installation and usage notes

## Linux

```sh
 tar -xzf stablec-1.0.0-linux-x86_64.tar.gz
 cd stablec-1.0.0-linux-x86_64
 ./bin/stablec examples/hello.st -o hello
 ./hello
```

The Linux package expects a compatible `clang` installation for LLVM-backed native code generation.

## Windows

Extract the ZIP, open PowerShell in the extracted directory, and run:

```powershell
.\\bin\\stablec.exe .\\examples\\hello.st -o hello.exe
.\\hello.exe
```

The Windows package is built for **x86_64 Windows**. A compatible LLVM/Clang toolchain is required for LLVM-backed native code generation.
