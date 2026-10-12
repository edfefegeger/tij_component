#!/usr/bin/env python3
"""
Проверка архива компоненты перед отправкой в 1С.

Ловит то, из-за чего подключение возвращает Ложь и по чему трудно догадаться:
  * это действительно ZIP, а не 7z и не RAR (1С читает только ZIP);
  * MANIFEST.XML и INFO.XML лежат в корне архива и разбираются;
  * каждый файл, на который ссылается манифест, реально есть в архиве
    (если собрана только одна разрядность, а манифест требует две,
    платформа компоненту не подключит);
  * библиотеки проходят проверку разрядности, экспортов и зависимостей.

Использование:
    python3 tools/check_package.py TIJPrinter1C.zip
"""

import pathlib
import sys
import tempfile
import xml.etree.ElementTree as ET
import zipfile

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from check_pe import check  # noqa: E402  (проверка одной библиотеки)

MAGIC_ZIP = b"PK"
MAGIC_7Z = b"7z\xbc\xaf\x27\x1c"


def main() -> int:
    if len(sys.argv) < 2:
        print("Укажите архив: python3 tools/check_package.py <архив.zip>")
        return 2

    path = pathlib.Path(sys.argv[1])
    if not path.is_file():
        print(f"Файл не найден: {path}")
        return 2

    problems = []
    notes = []

    head = path.read_bytes()[:8]
    if head.startswith(MAGIC_7Z):
        problems.append("это архив 7z, а 1С читает только ZIP")
    elif not head.startswith(MAGIC_ZIP):
        problems.append("это не ZIP-архив")

    if not problems:
        with zipfile.ZipFile(path) as z:
            names = [n for n in z.namelist() if not n.endswith("/")]

            nested = [n for n in names if "/" in n or "\\" in n]
            if nested:
                problems.append("файлы лежат во вложенной папке: " + ", ".join(nested[:3]))

            for required in ("MANIFEST.XML", "INFO.XML"):
                if required not in names:
                    problems.append(f"в корне архива нет файла {required}")

            declared = []
            if "MANIFEST.XML" in names:
                try:
                    root = ET.fromstring(z.read("MANIFEST.XML").decode("utf-8"))
                    declared = [c.get("path") for c in root if c.get("path")]
                    if not declared:
                        problems.append("в манифесте нет ни одной записи component")
                except ET.ParseError as exc:
                    problems.append(f"MANIFEST.XML не разбирается: {exc}")

            for item in declared:
                if item not in names:
                    problems.append(f"манифест ссылается на {item}, а такого файла в архиве нет")

            extra = [n for n in names if n not in declared + ["MANIFEST.XML", "INFO.XML"]]
            if extra:
                notes.append("лишние файлы в архиве: " + ", ".join(extra[:5]))

            if "INFO.XML" in names:
                try:
                    info = ET.fromstring(z.read("INFO.XML").decode("utf-8"))
                    for c in info:
                        notes.append("карточка заполнится полями: "
                                     + ", ".join(f"{k}={v}" for k, v in c.attrib.items()))
                except ET.ParseError as exc:
                    problems.append(f"INFO.XML не разбирается: {exc}")

            with tempfile.TemporaryDirectory() as tmp:
                for item in declared:
                    if item not in names:
                        continue
                    probe = pathlib.Path(tmp) / item
                    probe.write_bytes(z.read(item))
                    found = check(str(probe), "any", True)
                    if found:
                        problems.append(f"{item}: " + "; ".join(found))

    print(f"Проверка архива: {path.name}")
    print(f"  размер: {path.stat().st_size} байт")
    for n in notes:
        print(f"  [!] {n}")
    if problems:
        for p in problems:
            print(f"  FAIL {p}")
        print()
        print("ИТОГ: отправлять нельзя")
        return 1

    print()
    print("ИТОГ: архив годен, все проверки пройдены")
    return 0


if __name__ == "__main__":
    sys.exit(main())
