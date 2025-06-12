from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
import sys
import os
import pybind11

class BuildExt(build_ext):
    def run(self):
        # Ensures CMake is run before building
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        
        # Get the pybind11 cmake directory
        pybind11_cmake_dir = pybind11.get_cmake_dir()

        # Pass pybind11_DIR to cmake
        os.system(f"cmake {os.getcwd()} -B{self.build_temp} -Dpybind11_DIR={pybind11_cmake_dir}")
        os.system(f"cmake -build {self.build_temp} ")
        super().run()

# Get the pybind11 include directory directly
pybind11_include_dir = os.getenv("PYBIND11_INCLUDE", pybind11.get_include())

setup(
    name="binaryreader",
    version="0.1",
    ext_modules=[Extension(
        "binaryreader", 
        ["src/bindings.cc"],
        include_dirs=[pybind11_include_dir,"include"],  # Add this line to include pybind11 headers
        extra_compile_args=['-O3', '-Wall','-std=c++17','-Wno-implicit-function-declaration']  # you can set flags explicitly here
    )],
    cmdclass={"build_ext": BuildExt},
    zip_safe=False,
)
