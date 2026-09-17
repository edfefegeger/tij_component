#!/usr/bin/env bash
#
# Упаковка внешней компоненты в ZIP для загрузки в конфигурацию (макет)
# или в справочник "Внешние компоненты" БСП. Аналог pack.ps1 для Linux.
#
# Собирает найденные сборки в dist/, кладёт рядом MANIFEST.XML и INFO.XML,
# убирает из манифеста записи component, файлов для которых нет в пакете,
# и пакует всё в TIJPrinter1C.zip (файлы в корне архива).
#
# Использование:
#     tools/pack.sh                   # взять build/mingw-x64 и build/mingw-x86
#     tools/pack.sh <dll> [<dll>...]  # взять указанные DLL (x64, затем x86)
#
set -euo pipefail
cd "$(dirname "$0")/.."

MANIFEST="MANIFEST.XML"
INFO="INFO.XML"
OUT_ZIP="TIJPrinter1C.zip"

for f in "$MANIFEST" "$INFO"; do
    [ -f "$f" ] || { echo "Не найден $f"; exit 1; }
done

if [ "$#" -gt 0 ]; then
    SRC64="${1:-}"
    SRC86="${2:-}"
else
    SRC64="build/mingw-x64/TIJPrinter1C.dll"
    SRC86="build/mingw-x86/TIJPrinter1C.dll"
fi

SRC64="$SRC64" SRC86="$SRC86" python3 - "$MANIFEST" "$INFO" "$OUT_ZIP" <<'PY'
import os, pathlib, re, shutil, sys, zipfile

manifest, info, out_zip = sys.argv[1], sys.argv[2], sys.argv[3]
root = pathlib.Path('.')
dist = root / 'dist'

if dist.exists():
    shutil.rmtree(dist)
dist.mkdir()

candidates = [
    (os.environ.get('SRC64'), 'TIJPrinter1C_x64.dll'),
    (os.environ.get('SRC86'), 'TIJPrinter1C_x86.dll'),
]
for src, name in candidates:
    if src and pathlib.Path(src).is_file():
        shutil.copy2(src, dist / name)
        print(f'[+] dist/{name}  <=  {src}')
    elif src:
        print(f'[!] не найдено: {src}')

for extra in (manifest, info):
    shutil.copy2(root / extra, dist / extra)
    print(f'[+] dist/{extra}')

# Манифест: записи component без файла в пакете не попадают в архив.
lines = []
for line in (root / manifest).read_text(encoding='utf-8').splitlines():
    if '<component' in line:
        m = re.search(r'path="([^"]+)"', line)
        if m and (dist / m.group(1)).is_file():
            lines.append(line)
        else:
            print(f'[-] из манифеста исключено: {line.strip()}')
    else:
        lines.append(line)

if not any('<component' in l for l in lines):
    raise SystemExit('В пакете нет ни одной библиотеки — нечего упаковывать.')

(dist / manifest).write_text('\n'.join(lines) + '\n', encoding='utf-8')

out = root / out_zip
if out.exists():
    out.unlink()
with zipfile.ZipFile(out, 'w', zipfile.ZIP_DEFLATED) as z:
    for f in sorted(dist.iterdir()):
        z.write(f, f.name)

print()
print('Содержимое архива:')
for f in sorted(dist.iterdir()):
    print('  ' + f.name)
print()
print(f'Готово: {out}')
PY
