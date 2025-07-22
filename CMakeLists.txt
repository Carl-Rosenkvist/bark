cmake_minimum_required(VERSION 3.14)
project(smash_binary_reader)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include_directories(include)

file(GLOB SRC_FILES src/*.cc)
file(GLOB ANALYSIS_FILES analyses/*.cc)

# Build C++ binary
add_executable(binary_reader ${SRC_FILES} ${ANALYSIS_FILES})

# Find and link yaml-cpp
find_package(yaml-cpp REQUIRED)
target_link_libraries(binary_reader PRIVATE yaml-cpp)

# Optional pybind11 binding
option(WITH_PYBIND "Build Python bindings with pybind11" OFF)

if(WITH_PYBIND)
    find_package(pybind11 REQUIRED)
    pybind11_add_module(_bindings
        python/bindings.cc
        ${SRC_FILES}
        ${ANALYSIS_FILES}
    )
    target_include_directories(_bindings PRIVATE include)
    target_link_libraries(_bindings PRIVATE pybind11::module yaml-cpp)
    set_target_properties(_bindings PROPERTIES OUTPUT_NAME "_bindings")
endif()
