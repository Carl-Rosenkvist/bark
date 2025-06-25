from setuptools import setup, Extension, find_packages
from setuptools.command.build_ext import build_ext
import os
import shutil
import pybind11
import glob

source_files = ["bindings.cc"] + glob.glob("../src/*.cc") + glob.glob("../analyses/*.cc")

class CustomBuildExt(build_ext):
    def run(self):
        build_temp = self.build_temp

        # Create build directory and run cmake
        os.makedirs(build_temp, exist_ok=True)
        pybind11_cmake_dir = pybind11.get_cmake_dir()
        os.system(f"cmake .. -B{build_temp} -Dpybind11_DIR={pybind11_cmake_dir} -DWITH_PYBIND=ON")
        os.system(f"cmake --build {build_temp} -j")

        # Automatically copy .so file from build/ to bark/
        for filename in os.listdir(build_temp):
            if filename.endswith(".so"):
                shutil.copy(os.path.join(build_temp, filename), "bark/")
                print(f"✔ Copied {filename} to bark/")
                break

        super().run()

setup(
    name="bark",
    version="0.1",
    description="Block-based binary reader for particle data",
    author="Carl B. Rosenkvist",
    packages=find_packages(),  # finds bark/
    cmdclass={"build_ext": CustomBuildExt},
    zip_safe=False,
)
