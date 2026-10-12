#requires -Version 5.1
<#
    Packaging the external component into a ZIP for loading into 1C.

    The script collects built DLLs into the "pkg" folder, copies MANIFEST.XML
    and INFO.XML there, drops manifest entries whose file is missing from the
    package, and packs everything into TIJPrinter1C.zip. Files are placed in the
    archive root, as required for loading into a configuration.

    NOTE: this file intentionally contains ASCII characters only.
    Windows PowerShell 5.1 reads a .ps1 file without a BOM using the ANSI code
    page, so non-ASCII text can break script parsing on non-English Windows.

    Usage:
        powershell -File tools\pack.ps1
        powershell -File tools\pack.ps1 -MinGW64 build\mingw\TIJPrinter1C.dll
#>
[CmdletBinding()]
param(
    # Project root. Defaults to the folder one level above tools.
    [string]$Root = (Split-Path -Parent $PSScriptRoot),

    # Path to a 64-bit MinGW build, if it was built.
    [string]$MinGW64,

    # Path to a 32-bit MinGW build, if it was built.
    [string]$MinGW32
)

$ErrorActionPreference = 'Stop'

$root     = (Resolve-Path $Root).Path
$pkg      = Join-Path $root 'pkg'
$zip      = Join-Path $root 'TIJPrinter1C.zip'
$manifest = Join-Path $root 'MANIFEST.XML'
$info     = Join-Path $root 'INFO.XML'

if (-not (Test-Path $manifest)) { throw "MANIFEST.XML not found: $manifest" }
if (-not (Test-Path $info))     { throw "INFO.XML not found: $info" }

if (Test-Path $pkg) { Remove-Item $pkg -Recurse -Force }
New-Item -ItemType Directory -Path $pkg | Out-Null

# build.cmd puts results in build\<arch>\.
# A CMake build with the Visual Studio generator adds a Release subfolder.
$candidates = @(
    @{ Src = (Join-Path $root 'build\x64\TIJPrinter1C.dll');         Name = 'TIJPrinter1C_x64.dll' },
    @{ Src = (Join-Path $root 'build\x86\TIJPrinter1C.dll');         Name = 'TIJPrinter1C_x86.dll' },
    @{ Src = (Join-Path $root 'build\x64\Release\TIJPrinter1C.dll'); Name = 'TIJPrinter1C_x64.dll' },
    @{ Src = (Join-Path $root 'build\x86\Release\TIJPrinter1C.dll'); Name = 'TIJPrinter1C_x86.dll' }
)
if ($MinGW64) { $candidates += @{ Src = $MinGW64; Name = 'TIJPrinter1C_x64.dll' } }
if ($MinGW32) { $candidates += @{ Src = $MinGW32; Name = 'TIJPrinter1C_x86.dll' } }

foreach ($c in $candidates) {
    if (Test-Path $c.Src) {
        Copy-Item -LiteralPath $c.Src -Destination (Join-Path $pkg $c.Name) -Force
        Write-Host ('[+] ' + $c.Name + '  <=  ' + $c.Src)
    } else {
        Write-Host ('[!] not found: ' + $c.Src)
    }
}

# Manifest: keep only component entries whose file is present in the package.
$lines = New-Object System.Collections.Generic.List[string]
foreach ($line in (Get-Content -LiteralPath $manifest)) {
    if ($line -match '<component') {
        $m = [regex]::Match($line, 'path="([^"]+)"')
        if ($m.Success -and (Test-Path (Join-Path $pkg $m.Groups[1].Value))) {
            $lines.Add($line)
        } else {
            Write-Host ('[-] dropped from manifest: ' + $line.Trim())
        }
    } else {
        $lines.Add($line)
    }
}

$hasComponent = $false
foreach ($l in $lines) {
    if ($l -match '<component') { $hasComponent = $true }
}
if (-not $hasComponent) { throw 'No libraries in the package, nothing to pack.' }

$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllLines((Join-Path $pkg 'MANIFEST.XML'), $lines, $utf8NoBom)

# INFO.XML describes the component: the BSP subsystem fills the "Identifier",
# "Name" and "Version" fields of the component card from this file.
Copy-Item -LiteralPath $info -Destination (Join-Path $pkg 'INFO.XML') -Force
Write-Host '[+] INFO.XML'

if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path (Join-Path $pkg '*') -DestinationPath $zip -Force

Write-Host ''
Write-Host 'Archive contents:'
Get-ChildItem -LiteralPath $pkg | ForEach-Object { Write-Host ('  ' + $_.Name) }
Write-Host ''
Write-Host ('Done: ' + $zip)
