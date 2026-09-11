cmake_minimum_required(VERSION 3.16)
project(TIJPrinter1C)

set(CMAKE_CXX_STANDARD 17)

add_library(TIJPrinter1C SHARED
    src/dllmain.cpp
    src/AddInNative.cpp
    src/TIJPrinter.cpp
)

target_include_directories(TIJPrinter1C PRIVATE include src)
target_link_libraries(TIJPrinter1C ws2_32)

set_target_properties(TIJPrinter1C PROPERTIES
    OUTPUT_NAME "TIJPrinter1C"
    PREFIX ""
)
