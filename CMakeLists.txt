cmake_minimum_required(VERSION 3.14)
project(smash_binary_reader)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Include headers
include_directories(include)

# Source files
file(GLOB SRC_FILES CONFIGURE_DEPENDS src/*.cc)
file(GLOB ANALYSIS_FILES CONFIGURE_DEPENDS analyses/*.cc)

# Third-party: yaml-cpp (vendored)
add_subdirectory(external/yaml-cpp)

# Build executable (like the old CMake)
add_executable(binary_reader ${SRC_FILES} ${ANALYSIS_FILES})
target_link_libraries(binary_reader PRIVATE yaml-cpp)

# Optional: Pybind11 bindings
option(WITH_PYBIND "Build Python bindings with pybind11" OFF)
if(WITH_PYBIND)
    find_package(pybind11 REQUIRED)
    pybind11_add_module(bark
        python/bindings.cc
        ${SRC_FILES}
        ${ANALYSIS_FILES}
    )
    target_include_directories(bark PRIVATE include)
    target_link_libraries(bark PRIVATE yaml-cpp pybind11::module)
   
    set_target_properties(bark PROPERTIES
        OUTPUT_NAME "bark"
    )

    install(TARGETS bark
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

endif()

# Optional: Tests
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    enable_testing()
    file(GLOB TEST_SOURCES CONFIGURE_DEPENDS tests/*.cc)
    if(TEST_SOURCES)
        add_executable(unit_tests ${TEST_SOURCES})
        target_include_directories(unit_tests PRIVATE tests include)
        target_link_libraries(unit_tests PRIVATE yaml-cpp)
        add_test(NAME unit_tests COMMAND unit_tests)
    endif()
endif()
