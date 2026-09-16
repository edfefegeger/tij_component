#!/usr/bin/env python3
"""
Проверка внешней компоненты 1С (Native API) для Windows без Windows.

Читает PE-файл и проверяет именно то, из-за чего ПодключитьВнешнююКомпоненту()
возвращает Ложь:

  * разрядность (x86 / x64 / ARM64) и её соответствие ожидаемой;
  * обязательные экспортируемые функции и неискажённые имена;
  * зависимости: несистемные DLL (MSVCP140, VCRUNTIME140_1, libstdc++-6, ...);
  * наличие встроенного манифеста и SxS-зависимостей CRT.

Зависимостей нет — только стандартная библиотека Python 3.

Использование:
    python3 tools/check_pe.py build/mingw-x64/TIJPrinter1C.dll
    python3 tools/check_pe.py TIJPrinter1C.dll --expect x86
    python3 tools/check_pe.py каталог_или_*.dll --quiet

Код возврата: 0 — все проверки пройдены, 1 — есть проблемы.
"""

import argparse
import glob
import os
import re
import struct
import sys

MACHINES = {
    0x014C: ("x86", 32),
    0x8664: ("x64", 64),
    0x01C0: ("ARM", 32),
    0xAA64: ("ARM64", 64),
}

# Обязательные экспорты Native-компоненты 1С.
REQUIRED_EXPORTS = ["GetClassNames", "GetClassObject", "DestroyObject"]

# Необязательные: без них компонента работает, но теряет часть возможностей.
OPTIONAL_EXPORTS = ["SetPlatformCapabilities", "GetAttachType"]

# Зависимости, которых быть не должно: на машине пользователя их может не
# оказаться или они могут быть другой версии.
PROBLEM_DLLS = [
    (re.compile(r"^(msvcp\d+(d)?|msvcr\d+(d)?|vcruntime\d+(_\d+)?(d)?|concrt\d+(d)?)\.dll$", re.I),
     "рантайм Visual C++ — нужен установленный Redistributable"),
    (re.compile(r"^(ucrtbase(d)?|api-ms-win-crt-.+)\.dll$", re.I),
     "Universal CRT — при статическом CRT импортироваться не должен"),
    (re.compile(r"^(libstdc\+\+|libgcc|libwinpthread).*\.dll$", re.I),
     "рантайм MinGW — на чистой Windows отсутствует"),
    (re.compile(r"^ucrtbased\.dll$", re.I),
     "ОТЛАДОЧНЫЙ рантайм — есть только там, где установлена Visual Studio"),
]


class PeError(Exception):
    pass


def _rva_to_offset(sections, rva):
    for _, vaddr, vsize, rawaddr, rawsize in sections:
        span = max(vsize, rawsize)
        if vaddr <= rva < vaddr + span:
            return rawaddr + (rva - vaddr)
    return None


def _cstring(data, offset, limit=260):
    if offset is None or offset >= len(data):
        return None
    end = data.find(b"\0", offset, offset + limit)
    if end < 0:
        end = min(len(data), offset + limit)
    return data[offset:end].decode("utf-8", "replace")


