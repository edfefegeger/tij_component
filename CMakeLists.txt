cmake_minimum_required(VERSION 3.16)
project(TIJPrinter)

set(CMAKE_CXX_STANDARD 17)

# 64-bit DLL
add_library(TIJPrinter SHARED
    src/TIJPrinter.cpp
    src/AddIn.cpp
)

target_include_directories(TIJPrinter PRIVATE include)

target_link_libraries(TIJPrinter ws2_32)

# Output name
set_target_properties(TIJPrinter PROPERTIES
    OUTPUT_NAME "TIJPrinter1C"
    PREFIX ""
)
