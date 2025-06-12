
cmake_minimum_required(VERSION 3.14)
project(smash_binary_reader)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

file(GLOB SRC_FILES src/*.cc analyses/*.cc)

add_executable(binary_reader ${SRC_FILES})
# Optional pybind11 binding (enable with -DWITH_PYBIND=ON)
option(WITH_PYBIND "Build Python bindings with pybind11" OFF)

if(WITH_PYBIND)
    find_package(pybind11 REQUIRED)
    pybind11_add_module(binaryreader src/bindings.cc)
    target_include_directories(binaryreader PRIVATE include)
endif()
