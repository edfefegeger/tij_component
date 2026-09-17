#requires -Version 5.1
<#
    Упаковка внешней компоненты в ZIP для загрузки в конфигурацию 1С (макет).

    Скрипт собирает найденные варианты DLL в каталог pkg, копирует MANIFEST.XML
    и INFO.XML и убирает из манифеста те записи component, файлов для которых
    нет в пакете — чтобы атрибут path всегда соответствовал содержимому архива.

    Примеры:
        powershell -File tools\pack.ps1
        powershell -File tools\pack.ps1 -MinGW64 build\mingw\TIJPrinter1C.dll
#>
[CmdletBinding()]
param(
    # Корень проекта. По умолчанию — каталог уровнем выше tools.
    [string]$Root = (Split-Path -Parent $PSScriptRoot),

    # Путь к 64-разрядной сборке MinGW (если собирали ей).
    [string]$MinGW64,

    # Путь к 32-разрядной сборке MinGW (если собирали ей).
    [string]$MinGW32
)

$ErrorActionPreference = 'Stop'

$root     = (Resolve-Path $Root).Path
$pkg      = Join-Path $root 'pkg'
$zip      = Join-Path $root 'TIJPrinter1C.zip'
$manifest = Join-Path $root 'MANIFEST.XML'
$info     = Join-Path $root 'INFO.XML'

if (-not (Test-Path $manifest)) { throw "Не найден MANIFEST.XML: $manifest" }
if (-not (Test-Path $info))     { throw "Не найден INFO.XML: $info" }

if (Test-Path $pkg) { Remove-Item $pkg -Recurse -Force }
New-Item -ItemType Directory -Path $pkg | Out-Null

$candidates = @(
    @{ Src = (Join-Path $root 'build\x64\Release\TIJPrinter1C.dll');   Name = 'TIJPrinter1C_x64.dll' },
    @{ Src = (Join-Path $root 'build\Win32\Release\TIJPrinter1C.dll'); Name = 'TIJPrinter1C_x86.dll' }
)
if ($MinGW64) { $candidates += @{ Src = $MinGW64; Name = 'TIJPrinter1C_x64.dll' } }
if ($MinGW32) { $candidates += @{ Src = $MinGW32; Name = 'TIJPrinter1C_x86.dll' } }

foreach ($c in $candidates) {
    if (Test-Path $c.Src) {
        Copy-Item -LiteralPath $c.Src -Destination (Join-Path $pkg $c.Name) -Force
        Write-Host "[+] $($c.Name)  <=  $($c.Src)"
    } else {
        Write-Host "[!] не найдено: $($c.Src)"
    }
}

# Манифест: записи component без соответствующего файла в пакет не попадают.
$lines = New-Object System.Collections.Generic.List[string]
foreach ($line in (Get-Content -LiteralPath $manifest)) {
    if ($line -match '<component\b') {
        $m = [regex]::Match($line, 'path="([^"]+)"')
        if ($m.Success -and (Test-Path (Join-Path $pkg $m.Groups[1].Value))) {
            $lines.Add($line)
        } else {
            Write-Host "[-] из манифеста исключено: $($line.Trim())"
        }
    } else {
        $lines.Add($line)
    }
}

if (-not ($lines | Where-Object { $_ -match '<component\b' })) {
    throw 'В пакете нет ни одной библиотеки — нечего упаковывать.'
}

$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllLines((Join-Path $pkg 'MANIFEST.XML'), $lines, $utf8NoBom)

# INFO.XML - сведения о компоненте: по нему БСП и БПО заполняют поля
# "Идентификатор", "Наименование" и "Версия" в карточке компоненты.
Copy-Item -LiteralPath $info -Destination (Join-Path $pkg 'INFO.XML') -Force
Write-Host '[+] INFO.XML'

if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path (Join-Path $pkg '*') -DestinationPath $zip -Force

Write-Host ''
Write-Host 'Содержимое архива:'
Get-ChildItem -LiteralPath $pkg | ForEach-Object { Write-Host ("  " + $_.Name) }
Write-Host ''
Write-Host "Готово: $zip"
