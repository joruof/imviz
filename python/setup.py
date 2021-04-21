from setuptools import setup

setup(name="imviz",
      version="0.1",
      description="Pythonic bindings for imgui/implot",
      url="https://github.com/joruof/pyimplot",
      author="Jona Ruof",
      author_email="jona.ruof@uni-ulm.de",
      license="MIT",
      zip_safe=True,
      packages=[''],
      package_dir={'': 'build'},
      package_data={'': ["imviz.cpython-38-x86_64-linux-gnu.so"]},
    )

