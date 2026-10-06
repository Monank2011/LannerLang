# **Stable 1.0.0 Release Packages**

> **Stable was created and is developed by Monank Gohil, who began developing it at age 15.**

Stable 1.0.0 provides prebuilt `stablec` compiler packages for **Linux x86_64** and **Windows x86_64**. The included installer downloads the official **LLVM/Clang 23.1.2** archive, verifies its SHA-256 checksum, and installs it side by side under Stable's own directory. It does not replace or modify a system LLVM installation.

## Recommended installation

### Linux

```sh
tar -xzf stablec-1.0.0-linux-x86_64.tar.gz
cd stablec-1.0.0-linux-x86_64
./install.sh
export PATH="$HOME/.local/stable/1.0.0/bin:$PATH"
stablec examples/hello.st -o hello
./hello
```

Use `sudo ./install.sh --system` to install under `/usr/local/lib/stable/1.0.0`.

### Windows

Extract the ZIP, open PowerShell in the extracted directory, and run:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\install.ps1
$env:Path = "$env:LOCALAPPDATA\Stable\1.0.0\bin;$env:Path"
stablec.cmd .\examples\hello.st -o hello.exe
.\hello.exe
```

The Windows installer uses the official x86_64 LLVM archive and stores it at `%LOCALAPPDATA%\Stable\1.0.0\llvm\23.1.2`.

## Pinned LLVM downloads

- Linux: `LLVM-23.1.2-Linux-X64.tar.xz`
- Windows: `clang+llvm-23.1.2-x86_64-pc-windows-msvc.tar.xz`
- Official release: [LLVM 23.1.2](https://github.com/llvm/llvm-project/releases/tag/llvmorg-23.1.2)

The installers verify these SHA-256 values before extraction:

```text
Linux:   b5ed9675149cc837c282e9b6962c276c9fa62863d5b2f91537b60848552995b7
Windows: 8fb91cdc44fcbbdcf6b3ffd0a1f9859abd14a3c3aae4423c2b6d4a4f90bf0095
```

## Package contents

- `bin/stablec` or `bin/stablec.exe` — the Stable compiler
- `install.sh` / `install.ps1` — LLVM-aware installers
- `lib/stable_runtime.c` — runtime support source
- `examples/hello.st` — a minimal example
- `README.txt` — platform-specific installation and usage notes

## Convenience commands

Install the latest release bootstrapper through pip:

```bash
python -m pip install --upgrade git+https://github.com/Monank2011/STABLE.git#subdirectory=packaging/python
stable-install
```

On Linux, build the latest source with `g++`:

```bash
curl -fsSL https://raw.githubusercontent.com/Monank2011/STABLE/main/tools/install-from-source-g++.sh | bash
```

See [`docs/INSTALL.md`](../INSTALL.md) for the full installation guide.

New users can read the [**Stable 1.0.0 Handbook**](../../Stable_handbook.md) for a practical introduction to Stable syntax, types, ownership, borrowing, memory safety, runtime APIs, and the V1 feature set.

The pip bootstrapper selects the actual latest release asset name correctly and supports an optional `GITHUB_TOKEN` environment variable for GitHub API rate-limit resilience.
