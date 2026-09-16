#!/usr/bin/env bash
#
# Быстрая синтаксическая проверка исходников без Windows.
#
# Компиляция только анализируется (-fsyntax-only), объектные файлы не создаются.
# Заголовки Windows API подменяются минимальными заглушками из tools/stubs,
# поэтому нужен только обычный g++ — ни кросс-компилятора, ни 1С.
#
# Ловит: синтаксические ошибки, несоответствие сигнатур override, ошибки типов,
#        предупреждения компилятора.
# Не ловит: разрядность, реальные зависимости, работу в 1С — для этого
#        tools/build-mingw.sh и tools/check_pe.py.
#
set -euo pipefail
cd "$(dirname "$0")/.."

CXX=${CXX:-g++}
"$CXX" -std=c++17 -fsyntax-only \
    -Wall -Wextra -Werror -Wconversion -Wshadow -Wno-unknown-pragmas \
    -D_WIN32 -D'__declspec(x)=' -D'__stdcall=' \
    -I tools/stubs -I include -I src \
    src/dllmain.cpp src/AddInNative.cpp src/TIJPrinter.cpp

echo "Синтаксическая проверка пройдена: предупреждений нет"
