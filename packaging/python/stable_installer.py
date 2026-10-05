"""Install the latest Stable compiler release from GitHub."""
from __future__ import annotations

import json
import os
import platform
import shutil
import subprocess
import sys
import tarfile
import tempfile
import urllib.request
import zipfile
from pathlib import Path

REPO = "Monank2011/STABLE"

def download(url: str, destination: Path) -> None:
    request = urllib.request.Request(url, headers={"User-Agent": "stable-lang-installer"})
    with urllib.request.urlopen(request) as response, destination.open("wb") as output:
        shutil.copyfileobj(response, output)

def main() -> int:
    system = platform.system()
    machine = platform.machine().lower()
    if machine not in {"x86_64", "amd64"}:
        print("stable-install currently supports x86_64 Linux and Windows.", file=sys.stderr)
        return 2
    platform_suffix = "linux-x86_64.tar.gz" if system == "Linux" else "windows-x86_64.zip"
    if system in {"Linux", "Windows"}:
        asset_name = None
    else:
        print("stable-install currently supports Linux and Windows.", file=sys.stderr)
        return 2

    api_url = f"https://api.github.com/repos/{REPO}/releases/latest"
    request = urllib.request.Request(api_url, headers={"Accept": "application/vnd.github+json", "User-Agent": "stable-lang-installer"})
    with urllib.request.urlopen(request) as response:
        release = json.load(response)
    asset = next((item for item in release.get("assets", []) if item["name"].startswith("stablec-") and item["name"].endswith(platform_suffix)), None)
    if asset is None:
        print(f"Latest release has no {platform_suffix} compiler asset.", file=sys.stderr)
        return 1

    prefix = Path(os.environ.get("STABLE_PREFIX", Path.home() / ".local" / "stable" / release["tag_name"].lstrip("v")))
    with tempfile.TemporaryDirectory(prefix="stable-install-") as temporary:
        archive = Path(temporary) / asset_name
        print(f"Downloading Stable {release['tag_name']} from GitHub...")
        download(asset["browser_download_url"], archive)
        if system == "Linux":
            with tarfile.open(archive, "r:gz") as package:
                destination = Path(temporary).resolve()
                for member in package.getmembers():
                    target = (destination / member.name).resolve()
                    if target != destination and destination not in target.parents:
                        raise RuntimeError("release archive contains an unsafe path")
                package.extractall(destination)
            root = next(Path(temporary).glob("stablec-*-linux-x86_64"))
            subprocess.run(["bash", str(root / "install.sh")], check=True, env={**os.environ, "STABLE_PREFIX": str(prefix)})
        else:
            with zipfile.ZipFile(archive) as package:
                package.extractall(temporary)
            root = next(Path(temporary).glob("stablec-*-windows-x86_64"))
            powershell = shutil.which("pwsh") or shutil.which("powershell")
            if not powershell:
                print("PowerShell is required to finish the Windows installation.", file=sys.stderr)
                return 2
            subprocess.run([powershell, "-ExecutionPolicy", "Bypass", "-File", str(root / "install.ps1")], check=True, env={**os.environ, "STABLE_PREFIX": str(prefix)})
    print(f"Stable installed under {prefix}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