def parse_pe(path):
    with open(path, "rb") as fh:
        data = fh.read()

    if len(data) < 0x40:
        raise PeError("файл слишком мал для PE (%d байт)" % len(data))
    if data[:2] != b"MZ":
        raise PeError("не PE-файл (нет сигнатуры MZ)")

    e_lfanew = struct.unpack_from("<I", data, 0x3C)[0]
    if data[e_lfanew:e_lfanew + 4] != b"PE\0\0":
        raise PeError("не PE-файл (нет сигнатуры PE)")

    coff = e_lfanew + 4
    machine, n_sections, _, _, _, opt_size, _ = struct.unpack_from("<HHIIIHH", data, coff)

    opt = coff + 20
    magic = struct.unpack_from("<H", data, opt)[0]
    is_pe32_plus = magic == 0x20B
    if magic not in (0x10B, 0x20B):
        raise PeError("неизвестный формат опционального заголовка")

    dd_offset = opt + (112 if is_pe32_plus else 96)
    n_dd = struct.unpack_from("<I", data, opt + (108 if is_pe32_plus else 92))[0]

    sections = []
    for i in range(n_sections):
        o = opt + opt_size + i * 40
        name = data[o:o + 8].rstrip(b"\0").decode("ascii", "replace")
        vsize, vaddr, rawsize, rawaddr = struct.unpack_from("<IIII", data, o + 8)
        sections.append((name, vaddr, vsize, rawaddr, rawsize))

    def directory(index):
        if index >= n_dd:
            return (0, 0)
        return struct.unpack_from("<II", data, dd_offset + index * 8)

    # --- экспорт ---
    exports = []
    export_dll_name = None
    exp_rva, exp_size = directory(0)
    if exp_rva and exp_size:
        eo = _rva_to_offset(sections, exp_rva)
        if eo is not None:
            (_, _, _, name_rva, _, n_func, n_names, _, names_rva, _) = \
                struct.unpack_from("<IIIIIIIIII", data, eo)
            export_dll_name = _cstring(data, _rva_to_offset(sections, name_rva))
            names_off = _rva_to_offset(sections, names_rva)
            for i in range(n_names):
                if names_off is None:
                    break
                nr = struct.unpack_from("<I", data, names_off + i * 4)[0]
                nm = _cstring(data, _rva_to_offset(sections, nr), 160)
                if nm:
                    exports.append(nm)

    # --- импорт ---
    imports = []
    imp_rva, imp_size = directory(1)
    if imp_rva and imp_size:
        off = _rva_to_offset(sections, imp_rva)
        ptr_size = 8 if is_pe32_plus else 4
        ordinal_flag = 1 << (63 if is_pe32_plus else 31)
        fmt = "<Q" if is_pe32_plus else "<I"
        i = 0
        while off is not None:
            entry = off + i * 20
            if entry + 20 > len(data):
                break
            oft, _, _, name_rva, first_thunk = struct.unpack_from("<IIIII", data, entry)
            if name_rva == 0 and first_thunk == 0:
                break
            dll_name = _cstring(data, _rva_to_offset(sections, name_rva)) or "<без имени>"
            funcs = []
            thunk_rva = oft or first_thunk
            to = _rva_to_offset(sections, thunk_rva)
            if to is not None:
                j = 0
                while to + (j + 1) * ptr_size <= len(data):
                    val = struct.unpack_from(fmt, data, to + j * ptr_size)[0]
                    if val == 0:
                        break
                    if not (val & ordinal_flag):
                        # IMAGE_IMPORT_BY_NAME = WORD Hint + CHAR Name[], поэтому +2:
                        # без этого имя читается со сдвигом и портится.
                        by_name_rva = (val & (ordinal_flag - 1)) + 2
                        fn = _cstring(data, _rva_to_offset(sections, by_name_rva), 160)
                        funcs.append(fn or "<не читается>")
                    else:
                        funcs.append("#%d" % (val & 0xFFFF))
                    j += 1
                    if j > 4096:
                        break
            imports.append((dll_name, funcs))
            i += 1
            if i > 256:
                break

    # Манифест ищется только в секции ресурсов: поиск по всему файлу давал бы
    # ложные срабатывания на строке "<assembly" в обычных данных.
    rsrc = b""
    for s_name, s_vaddr, s_vsize, s_rawaddr, s_rawsize in sections:
        if s_name == ".rsrc" and s_rawsize:
            rsrc = data[s_rawaddr:s_rawaddr + s_rawsize]
            break

    has_manifest = b"<assembly" in rsrc
    manifest_has_sxs = bool(re.search(rb"dependentAssembly|Microsoft\.VC\d+", rsrc))

    return {
        "file": path,
        "machine": machine,
        "is_pe32_plus": is_pe32_plus,
        "dll": bool(struct.unpack_from("<H", data, coff + 18)[0] & 0x2000),
        "export_dll_name": export_dll_name,
        "exports": exports,
        "imports": imports,
        "has_manifest": has_manifest,
        "manifest_has_sxs": manifest_has_sxs,
    }


