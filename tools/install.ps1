$ErrorActionPreference = 'Stop'

$LannerVersion = '3.0.0'
$LLVMVersion = '23.1.2'
$LlmUrl = "https://github.com/llvm/llvm-project/releases/download/llvmorg-$LLVMVersion/clang%2Bllvm-$LLVMVersion-x86_64-pc-windows-msvc.tar.xz"
$LlvmSha256 = '8fb91cdc44fcbbdcf6b3ffd0a1f9859abd14a3c3aae4423c2b6d4a4f90bf0095'
$Prefix = if ($env:LANNER_PREFIX) { $env:LANNER_PREFIX } else { Join-Path $env:LOCALAPPDATA "Lanner\$LannerVersion" }
$Temp = Join-Path ([System.IO.Path]::GetTempPath()) ("lanner-install-" + [guid]::NewGuid())
$Archive = Join-Path $Temp 'llvm.tar.xz'
$LlvmRoot = Join-Path $Prefix "llvm\$LLVMVersion"

New-Item -ItemType Directory -Force -Path $Temp, (Join-Path $Prefix 'bin') | Out-Null
try {
    Invoke-WebRequest -Uri $LlmUrl -OutFile $Archive
    $ActualHash = (Get-FileHash -Algorithm SHA256 $Archive).Hash.ToLowerInvariant()
    if ($ActualHash -ne $LlvmSha256) {
        throw "LLVM checksum mismatch. Expected $LlvmSha256, got $ActualHash"
    }

    $Extract = Join-Path $Temp 'llvm'
    New-Item -ItemType Directory -Force -Path $Extract | Out-Null
    tar -xf $Archive -C $Extract
    $ExtractedRoot = Get-ChildItem -Directory $Extract | Select-Object -First 1
    if (-not $ExtractedRoot -or -not (Test-Path (Join-Path $ExtractedRoot.FullName 'bin\clang.exe'))) {
        throw 'Downloaded LLVM archive has an unexpected layout.'
    }
    Remove-Item -Recurse -Force $LlvmRoot -ErrorAction SilentlyContinue
    New-Item -ItemType Directory -Force -Path (Split-Path $LlvmRoot) | Out-Null
    Move-Item $ExtractedRoot.FullName $LlvmRoot

    $ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
    $Compiler = Join-Path $ScriptDir '..\bin\lanner.exe'
    if (-not (Test-Path $Compiler)) { throw 'lanner.exe was not found next to this installer.' }
    Copy-Item $Compiler (Join-Path $Prefix 'bin\lanner-bin.exe') -Force
    @"
@echo off
set "LANNER_CLANG=$LlvmRoot\bin\clang.exe"
"$Prefix\bin\lanner-bin.exe" %*
"@ | Set-Content -Encoding ASCII (Join-Path $Prefix 'bin\lanner.cmd')

    Write-Host "Lanner $LannerVersion installed."
    Write-Host "LLVM/Clang $LLVMVersion installed side by side at $LlvmRoot"
    Write-Host "Compiler wrapper: $(Join-Path $Prefix 'bin\lanner.cmd')"
    Write-Host "Add $(Join-Path $Prefix 'bin') to PATH to use lanner."
}
finally {
    Remove-Item -Recurse -Force $Temp -ErrorAction SilentlyContinue
}
