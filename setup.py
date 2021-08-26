import os
import sys
import platform
import subprocess
import multiprocessing

from pathlib import Path

from setuptools import setup, Extension, find_packages
from setuptools.command.build_ext import build_ext

# Setuptools calls cmake, which takes care of further
# C++ dependency resolution and the extension build.

# Inspired by:
# https://gist.github.com/hovren/5b62175731433c741d07ee6f482e2936
# https://www.benjack.io/2018/02/02/python-cpp-revisited.html
# Thanks a lot, very helpful :)


class CMakeExtension(Extension):
    def __init__(self, name):
        Extension.__init__(self, name, sources=[])


class CMakeBuild(build_ext):

    def run(self):

        try:
            _ = subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError(
                "CMake must be installed to build the following extensions: " +
                ", ".join(e.name for e in self.extensions))

        # preparing cmake arguments

        cmake_args = [
            ('-DCMAKE_LIBRARY_OUTPUT_DIRECTORY='
                + os.path.abspath(self.build_temp)),
            '-DPYTHON_EXECUTABLE=' + sys.executable
        ]

        # determining build type

        cfg = 'Debug' if self.debug else 'Release'
        self.build_args = ['--config', cfg]

        cmake_args += ['-DCMAKE_BUILD_TYPE=' + cfg]

        # parallelize compilation, if using make files

        if not platform.system() == "Windows":
            cpu_count = multiprocessing.cpu_count()
            self.build_args += ['--', '-j{}'.format(cpu_count)]

        # additional flags

        env = os.environ.copy()
        env['CXXFLAGS'] = '{} -DVERSION_INFO=\\"{}\\"'.format(
                                env.get('CXXFLAGS', ''),
                                self.distribution.get_version())

        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)

        # call cmake to configure and build

        cmake_dir = os.path.abspath(os.path.dirname(__file__))

        subprocess.check_call(['cmake', cmake_dir] + cmake_args,
                              cwd=self.build_temp,
                              env=env)

        cmake_cmd = ['cmake', '--build', '.'] + self.build_args

        subprocess.check_call(cmake_cmd, cwd=self.build_temp)

        # move from temp. build dir to final position

        for ext in self.extensions:

            build_temp = Path(self.build_temp).resolve()
            dest_path = Path(self.get_ext_fullpath(ext.name)).resolve()
            source_path = build_temp / self.get_ext_filename(ext.name)

            dest_directory = dest_path.parents[0]
            dest_directory.mkdir(parents=True, exist_ok=True)

            self.copy_file(source_path, dest_path)


with open("README.md", "r", encoding="utf-8") as fh:
    readme = fh.read()


setup(name="imviz",
      version="0.1.4",
      description="Pythonic bindings for imgui/implot",
      url="https://github.com/joruof/imviz",
      author="Jona Ruof",
      author_email="jona.ruof@uni-ulm.de",
      license="MIT",
      zip_safe=False,
      long_description=readme,
      long_description_content_type="text/markdown",
      python_requires=">=3.6",
      packages=find_packages(),
      ext_modules=[CMakeExtension("imviz")],
      cmdclass=dict(build_ext=CMakeBuild),
      include_package_data=True,
      )
