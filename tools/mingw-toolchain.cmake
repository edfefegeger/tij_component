# Файл инструментов CMake для кросс-сборки под Windows на Linux (MinGW-w64).
#
# Использование:
#     cmake -S . -B build/mingw-x64 -DCMAKE_TOOLCHAIN_FILE=tools/mingw-toolchain.cmake
#     cmake --build build/mingw-x64
#
# Для 32-битной сборки передайте -DTOOLCHAIN_PREFIX=i686-w64-mingw32
# (или используйте tools/build-mingw.sh, где это уже сделано).

set(CMAKE_SYSTEM_NAME Windows)

set(TOOLCHAIN_PREFIX x86_64-w64-mingw32 CACHE STRING "Префикс кросс-компилятора MinGW-w64")

if(TOOLCHAIN_PREFIX MATCHES "^i686")
    set(CMAKE_SYSTEM_PROCESSOR x86)
else()
    set(CMAKE_SYSTEM_PROCESSOR x86_64)
endif()

find_program(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}-gcc     REQUIRED)
find_program(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++     REQUIRED)
find_program(CMAKE_RC_COMPILER  ${TOOLCHAIN_PREFIX}-windres)

set(CMAKE_FIND_ROOT_PATH /usr/${TOOLCHAIN_PREFIX})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
