#!/usr/bin/env bash
#
# Кросс-сборка внешней компоненты под Windows на Linux (MinGW-w64).
#
# ВНИМАНИЕ: MinGW-сборку НЕЛЬЗЯ передавать в 1С на Windows.
# GCC и MSVC по-разному раскладывают таблицу виртуальных методов (разные ABI
# C++), а платформа 1С собрана MSVC и вызывает методы компоненты по своей
# разметке. В результате платформа обращается не к тем методам: по сообщениям
# на форумах первым вызывается деструктор вместо Init, после чего клиент
# падает. Скрипт нужен только для проверки на Linux: разрядность, экспорты,
# зависимости и то, что код компилируется.
# Для сдачи собирайте MSVC: build.cmd, check.cmd, pack.cmd.
#
# После сборки сразу запускается tools/check_pe.py и проверяет разрядность,
# экспорты и зависимости получившегося файла.
#
# Установка компилятора:
#     Ubuntu/Debian: sudo apt install mingw-w64
#
# Использование:
#     tools/build-mingw.sh          # 64-битная сборка
#     tools/build-mingw.sh x86      # 32-битная сборка
#     tools/build-mingw.sh all      # обе
#
set -euo pipefail
cd "$(dirname "$0")/.."

ARCHES=${1:-x64}
[ "$ARCHES" = "all" ] && ARCHES="x64 x86"

for ARCH in $ARCHES; do
    case "$ARCH" in
        x64) TRIPLE=x86_64-w64-mingw32 ;;
        x86) TRIPLE=i686-w64-mingw32 ;;
        *) echo "Неизвестная разрядность: $ARCH (ожидается x64, x86 или all)"; exit 2 ;;
    esac

    if ! command -v "${TRIPLE}-g++" >/dev/null 2>&1; then
        echo "Не найден ${TRIPLE}-g++. Установите: sudo apt install mingw-w64"
        exit 1
    fi

    OUT="build/mingw-${ARCH}"
    mkdir -p "$OUT"

    echo "=== Сборка $ARCH ($TRIPLE) ==="
    # -static -static-libgcc -static-libstdc++ обязательны: без них DLL потянет
    # libstdc++-6.dll, libgcc_s_seh-1.dll и libwinpthread-1.dll, которых на
    # чистой Windows нет — и подключение в 1С будет молча возвращать Ложь.
    # -fwide-exec-charset нужен для корректных русских литералов (имена методов).
    "${TRIPLE}-g++" -std=c++17 -O2 -shared \
        -static -static-libgcc -static-libstdc++ \
        -finput-charset=UTF-8 -fwide-exec-charset=UTF-16LE \
        -Wall -Wextra -Wno-unknown-pragmas \
        -I include -I src \
        src/dllmain.cpp src/AddInNative.cpp src/TIJPrinter.cpp \
        -lws2_32 \
        -o "$OUT/TIJPrinter1C.dll"

    echo "Собрано: $OUT/TIJPrinter1C.dll"
    echo
    python3 tools/check_pe.py "$OUT/TIJPrinter1C.dll" --expect "$ARCH"
    echo
done