def check(path, expect, quiet):
    problems = []
    notes = []

    try:
        pe = parse_pe(path)
    except PeError as exc:
        print("FAIL  %s: %s" % (path, exc))
        return ["не PE-файл"]

    arch, bits = MACHINES.get(pe["machine"], ("неизвестная (%#x)" % pe["machine"], 0))

    if not quiet:
        print("=" * 70)
        print("Файл: %s" % path)
        print("=" * 70)
        print("Разрядность : %s (%d бит, %s)" % (arch, bits, "PE32+" if pe["is_pe32_plus"] else "PE32"))
        print("Тип         : %s" % ("DLL" if pe["dll"] else "не DLL"))
        print("Имя в экспорте: %s" % pe["export_dll_name"])

    # 1. разрядность
    if expect and expect != "any":
        if arch == expect:
            if not quiet:
                print("  OK   разрядность соответствует ожидаемой (%s)" % expect)
        else:
            problems.append("разрядность %s, а ожидалась %s" % (arch, expect))

    # 2. экспорты
    if not quiet:
        print("\nЭкспортируемые функции (%d):" % len(pe["exports"]))
        for name in pe["exports"]:
            print("    %s" % name)

    for name in REQUIRED_EXPORTS:
        if name in pe["exports"]:
            if not quiet:
                print("  OK   обязательный экспорт %s" % name)
        else:
            problems.append("нет обязательного экспорта %s" % name)

    for name in OPTIONAL_EXPORTS:
        if name not in pe["exports"]:
            notes.append("нет необязательного экспорта %s" % name)

    decorated = [n for n in pe["exports"]
                 if n.startswith("_") or re.search(r"@\d+$", n) or n.startswith("?")]
    if decorated:
        problems.append("имена экспортов искажены: %s" % ", ".join(decorated))

    if not pe["exports"]:
        problems.append("таблица экспорта пуста")

    # 3. зависимости
    if not quiet:
        print("\nИмпортируемые библиотеки (%d):" % len(pe["imports"]))
        for dll_name, funcs in pe["imports"]:
            print("    %-32s (%d функций)" % (dll_name, len(funcs)))

    for dll_name, _ in pe["imports"]:
        for pattern, why in PROBLEM_DLLS:
            if pattern.match(dll_name):
                problems.append("зависимость %s — %s" % (dll_name, why))
                break

    # 4. манифест
    if pe["has_manifest"]:
        if not quiet:
            print("\n  OK   встроенный манифест найден")
        if pe["manifest_has_sxs"]:
            notes.append("в манифесте есть SxS-зависимости (dependentAssembly/Microsoft.VC) — "
                         "проверьте, что они разрешаются на машине пользователя")
    else:
        notes.append("встроенный манифест не найден (для Native-компоненты не критично)")

    print()
    for name in notes:
        print("  [!]  %s" % name)
    if problems:
        for item in problems:
            print("  FAIL %s" % item)
        print("\nИТОГ: %s — есть проблемы (%d)" % (os.path.basename(path), len(problems)))
    else:
        print("ИТОГ: %s — все проверки пройдены" % os.path.basename(path))

    return problems


def main():
    parser = argparse.ArgumentParser(description="Проверка PE-файла внешней компоненты 1С")
    parser.add_argument("paths", nargs="+", help="DLL, каталог или маска")
    parser.add_argument("--expect", choices=["x64", "x86", "any"], default="x64",
                        help="ожидаемая разрядность (по умолчанию x64)")
    parser.add_argument("--quiet", action="store_true", help="только итог")
    args = parser.parse_args()

    targets = []
    for item in args.paths:
        if os.path.isdir(item):
            targets += sorted(glob.glob(os.path.join(item, "*.dll")))
        elif any(ch in item for ch in "*?["):
            targets += sorted(glob.glob(item))
        else:
            targets.append(item)

    if not targets:
        print("Не найдено файлов для проверки")
        return 2

    total = 0
    for path in targets:
        if not os.path.isfile(path):
            print("FAIL  %s: файл не найден" % path)
            total += 1
            continue
        total += len(check(path, args.expect, args.quiet))
        print()

    return 1 if total else 0


if __name__ == "__main__":
    sys.exit(main())
