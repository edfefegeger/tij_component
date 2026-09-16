cmake_minimum_required(VERSION 3.16)
project(TIJPrinter1C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ---------------------------------------------------------------------------
# Статический CRT — принципиальный момент.
#
# С динамическим CRT (/MD, поведение по умолчанию) компонента импортирует
# MSVCP140.dll, VCRUNTIME140.dll и VCRUNTIME140_1.dll и будет подключаться
# только там, где установлен Visual C++ Redistributable x64 нужной версии.
# ИТС прямо рекомендует включать несистемные run-time библиотеки статически:
# "так как на компьютере, где будет использоваться компонента, их может не
# оказаться или они могут быть другой версии".
# ---------------------------------------------------------------------------
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
    # /utf-8 обязателен: в AddInNative.cpp есть русские строковые литералы.
    add_compile_options(/W4 /EHsc /utf-8)
elseif(MINGW)
    # -Wno-unknown-pragmas: MinGW игнорирует #pragma comment(lib, ...) из TIJPrinter.h,
    #   библиотека ws2_32 подключается через target_link_libraries ниже.
    # -fwide-exec-charset: корректные русские строковые литералы (имена методов).
    add_compile_options(-Wall -Wextra -Wno-unknown-pragmas
                        -finput-charset=UTF-8 -fwide-exec-charset=UTF-16LE)
endif()

add_library(TIJPrinter1C SHARED
    src/dllmain.cpp
    src/AddInNative.cpp
    src/TIJPrinter.cpp
)

target_include_directories(TIJPrinter1C PRIVATE include src)
target_link_libraries(TIJPrinter1C PRIVATE ws2_32)

set_target_properties(TIJPrinter1C PROPERTIES
    OUTPUT_NAME "TIJPrinter1C"
    PREFIX ""
)

# Для MinGW: без этих флагов DLL потянет libstdc++-6.dll, libgcc_s_seh-1.dll
# и libwinpthread-1.dll, которых на чистой Windows нет.
if(MINGW)
    target_link_options(TIJPrinter1C PRIVATE -static -static-libgcc -static-libstdc++)
endif()
