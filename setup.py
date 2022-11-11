import os
import sys
import platform
import subprocess
import multiprocessing

from pathlib import Path
from typing import List

from setuptools import setup, Extension, find_packages
from setuptools.command.build_ext import build_ext
from setuptools.command.install import install

# Setuptools calls cmake, which takes care of further
# C++ dependency resolution and the extension build.

# Inspired by:
# https://gist.github.com/hovren/5b62175731433c741d07ee6f482e2936
# https://www.benjack.io/2018/02/02/python-cpp-revisited.html
# Thanks a lot, very helpful :)

binary_files: List[Path] = []

class Install(install):
    def run(self):
        if self.root:
            # setup.py bdist_wheel
            os.makedirs(self.root, exist_ok=True)
            for f in binary_files:
                self.copy_file(f, Path(self.root) / f.name)
        else:
            # setup.py install
            pass
        
        # Continue as normal
        super().run()

class CMakeExtension(Extension):
    def __init__(self, name):
        Extension.__init__(self, name, sources=[])


class CMakeBuild(build_ext):

    def run(self):
        global binary_files

        is_win = platform.system() == 'Windows'

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
            '-DPYTHON_EXECUTABLE=' + sys.executable,
            '-DCMAKE_CXX_STANDARD=17',
        ]

        # determining build type

        cfg = 'Debug' if self.debug else 'Release'
        self.build_args = ['--config', cfg]

        cmake_args += ['-DCMAKE_BUILD_TYPE=' + cfg]

        # parallelize compilation, if using make files

        if not is_win:
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

        # Are we installing or building wheel?
        is_wheel = 'bdist_wheel' in sys.argv

        # Detect binary files to include in wheel
        for ext in self.extensions:
            build_temp = Path(self.build_temp).resolve()
            if platform.system() == 'Windows':
                build_temp = build_temp / 'Release'

            suff = Path(self.get_ext_filename(ext.name)).suffix
            fmts = set([suff, '.dll', '.pyd', '.so'])
            matches = [p for p in build_temp.glob('*') if p.suffix in fmts]
            dest_dir = Path(self.get_ext_fullpath(ext.name)).resolve().parent

            for m in matches:
                binary_files.append(m)
                if not is_wheel:
                    self.copy_file(m, dest_dir / m.name) # tmp => lib


setup(name="imviz",
      version="0.1.22",
      description="Pythonic bindings for imgui/implot",
      url="https://github.com/joruof/imviz",
      author="Jona Ruof",
      author_email="jona.ruof@uni-ulm.de",
      license="MIT",
      zip_safe=False,
      long_description=Path('README.md').read_text('utf-8'),
      long_description_content_type="text/markdown",
      python_requires=">=3.6",
      packages=find_packages(),
      ext_modules=[CMakeExtension("cppimviz")],
      cmdclass=dict(build_ext=CMakeBuild, install=Install),
      include_package_data=True,
      install_requires=[
            "numpy", "zarr>=2.11.3", "Pillow>=9.0.1"
          ]
      )
