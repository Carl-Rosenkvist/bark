cmake_minimum_required(VERSION 3.14)
project(binary_reader)

set(CMAKE_CXX_STANDARD 17)

find_package(pybind11 REQUIRED)

pybind11_add_module(binaryreader src/bindings.cc include)
