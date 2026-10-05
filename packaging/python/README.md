# **Stable installer for pip**

Stable was created and is developed by **Monank Gohil**.

Install the latest GitHub release bootstrapper directly from this repository:

```bash
python -m pip install --upgrade git+https://github.com/Monank2011/STABLE.git#subdirectory=packaging/python
stable-install
```

`stable-install` detects Linux or Windows x86_64, downloads the latest Stable compiler asset, and runs the checksum-verifying LLVM 23.1.2 side-by-side installer. Set `STABLE_PREFIX` to choose another installation directory.
