# **Installing Lanner**

Lanner was created and is developed by **Monank Gohil**, who began developing it at age **15**.

## Latest release via pip

```bash
python -m pip install --upgrade git+https://github.com/Monank2011/LannerLang.git#subdirectory=packaging/python
lanner-install
```

The `lanner-install` command downloads the latest Linux or Windows x86_64 compiler release and installs the pinned LLVM/Clang 23.1.2 toolchain side by side. It does not replace a system LLVM installation. If GitHub's unauthenticated API limit is exhausted on a shared IP, retry with an optional GitHub token:

```bash
GITHUB_TOKEN=your_token_here lanner-install
```

## Latest source via g++

On Linux, this command clones the latest repository source, configures CMake to use `g++`, builds `lanner`, and installs it under `~/.local/lanner/source/bin`:

```bash
curl -fsSL https://raw.githubusercontent.com/Monank2011/LannerLang/main/tools/install-from-source-g++.sh | bash
```

For a reviewable local copy instead of piping a script:

```bash
curl -fsSLO https://raw.githubusercontent.com/Monank2011/LannerLang/main/tools/install-from-source-g++.sh
bash install-from-source-g++.sh
```

Requirements for the source path: `git`, `g++` with C++17 support, and CMake 3.20 or newer. The source-build command builds the compiler itself; LLVM 23.1.2 is installed separately by the release installer when native Lanner programs are compiled.
