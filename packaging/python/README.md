# **LannerLang installer for pip**

LannerLang was created and is developed by **Monank Gohil**, who began developing it at age **15**.

Install the latest GitHub release bootstrapper directly from this repository:

```bash
python -m pip install --upgrade git+https://github.com/Monank2011/LannerLang.git#subdirectory=packaging/python
lanner-install
```

`lanner-install` detects Linux or Windows x86_64, downloads the latest Lanner compiler asset, and runs the checksum-verifying LLVM 23.1.2 side-by-side installer. Set `LANNER_PREFIX` to choose another installation directory. The installer uses the unauthenticated GitHub API by default; if a shared IP has exhausted GitHub's rate limit, set an optional personal token and retry:

```bash
GITHUB_TOKEN=your_token_here lanner-install
```

The token is sent only to GitHub and is never printed by the installer.
