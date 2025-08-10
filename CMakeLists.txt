cmake_minimum_required(VERSION 3.14)
project(smash_binary_reader)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include_directories(include)

file(GLOB SRC_FILES src/*.cc)
file(GLOB ANALYSIS_FILES analyses/*.cc)

# Build C++ binary
add_executable(binary_reader ${SRC_FILES} ${ANALYSIS_FILES})

# Find and link yaml-cpp
add_subdirectory(external/yaml-cpp)
target_link_libraries(binary_reader PRIVATE yaml-cpp)

option(WITH_PYBIND "Build Python bindings with pybind11" OFF)

if(WITH_PYBIND)
    find_package(pybind11 REQUIRED)
    pybind11_add_module(bark
        python/bindings.cc
        ${SRC_FILES}
        ${ANALYSIS_FILES}
    )



    set_target_properties(bark PROPERTIES
      OUTPUT_NAME "bark"
      LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/python/bark"
    )
    target_include_directories(bark PRIVATE include)
    target_link_libraries(bark PRIVATE pybind11::module yaml-cpp)
endif()
