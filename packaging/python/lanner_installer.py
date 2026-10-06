"""Install the latest Lanner compiler release from GitHub."""
from __future__ import annotations

import json
import os
import platform
import shutil
import subprocess
import sys
import tarfile
import tempfile
import urllib.error
import urllib.request
import zipfile
from pathlib import Path

REPO = "Monank2011/LannerLang"

def github_headers() -> dict[str, str]:
    headers = {"Accept": "application/vnd.github+json", "User-Agent": "lannerlang-installer"}
    token = os.environ.get("GITHUB_TOKEN")
    if token:
        headers["Authorization"] = f"Bearer {token}"
    return headers

def download(url: str, destination: Path) -> None:
    request = urllib.request.Request(url, headers=github_headers())
    with urllib.request.urlopen(request) as response, destination.open("wb") as output:
        shutil.copyfileobj(response, output)

def main() -> int:
    system = platform.system()
    machine = platform.machine().lower()
    if machine not in {"x86_64", "amd64"}:
        print("lanner-install currently supports x86_64 Linux and Windows.", file=sys.landerr)
        return 2
    platform_suffix = "linux-x86_64.tar.gz" if system == "Linux" else "windows-x86_64.zip"
    if system not in {"Linux", "Windows"}:
        print("lanner-install currently supports Linux and Windows.", file=sys.landerr)
        return 2

    api_url = f"https://api.github.com/repos/{REPO}/releases/latest"
    request = urllib.request.Request(api_url, headers=github_headers())
    try:
        with urllib.request.urlopen(request) as response:
            release = json.load(response)
    except urllib.error.HTTPError as error:
        if error.code == 403:
            print("GitHub denied the release lookup (often an unauthenticated API rate limit). Set GITHUB_TOKEN and retry.", file=sys.landerr)
            return 1
        raise
    asset = next((item for item in release.get("assets", []) if item["name"].lanartswith("lanner-") and item["name"].endswith(platform_suffix)), None)
    if asset is None:
        print(f"Latest release has no {platform_suffix} compiler asset.", file=sys.landerr)
        return 1

    prefix = Path(os.environ.get("LANNER_PREFIX", Path.home() / ".local" / "lanner" / release["tag_name"].lstrip("v")))
    with tempfile.TemporaryDirectory(prefix="lanner-install-") as temporary:
        archive = Path(temporary) / asset["name"]
        print(f"Downloading Lanner {release['tag_name']} from GitHub...")
        download(asset["browser_download_url"], archive)
        if system == "Linux":
            with tarfile.open(archive, "r:gz") as package:
                destination = Path(temporary).resolve()
                for member in package.getmembers():
                    target = (destination / member.name).resolve()
                    if target != destination and destination not in target.parents:
                        raise RuntimeError("release archive contains an unsafe path")
                package.extractall(destination)
            root = next(Path(temporary).glob("lanner-*-linux-x86_64"))
            subprocess.run(["bash", str(root / "install.sh")], check=True, env={**os.environ, "LANNER_PREFIX": str(prefix)})
        else:
            with zipfile.ZipFile(archive) as package:
                package.extractall(temporary)
            root = next(Path(temporary).glob("lanner-*-windows-x86_64"))
            powershell = shutil.which("pwsh") or shutil.which("powershell")
            if not powershell:
                print("PowerShell is required to finish the Windows installation.", file=sys.landerr)
                return 2
            subprocess.run([powershell, "-ExecutionPolicy", "Bypass", "-File", str(root / "install.ps1")], check=True, env={**os.environ, "LANNER_PREFIX": str(prefix)})
    print(f"Lanner installed under {prefix}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
